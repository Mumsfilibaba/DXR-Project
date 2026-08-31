#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates the engine workspace, builds the standalone editor and launches it.
#
#  The editor is a GUI application and needs a window server, so it is absent
#  from RunAllTests and cannot be driven over a plain ssh command. Double-click
#  this on the Mac, or run it from a terminal attached to a login session.
#
#  Usage:
#    RunEditor.command [options]
#
#  Options:
#    --ui <name>         Which editor to build the window from: imgui or custom.
#                        Maps to -Engine.UseCustomEditorUI. Left alone, the ini
#                        files decide.
#    --config <name>     Build the named configuration. Defaults to
#                        "Development Editor".
#    --arch <name>       Build for x86_64, arm64 or universal. Defaults to the
#                        host architecture. A foreign architecture is built but
#                        not launched.
#    --rhi <name>        Force the backend: Metal, Vulkan, D3D12 or Null. Left
#                        alone, the ini files decide.
#    --frames <count>    Exit after this many frames, which is what makes a boot
#                        check possible without a person closing the window.
#    --build-only        Build and stop, without launching.
#    --no-pause          Never wait for a keypress before closing.
#
#  Anything after -- is passed through to the editor unchanged, so console
#  variables can be set the usual way:
#    RunEditor.command --ui custom -- -ApplicationRenderer.DumpDrawData=1
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so the Homebrew prefixes
# are absent and every tool installed through it is invisible.
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )
cd "$ROOT"

CONFIG="Development Editor"
ARCH=""
RHI=""
UI=""
FRAMES=""
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
            --ui)     UI="$1" ;;
            --frames) FRAMES="$1" ;;
        esac

        EXPECT_VALUE=""
        shift
        continue
    fi

    case "$1" in
        --config|--arch|--rhi|--ui|--frames)
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

USE_CUSTOM_UI=""
case "$UI" in
    "")            ;;
    imgui|ImGui)   USE_CUSTOM_UI="false" ;;
    custom|Custom) USE_CUSTOM_UI="true" ;;
    *)
        echo "[ERROR] --ui takes imgui or custom, got: $UI"
        exit 2
        ;;
esac

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

TARGET="SandboxStandalone"

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

echo "------------------------------------------------------------"
echo " Building $TARGET ($CONFIG | $TOKEN)..."
echo "------------------------------------------------------------"

# Generation and the xcodebuild invocation both live in Compile_Xcode.command, which already knows
# the workspace name, the architecture tokens and how to filter the noise out of the build log.
"$ROOT/SetupScripts/Compile_Xcode.command" "$TARGET" "$CONFIG" --arch "$ARCH" --no-pause
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed for $TARGET ($CONFIG)."
    pause_if_needed
    exit 1
fi

BUNDLE="$ROOT/Build/bin/$CONFIG-macosx-${TOKEN}/$TARGET.app"
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
if [ -n "$USE_CUSTOM_UI" ]; then
    ARGS+=("-Engine.UseCustomEditorUI=$USE_CUSTOM_UI")
fi
if [ -n "$FRAMES" ]; then
    ARGS+=("-Engine.ExitAfterFrames=$FRAMES")
fi
ARGS+=("${PASSTHROUGH[@]}")

echo
echo "------------------------------------------------------------"
echo " Launching $TARGET..."
echo "------------------------------------------------------------"

# Run from the repo root, since the shader compiler, the ini layers and the font loader all
# resolve relative to it.
( cd "$ROOT" && "$EXE" "${ARGS[@]}" )
EC=$?

echo
echo "[RESULT] $TARGET exited with code $EC"
pause_if_needed
exit $EC
