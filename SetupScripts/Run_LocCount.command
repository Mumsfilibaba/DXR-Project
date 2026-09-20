#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates the tools workspace, builds LocCount, and launches it.
#
#    Run_LocCount.command [options] [-- loccount-args]
#
#  Options:
#    --config <name>   Build configuration. Defaults to Development.
#    --arch <name>     x86_64, arm64 or universal. Defaults to the host.
#    --build-only      Build and stop, without launching.
#    --no-pause        Never wait for a keypress before closing.
# ----------------------------------------------------------------------------

export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )
cd "$( dirname "${BASH_SOURCE[0]}" )"

CONFIG="Development"
ARCH=""
BUILD_ONLY=0
NO_PAUSE=0
PASSTHROUGH=()
EXPECT_VALUE=""

while [ $# -gt 0 ]; do
    if [ "$1" = "--" ]; then
        shift
        PASSTHROUGH=("$@")
        break
    fi

    if [ -n "$EXPECT_VALUE" ]; then
        case "$1" in
            --*)
                echo "[ERROR] $EXPECT_VALUE requires a value, got: $1"
                exit 2
                ;;
        esac

        case "$EXPECT_VALUE" in
            --config) CONFIG="$1" ;;
            --arch)   ARCH="$1" ;;
        esac
        EXPECT_VALUE=""
        shift
        continue
    fi

    case "$1" in
        --config|--arch)
            EXPECT_VALUE="$1"
            ;;
        --build-only)
            BUILD_ONLY=1
            ;;
        --no-pause)
            NO_PAUSE=1
            ;;
        *)
            echo "[ERROR] Unexpected argument: $1"
            exit 2
            ;;
    esac
    shift
done

if [ -n "$EXPECT_VALUE" ]; then
    echo "[ERROR] $EXPECT_VALUE requires a value."
    exit 2
fi

if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
fi

if [ -z "$ARCH" ]; then
    ARCH=$( uname -m )
fi

case "$ARCH" in
    arm64)     TOKEN="ARM64" ;;
    universal) TOKEN="Universal" ;;
    *)         TOKEN="x64" ;;
esac

CAN_RUN=1
if [ "$ARCH" != "universal" ] && [ "$ARCH" != "$( uname -m )" ]; then
    CAN_RUN=0
fi

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

echo "------------------------------------------------------------"
echo " Generating tools workspace..."
echo "------------------------------------------------------------"

PREMAKE_ARGS=(xcode4 --file=../Tools/build.lua --platform=macOS --monolithic --buildsuffix=Tools)
if [ -n "$ARCH" ]; then
    PREMAKE_ARGS+=("--architecture=${ARCH}")
fi

./Premake/premake5 "${PREMAKE_ARGS[@]}"
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to generate the tools workspace."
    pause_if_needed
    exit 1
fi

WORKSPACE="$ROOT/Solutions/Tools/DXR-Engine Tools.xcworkspace"
if [ ! -d "$WORKSPACE" ]; then
    echo "[ERROR] Workspace not found: $WORKSPACE"
    pause_if_needed
    exit 1
fi

echo
echo "------------------------------------------------------------"
echo " Building LocCount ($CONFIG | $TOKEN)..."
echo "------------------------------------------------------------"

xcodebuild -workspace "$WORKSPACE" -scheme LocCount -configuration "$CONFIG" \
    -destination 'generic/platform=macOS' build
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed for LocCount ($CONFIG)."
    pause_if_needed
    exit 1
fi

BUNDLE="$ROOT/Build/bin/${CONFIG}-macosx-${TOKEN}-Monolithic-Tools/LocCount.app"
EXE="$BUNDLE/Contents/MacOS/LocCount"

if [ $BUILD_ONLY -eq 1 ]; then
    echo
    echo "Built: $BUNDLE"
    pause_if_needed
    exit 0
fi

if [ $CAN_RUN -eq 0 ]; then
    echo
    echo "Built for $ARCH, which the host cannot execute. Not launching."
    pause_if_needed
    exit 0
fi

if [ ! -x "$EXE" ]; then
    echo "[ERROR] Executable not found: $EXE"
    pause_if_needed
    exit 1
fi

echo
echo "------------------------------------------------------------"
echo " Launching LocCount..."
echo "------------------------------------------------------------"

( cd "$ROOT" && "$EXE" "${PASSTHROUGH[@]}" )
EC=$?

echo
echo "[RESULT] LocCount exited with code $EC"
pause_if_needed
exit $EC
