#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Generates, builds and runs every engine test suite on macOS, then reports an
#  aggregated pass/fail result. Returns a non-zero exit code if any suite
#  failed so it can be used in automation. macOS counterpart to RunTests.bat.
#
#  The window pauses at the end (on success or failure) so results stay
#  readable when launched interactively. For automation, pass --no-pause or
#  set TESTS_NO_PAUSE=1 to skip the pause.
#
#  The Containers-Tests suite also runs its benchmarks in the Development and
#  Release configurations (gated by RUN_BENCHMARK in Config.h); they are off in
#  Debug because those timings are misleading.
#
#  MathLib is run once per vector backend (scalar + SSE, SSE2, SSE3, SSSE3,
#  SSE4.1, SSE4.2). Each variant is a separate executable that pins the math
#  backend via PLATFORM_SUPPORT_*_INTRIN defines (see Tests/premake5.lua), so
#  every SIMD and scalar code path is actually compiled and validated. This
#  applies unchanged on an Intel Mac.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so /usr/local/bin is
# absent and every Homebrew tool is invisible.
export PATH="/usr/local/bin:$PATH"

ROOT=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "$ROOT"

NO_PAUSE=0
for arg in "$@"; do
    if [ "$arg" = "--no-pause" ]; then
        NO_PAUSE=1
    fi
done
if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
fi

PREMAKE="$ROOT/SetupScripts/Premake/premake5"
WORKSPACE="$ROOT/Tests/EngineTests.xcworkspace"
LOG="$ROOT/TestResults.log"

RC=0
TOTAL=0
FAILED=0

CONFIGS="Debug Development Release"
SUITES="Containers-Tests \
MathLib-Tests-Scalar \
MathLib-Tests-SSE \
MathLib-Tests-SSE2 \
MathLib-Tests-SSE3 \
MathLib-Tests-SSSE3 \
MathLib-Tests-SSE4_1 \
MathLib-Tests-SSE4_2 \
Templates-Tests \
CoreTests"

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

    # Run from the repo root so each executable appends to the same
    # TestResults.log (the harness opens it with a relative path).
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
for CONFIG in $CONFIGS; do
    echo
    echo "------------------------------------------------------------"
    echo " Building tests ($CONFIG | x64)..."
    echo "------------------------------------------------------------"

    for NAME in $SUITES; do
        xcodebuild -workspace "$WORKSPACE" -scheme "$NAME" -configuration "$CONFIG" build 2>&1 \
            | grep -E "(error:|Undefined symbols|BUILD FAILED)"
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            echo "[ERROR] Build failed for $NAME ($CONFIG)."
            RC=1
            pause_if_needed
            exit $RC
        fi
    done

    BINDIR="$ROOT/Tests/Build/bin/$CONFIG-macosx-x64"
    echo "----- CONFIG: $CONFIG -----" >> "$LOG"

    for NAME in $SUITES; do
        run_suite "$NAME" "$CONFIG"
    done
done

echo
echo "------------------------------------------------------------"
echo " Test summary: $TOTAL (config x suite) run, $FAILED failed."
echo " Full output written to: $LOG"
echo "------------------------------------------------------------"

if [ $FAILED -gt 0 ]; then
    echo "One or more test suites FAILED."
    RC=1
else
    echo "All test suites PASSED."
    RC=0
fi

pause_if_needed
exit $RC
