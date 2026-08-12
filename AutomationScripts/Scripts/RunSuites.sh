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
#    --arch <name>       Build for x86_64, arm64 or universal. Defaults to the
#                        host architecture. A suite built for an architecture
#                        the host cannot execute is built but not run.
#
#  The window pauses at the end (on success or failure) so results stay
#  readable when launched interactively. For automation, pass --no-pause or
#  set TESTS_NO_PAUSE=1 to skip the pause.
#
#  Test suites run under a watchdog so a deadlock fails that suite instead of
#  wedging the whole run. Set SUITE_TIMEOUT to change the limit in seconds, or
#  to 0 to disable it, which is what benchmarks default to.
#
#  Benchmarks skip Debug entirely; those timings are misleading. This is the
#  configuration gate that used to live behind RUN_BENCHMARK in Config.h.
#
#  Core's math tests run once per vector backend that the target architecture
#  has: scalar plus SSE through SSE4.2 on x86_64, scalar plus NEON on arm64.
#  Each variant is a separate executable that pins the math backend via
#  PLATFORM_SUPPORT_*_INTRIN defines (see Tests/Core/Core-Math-Tests/Target.lua),
#  so every SIMD and scalar code path is actually compiled and validated.
# ----------------------------------------------------------------------------

# A non-interactive ssh session never runs path_helper, so the Homebrew prefixes
# are absent and every tool installed through it is invisible.
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

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
ARCH=""
EXPECT_VALUE=""

for arg in "$@"; do
    if [ -n "$EXPECT_VALUE" ]; then
        case "$arg" in
            --*)
                echo "[ERROR] $EXPECT_VALUE requires a value, got: $arg"
                exit 2
                ;;
        esac

        if [ "$EXPECT_VALUE" = "--arch" ]; then
            ARCH="$arg"
        else
            ONLY_CONFIG="$arg"
        fi

        EXPECT_VALUE=""
        continue
    fi

    case "$arg" in
        --no-pause)
            NO_PAUSE=1
            ;;
        --config)
            EXPECT_VALUE="--config"
            ;;
        --arch)
            EXPECT_VALUE="--arch"
            ;;
        *)
            echo "[ERROR] Unexpected argument: $arg"
            exit 2
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

if [ -z "$ARCH" ]; then
    ARCH=$( uname -m )
fi

# Must match GetArchitecturePlatformName(): x86_64 keeps the historical "x64" spelling.
case "$ARCH" in
    arm64)     TOKEN="ARM64" ;;
    universal) TOKEN="Universal" ;;
    *)         TOKEN="x64" ;;
esac

# A universal build carries the host's slice, so only a foreign thin build is unrunnable
CAN_RUN=1
if [ "$ARCH" != "universal" ] && [ "$ARCH" != "$( uname -m )" ]; then
    CAN_RUN=0
fi

PREMAKE="$ROOT/SetupScripts/Premake/premake5"
WORKSPACE="$ROOT/Solutions/Tests/DXR-Engine Tests.xcworkspace"

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
                # Must match the target list Tests/build.lua generates for this architecture
                MATH_TARGETS="Core-Math-Tests-Scalar"
                if [ "$ARCH" = "arm64" ]; then
                    MATH_TARGETS="$MATH_TARGETS Core-Math-Tests-NEON"
                else
                    MATH_TARGETS="$MATH_TARGETS \
Core-Math-Tests-SSE Core-Math-Tests-SSE2 Core-Math-Tests-SSE3 \
Core-Math-Tests-SSSE3 Core-Math-Tests-SSE4_1 Core-Math-Tests-SSE4_2"
                fi

                TARGETS="Core-Tests Core-Containers-Tests Core-Templates-Tests $MATH_TARGETS"
                ;;
            RHI)
                TARGETS="RHI-Tests"
                ;;
            RendererCore)
                TARGETS="RendererCore-Tests"
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

# Seconds a single suite may run before the watchdog kills it. Zero disables the
# watchdog, which is the default for benchmarks: those are expected to run far
# longer than any test, so a limit would only ever fire on a healthy run.
if [ "$MODE" = "benchmarks" ]; then
    SUITE_TIMEOUT=${SUITE_TIMEOUT:-0}
else
    SUITE_TIMEOUT=${SUITE_TIMEOUT:-300}
