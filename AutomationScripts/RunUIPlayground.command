#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates the test workspace, builds the UI playground and launches it.
#
#  The playground is a GUI application, not a suite, so it is deliberately
#  absent from RunSuites.sh and from RunAllTests: it needs a window server and
#  cannot run over SSH. Double-click this on the Mac, or run it from a terminal
#  attached to a real login session.
#
#  Usage:
#    RunUIPlayground.command [options]
#
#  Options:
#    --config <name>     Build the named configuration. Defaults to Development.
#    --arch <name>       Build for x86_64, arm64 or universal. Defaults to the
#                        host architecture. A foreign architecture is built but
#                        not launched.
#    --rhi <name>        Force the backend: Metal, Vulkan, D3D12 or Null. Left
#                        alone, the ini files decide.
#    --build-only        Build and stop, without launching.
#    --no-pause          Never wait for a keypress before closing.
#
#  Anything after -- is passed through to the playground unchanged, so
#  console variables can be set the usual way:
#    RunUIPlayground.command -- -ApplicationRenderer.DumpDrawData=1
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so the Homebrew prefixes
# are absent and every tool installed through it is invisible.
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )
cd "$ROOT"

CONFIG="Development"
ARCH=""
RHI=""
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
            --rhi)    RHI="$1" ;;
        esac

        EXPECT_VALUE=""
        shift
        continue
    fi

    case "$1" in
        --config|--arch|--rhi)
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

# Must match GetArchitecturePlatformName(): x86_64 keeps the historical "x64" spelling.
case "$ARCH" in
    arm64)     TOKEN="ARM64" ;;
    universal) TOKEN="Universal" ;;
    *)         TOKEN="x64" ;;
esac

CAN_RUN=1
if [ "$ARCH" != "universal" ] && [ "$ARCH" != "$( uname -m )" ]; then
    CAN_RUN=0
fi

PREMAKE="$ROOT/SetupScripts/Premake/premake5"
WORKSPACE="$ROOT/Solutions/Tests/DXR-Engine Tests.xcworkspace"
TARGET="Application-Playground"

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

echo "------------------------------------------------------------"
echo " Generating test workspace..."
echo "------------------------------------------------------------"

if [ ! -x "$PREMAKE" ]; then
    echo "[ERROR] premake5 not found or not executable: $PREMAKE"
    pause_if_needed
    exit 1
fi

"$PREMAKE" xcode4 --file="$ROOT/Tests/build.lua" --platform=macOS --monolithic --buildsuffix=Tests \
    --architecture="$ARCH"
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to generate the test workspace."
    pause_if_needed
    exit 1
fi

echo
echo "------------------------------------------------------------"
echo " Building $TARGET ($CONFIG | $TOKEN)..."
echo "------------------------------------------------------------"

# A generic destination keeps a cross-architecture build from being filtered out by the run
# destination Xcode would otherwise infer from the host.
xcodebuild -workspace "$WORKSPACE" -scheme "$TARGET" -configuration "$CONFIG" \
    -destination 'generic/platform=macOS' build 2>&1 \
    | grep -E "(error:|Undefined symbols|BUILD FAILED)"
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "[ERROR] Build failed for $TARGET ($CONFIG)."
    pause_if_needed
    exit 1
fi

BUNDLE="$ROOT/Build/bin/$CONFIG-macosx-${TOKEN}-Monolithic-Tests/$TARGET.app"
EXE="$BUNDLE/Contents/MacOS/$TARGET"

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

ARGS=()
if [ -n "$RHI" ]; then
    ARGS+=("-RHI.Type=$RHI")
fi
ARGS+=("${PASSTHROUGH[@]}")

echo
echo "------------------------------------------------------------"
echo " Launching $TARGET..."
echo "------------------------------------------------------------"

# Run from the repo root, since the shader compiler and the font loader both resolve relative to it.
( cd "$ROOT" && "$EXE" "${ARGS[@]}" )
EC=$?

echo
echo "[RESULT] $TARGET exited with code $EC"
pause_if_needed
exit $EC
