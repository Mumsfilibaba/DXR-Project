#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Compiles every engine configuration and reports an aggregated pass/fail
#  result. Returns a non-zero exit code if any configuration failed so it can
#  be used in automation. macOS counterpart to RunCompileTests.bat.
#
#  Warnings are errors here (engine modules only; thirdparty modules set
#  bSilenceWarnings and stay exempt), so this catches the warnings that only
#  appear in configurations nobody builds day to day.
#
#  Everything runs twice, because "Monolithic" is only a configuration *name*
#  in the generated workspace. What actually links the modules statically is
#  the --monolithic flag passed to premake at generation time, so the two
#  layouts need two separate generations:
#
#    Pass 1  modular     -> Solutions/CompileTest      + Build/bin/*-CompileTest
#    Pass 2  monolithic  -> Solutions/CompileTestMono  + Build/bin/*-CompileTestMono
#
#  Both live beside the workspace you work in rather than replacing it, so this
#  is safe to run with Xcode open. The flip side is that the first run is a cold
#  build; later runs are incremental.
#
#  Usage:
#    RunCompileTests.command [options]
#
#  Options:
#    --no-pause          Never wait for a keypress before closing.
#    --clean             Delete the isolated workspaces and binaries first.
#    --modular-only      Skip the monolithic pass.
#    --monolithic-only   Skip the modular pass.
#    --config <name>     Build only the named configuration, e.g. "Release".
#
#  The window pauses at the end (on success or failure) so results stay
#  readable when launched interactively. For automation, pass --no-pause or
#  set TESTS_NO_PAUSE=1 to skip the pause.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so /usr/local/bin is
# absent and every Homebrew tool is invisible.
export PATH="/usr/local/bin:$PATH"

# This script sits one level below the repo root.
ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )
cd "$ROOT"

LOG="${ROOT}/CompileResults.log"
COMPILE="${ROOT}/SetupScripts/Compile_Xcode.command"

NO_PAUSE=0
CLEAN=0
RUN_MODULAR=1
RUN_MONOLITHIC=1
ONLY_CONFIG=""
EXPECT_CONFIG=0

TOTAL=0
FAILED=0

for arg in "$@"; do
    if [ $EXPECT_CONFIG -eq 1 ]; then
        case "$arg" in
            --*)
                echo "[ERROR] --config requires a configuration name, got: $arg"
                exit 2
                ;;
        esac

        ONLY_CONFIG="$arg"
        EXPECT_CONFIG=0
        continue
    fi

    case "$arg" in
        --no-pause)
            NO_PAUSE=1
            ;;
        --clean)
            CLEAN=1
            ;;
        --modular-only)
            RUN_MONOLITHIC=0
            ;;
        --monolithic-only)
            RUN_MODULAR=0
            ;;
        --config)
            EXPECT_CONFIG=1
            ;;
        *)
            echo "[ERROR] Unexpected argument: $arg"
            exit 2
            ;;
    esac
done

if [ $EXPECT_CONFIG -eq 1 ]; then
    echo "[ERROR] --config requires a configuration name."
    exit 2
fi

if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
fi

# Configuration names contain spaces, so they cannot live in the space-separated
# list Scripts/RunSuites.sh uses for its suites.
CONFIGS="Debug
Development
Release
Debug Monolithic
Development Monolithic
Release Monolithic
Debug Editor
Development Editor
Release Editor"

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