fi

# --- Runs a command under a watchdog, returning its exit code --------------
#  macOS ships no timeout(1), so a background sleep does the job. A killed
#  suite surfaces as exit code 137 (128 + SIGKILL) and is reported as a
#  timeout, which keeps a deadlocked test from blocking the run forever.
run_with_timeout() {
    # exec so the pid below is the suite itself. Without it the pid belongs to the
    # wrapping subshell, and killing that reports a timeout while the suite keeps
    # running, reparented to init and still holding the pipeline's stdout open.
    ( cd "$ROOT" && exec "$@" ) &
    local SuitePid=$!

    if [ "$SUITE_TIMEOUT" -le 0 ]; then
        wait "$SuitePid"
        return $?
    fi

    ( sleep "$SUITE_TIMEOUT"; kill -9 "$SuitePid" 2>/dev/null ) &
    local WatchdogPid=$!

    wait "$SuitePid"
    local SuiteEc=$?

    # Retire the watchdog so it cannot outlive the suite and kill a reused pid.
    kill "$WatchdogPid" 2>/dev/null
    wait "$WatchdogPid" 2>/dev/null

    return $SuiteEc
}

# --- Runs a single suite executable and tallies the result -----------------
run_suite() {
    NAME="$1"
    CONFIG="$2"
    EXE="$BINDIR/$NAME"

    TOTAL=$((TOTAL + 1))

    echo
    echo "------------------------------------------------------------"
    echo " Running $NAME ($CONFIG)..."
    echo "------------------------------------------------------------"

    if [ $CAN_RUN -eq 0 ]; then
        echo "[RESULT] $NAME ($CONFIG) BUILT (not run: $ARCH is not the host architecture)"
        return
    fi

    if [ ! -x "$EXE" ]; then
        echo "[ERROR] Executable not found: $EXE"
        FAILED=$((FAILED + 1))
        return
    fi

    # Run from the repo root so each executable appends to the same log beside
    # it (the harness opens the log with a relative path).
    run_with_timeout "$EXE"
    EC=$?

    # Any non-zero exit code is a failure, including signals from a crash.
    if [ $EC -eq 137 ]; then
        echo "[RESULT] $NAME ($CONFIG) TIMED OUT after ${SUITE_TIMEOUT}s"
        FAILED=$((FAILED + 1))
    elif [ $EC -ne 0 ]; then
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

"$PREMAKE" xcode4 --file="$ROOT/Tests/build.lua" --platform=macOS --monolithic --buildsuffix=Tests \
    --architecture="$ARCH"
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
    echo " Building $MODULE ${KIND}s ($CONFIG | $TOKEN)..."
    echo "------------------------------------------------------------"

    for NAME in $TARGETS; do
        # A generic destination keeps a cross-architecture build from being filtered out by the
        # run destination Xcode would otherwise infer from the host.
        xcodebuild -workspace "$WORKSPACE" -scheme "$NAME" -configuration "$CONFIG" \
            -destination 'generic/platform=macOS' build 2>&1 \
            | grep -E "(error:|Undefined symbols|BUILD FAILED)"
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            echo "[ERROR] Build failed for $NAME ($CONFIG)."
            pause_if_needed
            exit 1
        fi
    done

    # The tests workspace is generated with --monolithic, and a generation whose
    # configuration names do not carry the layout gets it in the output folder instead.
    BINDIR="$ROOT/Build/bin/$CONFIG-macosx-${TOKEN}-Monolithic-Tests"
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
if [ $CAN_RUN -eq 0 ]; then
    echo " $MODULE $KIND summary: $TOTAL (config x suite) built, none run."
else
    echo " $MODULE $KIND summary: $TOTAL (config x suite) run, $FAILED failed."
fi
echo " Full output written to: $LOG"
echo "------------------------------------------------------------"

if [ $FAILED -gt 0 ]; then
    echo "One or more $MODULE $KIND suites FAILED."
    RC=1
elif [ $CAN_RUN -eq 0 ]; then
    echo "All $MODULE $KIND suites BUILT for $ARCH, which the host cannot execute."
    RC=0
else
    echo "All $MODULE $KIND suites PASSED."
    RC=0
fi

pause_if_needed
exit $RC
