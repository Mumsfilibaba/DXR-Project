#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Boots every RHI the host can load, first without a window (device + default
#  resources + a CreateBuffer/CreateTexture probe) and then, when a window
#  server is present, through two editor frames.
#
#  Metal is not expected to render a complete frame. The editor path records
#  how far [BOOT] checkpoints get so the known-good point is in the log.
#
#  Usage:
#    RunBootChecks.command [options]
#
#  Options:
#    --config <name>     Test-suite configuration. Defaults to Development.
#    --arch <name>       x86_64, arm64 or universal. Defaults to the host.
#    --skip-editor       Never launch SandboxStandalone, even with a GUI.
#    --no-pause          Never wait for a keypress before closing.
# ----------------------------------------------------------------------------

export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )
cd "$ROOT"

CONFIG="Development"
ARCH=""
SKIP_EDITOR=0
NO_PAUSE=0
EXPECT_VALUE=""

while [ $# -gt 0 ]; do
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
        --skip-editor)
            SKIP_EDITOR=1
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

PREMAKE="$ROOT/SetupScripts/Premake/premake5"
WORKSPACE="$ROOT/Solutions/Tests/DXR-Engine Tests.xcworkspace"
LOG="$ROOT/BootResults.log"
FAILED=0

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

has_window_server() {
    [ "$(launchctl managername 2>/dev/null)" = "Aqua" ]
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
echo " Building RHI-Boot-Tests ($CONFIG | $TOKEN)..."
echo "------------------------------------------------------------"

xcodebuild -workspace "$WORKSPACE" -scheme "RHI-Boot-Tests" -configuration "$CONFIG" \
    -destination 'generic/platform=macOS' build 2>&1 \
    | grep -E "(error:|Undefined symbols|BUILD FAILED)"
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "[ERROR] Build failed for RHI-Boot-Tests ($CONFIG)."
    pause_if_needed
    exit 1
fi

rm -f "$LOG"
echo "----- RHI-Boot-Tests | CONFIG: $CONFIG -----" >> "$LOG"

BINDIR="$ROOT/Build/bin/$CONFIG-macosx-${TOKEN}-Monolithic-Tests"
EXE="$BINDIR/RHI-Boot-Tests"

if [ $CAN_RUN -eq 0 ]; then
    echo "Built for $ARCH, which the host cannot execute. Not running RHI-Boot-Tests."
else
    echo
    echo "------------------------------------------------------------"
    echo " Running RHI-Boot-Tests..."
    echo "------------------------------------------------------------"

    ( cd "$ROOT" && "$EXE" )
    EC=$?
    if [ $EC -ne 0 ]; then
        echo "[RESULT] RHI-Boot-Tests FAILED (exit code $EC)"
        FAILED=$((FAILED + 1))
    else
        echo "[RESULT] RHI-Boot-Tests PASSED"
    fi
fi

if [ $SKIP_EDITOR -eq 0 ] && has_window_server && [ $CAN_RUN -eq 1 ]; then
    echo
    echo "------------------------------------------------------------"
    echo " Building SandboxStandalone (Development Editor | $TOKEN)..."
    echo "------------------------------------------------------------"

    "$ROOT/AutomationScripts/RunEditor.command" --build-only --no-pause --config "Development Editor" --arch "$ARCH"
    if [ $? -ne 0 ]; then
        echo "[ERROR] SandboxStandalone build failed."
        pause_if_needed
        exit 1
    fi

    EDITOR_EXE="$ROOT/Build/bin/Development Editor-macosx-${TOKEN}/SandboxStandalone.app/Contents/MacOS/SandboxStandalone"
    OUTPUT_LOG="$ROOT/Sandbox/OutputLog.txt"

    run_editor_boot() {
        local RHI="$1"
        local UI="$2"
        local COPY="$ROOT/BootResults_Editor_${RHI}_${UI}.log"

        echo
        echo "------------------------------------------------------------"
        echo " Editor boot: RHI=$RHI UI=$UI (2 frames)..."
        echo "------------------------------------------------------------"

        ( cd "$ROOT" && "$EDITOR_EXE" \
            "-RHI.Type=$RHI" \
            "-Engine.UseCustomEditorUI=$UI" \
            "-Engine.ExitAfterFrames=2" )
        local EditorEc=$?

        if [ -f "$OUTPUT_LOG" ]; then
            cp "$OUTPUT_LOG" "$COPY"
            echo " Log copy: $COPY"
            echo " Last [BOOT] lines:"
            grep "\[BOOT\]" "$COPY" || echo "  (none)"
        fi

        if [ "$RHI" = "Metal" ]; then
            if grep -q "\[BOOT\] RHI initialized type=Metal" "$COPY" 2>/dev/null; then
                echo "[RESULT] Editor Metal reached RHI initialize (exit $EditorEc)"
            else
                echo "[RESULT] Editor Metal FAILED before RHI initialize (exit $EditorEc)"
                FAILED=$((FAILED + 1))
            fi
        else
            if grep -q "\[BOOT\] ExitAfterFrames reached" "$COPY" 2>/dev/null; then
                echo "[RESULT] Editor $RHI ($UI) PASSED"
            else
                echo "[RESULT] Editor $RHI ($UI) FAILED (exit $EditorEc)"
                FAILED=$((FAILED + 1))
            fi
        fi
    }

    for RHI in Null Vulkan Metal; do
        run_editor_boot "$RHI" true
    done

    run_editor_boot Vulkan false
    run_editor_boot Metal false
else
    if [ $SKIP_EDITOR -eq 1 ]; then
        echo
        echo "Skipping editor boots (--skip-editor)."
    elif [ $CAN_RUN -eq 0 ]; then
        echo
        echo "Skipping editor boots (foreign architecture)."
    else
        echo
        echo "Skipping editor boots: no window server (launchctl managername is not Aqua)."
        echo "Run this from a login-session terminal, or double-click it on the Mac."
    fi
fi

echo
echo "------------------------------------------------------------"
if [ $FAILED -gt 0 ]; then
    echo " Boot checks FAILED ($FAILED)."
    echo " Headless log: $ROOT/TestResults_RHIBoot.log"
    echo " Combined:     $LOG"
    pause_if_needed
    exit 1
fi

echo " Boot checks PASSED."
echo " Headless log: $ROOT/TestResults_RHIBoot.log"
pause_if_needed
exit 0