# --- Deletes every *CompileTest* folder directly inside the given folder ----
clean_folder() {
    PARENT="$1"
    if [ ! -d "$PARENT" ]; then
        return 0
    fi

    for DIR in "$PARENT"/*CompileTest*; do
        if [ -d "$DIR" ]; then
            echo "  Deleting $DIR"
            rm -rf "$DIR"
        fi
    done
}

# --- Builds a single configuration and tallies the result ------------------
build_config() {
    CFG_NAME="$1"

    CFG_GENERATE="--skip-generate"
    if [ $PASS_GENERATED -eq 0 ]; then
        CFG_GENERATE=""
    fi

    TOTAL=$((TOTAL + 1))
    CFG_START=$(date +%s)

    echo
    echo "------------------------------------------------------------"
    echo " Compiling ${CFG_NAME} (${PASS_NAME})..."
    echo "------------------------------------------------------------"
    echo "----- PASS: ${PASS_NAME} | CONFIG: ${CFG_NAME} -----" >> "$LOG"

    "$COMPILE" all "$CFG_NAME" --suffix "$PASS_SUFFIX" --log "$LOG" --fatal-warnings $PASS_EXTRA $CFG_GENERATE --no-pause
    CFG_RESULT=$?

    # Even a failed build leaves usable project files, so never regenerate twice.
    PASS_GENERATED=1

    CFG_ELAPSED=$(( $(date +%s) - CFG_START ))
    CFG_MINS=$((CFG_ELAPSED / 60))
    CFG_SECS=$((CFG_ELAPSED % 60))

    if [ $CFG_RESULT -ne 0 ]; then
        echo "[RESULT] ${CFG_NAME} (${PASS_NAME}) FAILED (exit code ${CFG_RESULT}, ${CFG_MINS}m ${CFG_SECS}s)"
        FAILED=$((FAILED + 1))
    else
        echo "[RESULT] ${CFG_NAME} (${PASS_NAME}) PASSED (${CFG_MINS}m ${CFG_SECS}s)"
    fi
}

# --- Generates one layout and builds every configuration in it -------------
run_pass() {
    PASS_NAME="$1"
    PASS_SUFFIX="$2"
    PASS_EXTRA="$3"

    # Only the first configuration regenerates; the other eight reuse the result.
    PASS_GENERATED=0

    echo
    echo "============================================================"
    echo " Pass: ${PASS_NAME} (Solutions/${PASS_SUFFIX})"
    echo "============================================================"

    while IFS= read -r CONFIG; do
        if [ -z "$CONFIG" ]; then
            continue
        fi
        if [ -n "$ONLY_CONFIG" ] && [ "$CONFIG" != "$ONLY_CONFIG" ]; then
            continue
        fi

        build_config "$CONFIG"
    done <<< "$CONFIGS"
}

if [ ! -x "$COMPILE" ]; then
    echo "[ERROR] Compile script not found or not executable: $COMPILE"
    echo "        Try: chmod +x \"$COMPILE\""
    pause_if_needed
    exit 1
fi

# --- Optionally start from scratch -----------------------------------------
if [ $CLEAN -eq 1 ]; then
    echo "------------------------------------------------------------"
    echo " Removing previous compile-test output..."
    echo "------------------------------------------------------------"
    clean_folder "${ROOT}/Solutions"
    clean_folder "${ROOT}/Build/bin"
    clean_folder "${ROOT}/Build/bin-int"
fi

# --- Start each run from a clean combined log ------------------------------
rm -f "$LOG"

if [ $RUN_MODULAR -eq 1 ]; then
    run_pass "Modular" "CompileTest" ""
fi

if [ $RUN_MONOLITHIC -eq 1 ]; then
    run_pass "Monolithic" "CompileTestMono" "--monolithic"
fi

if [ $TOTAL -eq 0 ]; then
    echo
    echo "[ERROR] Nothing was built. Check --config against the configuration names."
    pause_if_needed
    exit 1
fi

echo
echo "------------------------------------------------------------"
echo " Compile summary: $TOTAL (pass x config) built, $FAILED failed."
echo " Full output written to: $LOG"
echo "------------------------------------------------------------"
echo "Compile summary: $TOTAL built, $FAILED failed." >> "$LOG"

if [ $FAILED -gt 0 ]; then
    echo "One or more configurations FAILED to compile."
    RC=1
else
    echo "All configurations compiled successfully."
    RC=0
fi

pause_if_needed
exit $RC
