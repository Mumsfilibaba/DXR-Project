#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates the Xcode workspace and then compiles a scheme from the command
#  line. The Generate_* scripts only produce project files; this one builds.
#
#    Compile_Xcode.command [scheme] [configuration] [options]
#
#  Defaults to the SandboxStandalone executable in the "Development Editor"
#  configuration. SandboxStandalone is the startup executable; the bare Sandbox
#  scheme builds only the game module, and MetalRHI alone is faster still. Pass
#  "all" as the scheme to build every scheme in the workspace instead.
#
#  Options:
#    --no-pause        Never wait for a keypress before closing.
#    --skip-generate   Build the existing workspace without regenerating it.
#    --fatal-warnings  Treat compiler warnings as errors (engine modules only).
#    --monolithic      Link all modules statically into the executable.
#    --suffix <name>   Generate into Solutions/<name> and write binaries to
#                      Build/bin/<config>-macosx-x64-<name>, so a build can run
#                      without disturbing the normal workspace.
#    --log <path>      Append the build transcript to <path> instead of
#                      overwriting the default CompileXcode.log.
#
#  A successful build is followed by VerifyBundle.command. Returns the exit code
#  of whichever step failed so it can be used in automation. The window pauses at
#  the end when interactive; pass --no-pause or set TESTS_NO_PAUSE=1 to skip that.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so /usr/local/bin is
# absent and every Homebrew tool is invisible.
export PATH="/usr/local/bin:$PATH"

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

ROOT=$( cd "${DIR}/.." && pwd )
LOG="${ROOT}/CompileXcode.log"

SCHEME="SandboxStandalone"
CONFIG="Development Editor"
SUFFIX=""
NO_PAUSE=0
SKIP_GENERATE=0
FATAL_WARNINGS=0
MONOLITHIC=0
KEEP_LOG=0
POSITIONAL=0
EXPECT_VALUE=""

for arg in "$@"; do
    if [ -n "$EXPECT_VALUE" ]; then
        # Catches "--suffix --no-pause", where the option swallows the following flag
        # and would otherwise generate into a folder literally named "--no-pause".
        case "$arg" in
            --*)
                echo "[ERROR] $EXPECT_VALUE requires a value, got: $arg"
                exit 2
                ;;
        esac

        if [ "$EXPECT_VALUE" = "--suffix" ]; then
            SUFFIX="$arg"
        else
            LOG="$arg"
            KEEP_LOG=1
        fi

        EXPECT_VALUE=""
        continue
    fi

    case "$arg" in
        --no-pause)
            NO_PAUSE=1
            ;;
        --skip-generate)
            SKIP_GENERATE=1
            ;;
        --fatal-warnings)
            FATAL_WARNINGS=1
            ;;
        --monolithic)
            MONOLITHIC=1
            ;;
        --suffix)
            EXPECT_VALUE="--suffix"
            ;;
        --log)
            EXPECT_VALUE="--log"
            ;;
        *)
            if [ $POSITIONAL -eq 0 ]; then
                SCHEME="$arg"
                POSITIONAL=1
            elif [ $POSITIONAL -eq 1 ]; then
                CONFIG="$arg"
                POSITIONAL=2
            else
                echo "[ERROR] Unexpected argument: $arg"
                exit 2
            fi
            ;;
    esac
done

if [ -n "$EXPECT_VALUE" ]; then
    echo "[ERROR] $EXPECT_VALUE requires a value."
    exit 2
fi

if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
fi

# A suffix moves the whole generation into its own folder
if [ -n "$SUFFIX" ]; then
    WORKSPACE="${ROOT}/Solutions/${SUFFIX}/DXR-Engine Sandbox.xcworkspace"
else
    WORKSPACE="${ROOT}/Solutions/DXR-Engine Sandbox.xcworkspace"
fi

PREMAKE_ARGS=(xcode4 --file=../build.lua --platform=macOS)
if [ -n "$SUFFIX" ]; then
    PREMAKE_ARGS+=("--buildsuffix=${SUFFIX}")
fi
if [ $FATAL_WARNINGS -eq 1 ]; then
    PREMAKE_ARGS+=(--fatalwarnings)
fi
if [ $MONOLITHIC -eq 1 ]; then
    PREMAKE_ARGS+=(--monolithic)
fi

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

if [ $SKIP_GENERATE -eq 0 ]; then
    echo "------------------------------------------------------------"
    echo " Generating Xcode workspace..."
    echo "------------------------------------------------------------"
    ./Premake/premake5 "${PREMAKE_ARGS[@]}"
    if [ $? -ne 0 ]; then
        echo "[ERROR] Failed to generate the Xcode workspace."
        pause_if_needed
        exit 1
    fi
fi

if [ ! -d "$WORKSPACE" ]; then
    echo "[ERROR] Workspace not found: $WORKSPACE"
    pause_if_needed
    exit 1
fi

# xcodebuild has no "build everything" switch for a workspace, so the scheme list is
# read back from the generated workspace. RunCompileTests.command relies on that,
# since SandboxStandalone is only generated for a non-monolithic build and
# ImGuiPlugin is loaded at runtime rather than linked.
if [ "$SCHEME" = "all" ]; then
    SCHEMES=$(xcodebuild -workspace "$WORKSPACE" -list 2>/dev/null \
        | awk '/Schemes:/ {found=1; next} found && NF {sub(/^[ \t]+/, ""); print}')
    if [ -z "$SCHEMES" ]; then
        echo "[ERROR] Could not list any schemes in: $WORKSPACE"
        pause_if_needed
        exit 1
    fi
else
    SCHEMES="$SCHEME"
fi

# A driver passing --log owns the file and collects every configuration into it.
if [ $KEEP_LOG -eq 0 ]; then
    : > "$LOG"
fi

RC=0

while IFS= read -r NAME; do
    if [ -z "$NAME" ]; then
        continue
    fi

    echo
    echo "------------------------------------------------------------"
    echo " Building ${NAME} (${CONFIG})..."
    echo "------------------------------------------------------------"

    # The build is warning-free, so warnings are surfaced rather than filtered out. The full
    # transcript is still kept in $LOG.
    xcodebuild -workspace "$WORKSPACE" -scheme "$NAME" -configuration "$CONFIG" build 2>&1 \
        | tee -a "$LOG" \
        | grep -E "(error:|warning:|Undefined symbols|BUILD (SUCCEEDED|FAILED))"
    EC=${PIPESTATUS[0]}

    if [ $EC -ne 0 ]; then
        RC=$EC
        break
    fi
done <<< "$SCHEMES"

echo
echo "------------------------------------------------------------"
if [ $RC -eq 0 ]; then
    echo " Build SUCCEEDED."
else
    echo " Build FAILED (exit code $RC)."
fi
echo " Full log: $LOG"
echo "------------------------------------------------------------"

if [ $RC -eq 0 ]; then
    echo
    ./VerifyBundle.command "$CONFIG" --no-pause
    RC=$?
fi

pause_if_needed
exit $RC
