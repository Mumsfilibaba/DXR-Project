#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates the Xcode workspace and then compiles a scheme from the command
#  line. The Generate_* scripts only produce project files; this one builds.
#
#    Compile_Xcode.command [scheme] [configuration] [--no-pause] [--skip-generate]
#
#  Defaults to the SandboxStandalone executable in the "Development Editor"
#  configuration. SandboxStandalone is the startup executable; the bare Sandbox
#  scheme builds only the game module, and MetalRHI alone is faster still.
#
#  Returns xcodebuild's exit code so it can be used in automation. The window
#  pauses at the end when interactive; pass --no-pause or set TESTS_NO_PAUSE=1
#  to skip that.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so /usr/local/bin is
# absent and every Homebrew tool is invisible.
export PATH="/usr/local/bin:$PATH"

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

ROOT=$( cd "${DIR}/.." && pwd )
WORKSPACE="${ROOT}/Solutions/DXR-Engine Sandbox.xcworkspace"
LOG="${ROOT}/CompileXcode.log"

SCHEME="SandboxStandalone"
CONFIG="Development Editor"
NO_PAUSE=0
SKIP_GENERATE=0
POSITIONAL=0

for arg in "$@"; do
    case "$arg" in
        --no-pause)
            NO_PAUSE=1
            ;;
        --skip-generate)
            SKIP_GENERATE=1
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

if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
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
    ./Premake/premake5 xcode4 --file=../build.lua --platform=macOS
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

echo
echo "------------------------------------------------------------"
echo " Building ${SCHEME} (${CONFIG})..."
echo "------------------------------------------------------------"

# The generated projects emit dozens of "file reference is a member of multiple
# groups" warnings on every invocation, which bury real errors. Only errors and
# the final result are shown; the full transcript is kept in $LOG.
xcodebuild -workspace "$WORKSPACE" -scheme "$SCHEME" -configuration "$CONFIG" build 2>&1 \
    | tee "$LOG" \
    | grep -E "(error:|Undefined symbols|BUILD (SUCCEEDED|FAILED))"
RC=${PIPESTATUS[0]}

echo
echo "------------------------------------------------------------"
if [ $RC -eq 0 ]; then
    echo " Build SUCCEEDED."
else
    echo " Build FAILED (exit code $RC)."
fi
echo " Full log: $LOG"
echo "------------------------------------------------------------"

pause_if_needed
exit $RC
