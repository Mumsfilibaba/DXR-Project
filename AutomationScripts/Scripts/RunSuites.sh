#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates, builds and runs the test or benchmark suites belonging to a single
#  engine module on macOS, then reports an aggregated pass/fail result. Returns a
#  non-zero exit code if any suite failed so it can be used in automation.
#  macOS counterpart to RunSuites.bat.
#
#  Usage:
#    RunSuites.sh <Module> <tests|benchmarks> [options]
#
#  Options:
#    --no-pause          Never wait for a keypress before closing.
#    --config <name>     Run only the named configuration, e.g. "Release".
#
#  The window pauses at the end (on success or failure) so results stay
#  readable when launched interactively. For automation, pass --no-pause or
#  set TESTS_NO_PAUSE=1 to skip the pause.
#
#  Benchmarks skip Debug entirely; those timings are misleading. This is the
#  configuration gate that used to live behind RUN_BENCHMARK in Config.h.
#
#  Core's math tests run once per vector backend (scalar + SSE, SSE2, SSE3,
#  SSSE3, SSE4.1, SSE4.2). Each variant is a separate executable that pins the
#  math backend via PLATFORM_SUPPORT_*_INTRIN defines (see Tests/premake5.lua),
#  so every SIMD and scalar code path is actually compiled and validated. This
#  applies unchanged on an Intel Mac.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so /usr/local/bin is
# absent and every Homebrew tool is invisible.
export PATH="/usr/local/bin:$PATH"

# This script sits two levels below the repo root.
ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )
cd "$ROOT"

MODULE="$1"
MODE="$2"
shift 2 2>/dev/null

if [ -z "$MODULE" ] || [ -z "$MODE" ]; then
    echo "[ERROR] Usage: RunSuites.sh <Module> <tests|benchmarks> [options]"
    exit 2
fi

NO_PAUSE=0
ONLY_CONFIG=""
EXPECT_CONFIG=0

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

PREMAKE="$ROOT/SetupScripts/Premake/premake5"
WORKSPACE="$ROOT/Tests/EngineTests.xcworkspace"

RC=0
TOTAL=0
FAILED=0

# --- Resolve the suites and configurations for this module and mode --------
TARGETS=""
CONFIGS="Debug Development Release"
KIND="test"

case "$MODE" in
    tests)
        LOG="$ROOT/TestResults_${MODULE}.log"
        case "$MODULE" in
            Core)
                TARGETS="Core-Tests \
Core-Containers-Tests \
Core-Templates-Tests \
Core-Math-Tests-Scalar \
Core-Math-Tests-SSE \
Core-Math-Tests-SSE2 \
Core-Math-Tests-SSE3 \
Core-Math-Tests-SSSE3 \
Core-Math-Tests-SSE4_1 \
Core-Math-Tests-SSE4_2"
                ;;
            RHI)
                TARGETS="RHI-Tests"
                ;;
        esac
        ;;
    benchmarks)
        KIND="benchmark"
        CONFIGS="Development Release"
        LOG="$ROOT/BenchmarkResults_${MODULE}.log"
        case "$MODULE" in
            Core)
                TARGETS="Core-Benchmarks"
                ;;
        esac
        ;;
    *)
        echo "[ERROR] Unknown mode '$MODE'. Expected 'tests' or 'benchmarks'."
        exit 2
        ;;
esac

if [ -z "$TARGETS" ]; then
    echo "[ERROR] No $MODE are registered for module '$MODULE'."
    echo "        Add it to the dispatch block in $( basename "${BASH_SOURCE[0]}" )."
    exit 2
fi

pause_if_needed() {
    if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
        echo
        echo "Press any key to close..."
        read -n 1 -s
    fi
}

# --- Runs a single suite executable and tallies the result -----------------
run_suite() {
    NAME="$1"
    CONFIG="$2"
    EXE="$BINDIR/$NAME/$NAME"

    TOTAL=$((TOTAL + 1))

    echo
    echo "------------------------------------------------------------"
    echo " Running $NAME ($CONFIG)..."
    echo "------------------------------------------------------------"

    if [ ! -x "$EXE" ]; then
        echo "[ERROR] Executable not found: $EXE"
        FAILED=$((FAILED + 1))
        return
    fi

    # Run from the repo root so each executable appends to the same log beside
    # it (the harness opens the log with a relative path).
    ( cd "$ROOT" && "$EXE" )
    EC=$?

    # Any non-zero exit code is a failure, including signals from a crash.
    if [ $EC -ne 0 ]; then
        echo "[RESULT] $NAME ($CONFIG) FAILED (exit code $EC)"
        FAILED=$((FAILED + 1))
    else
        echo "[RESULT] $NAME ($CONFIG) PASSED"
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

"$PREMAKE" xcode4 --file="$ROOT/Tests/premake5.lua"
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to generate the test workspace."
    pause_if_needed
    exit 1
fi

if [ ! -d "$WORKSPACE" ]; then
    echo "[ERROR] Test workspace not found: $WORKSPACE"
    pause_if_needed
    exit 1
fi

# --- Start each run from a clean combined log ------------------------------
rm -f "$LOG"

# --- Build and run every suite under each configuration --------------------
RAN_ANY=0
for CONFIG in $CONFIGS; do
    if [ -n "$ONLY_CONFIG" ] && [ "$CONFIG" != "$ONLY_CONFIG" ]; then
        continue
    fi

    RAN_ANY=1

    echo
    echo "------------------------------------------------------------"
    echo " Building $MODULE ${KIND}s ($CONFIG | x64)..."
    echo "------------------------------------------------------------"

    for NAME in $TARGETS; do
        xcodebuild -workspace "$WORKSPACE" -scheme "$NAME" -configuration "$CONFIG" build 2>&1 \
            | grep -E "(error:|Undefined symbols|BUILD FAILED)"
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            echo "[ERROR] Build failed for $NAME ($CONFIG)."
            pause_if_needed
            exit 1
        fi
    done

    BINDIR="$ROOT/Tests/Build/bin/$CONFIG-macosx-x64"
    echo "----- MODULE: $MODULE | CONFIG: $CONFIG -----" >> "$LOG"

    for NAME in $TARGETS; do
        run_suite "$NAME" "$CONFIG"
    done
done

if [ $RAN_ANY -eq 0 ]; then
    echo
    echo "[ERROR] Nothing was run. Check --config against the configuration names."
    pause_if_needed
    exit 1
fi

echo
echo "------------------------------------------------------------"
echo " $MODULE $KIND summary: $TOTAL (config x suite) run, $FAILED failed."
echo " Full output written to: $LOG"
echo "------------------------------------------------------------"

if [ $FAILED -gt 0 ]; then
    echo "One or more $MODULE $KIND suites FAILED."
    RC=1
else
    echo "All $MODULE $KIND suites PASSED."
    RC=0
fi

pause_if_needed
exit $RC
