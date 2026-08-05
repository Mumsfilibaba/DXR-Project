#!/usr/bin/env bash
# ----------------------------------------------------------------------------
#  Runs every module's benchmarks and reports an aggregated result. Returns a
#  non-zero exit code if any module failed so it can be used in automation.
#  macOS counterpart to RunAllBenchmarks.bat.
#
#  Each module writes its own BenchmarkResults_<Module>.log; this script only
#  aggregates the outcome. Debug is skipped because those timings are
#  misleading.
#
#  Usage:
#    RunAllBenchmarks.command [options]
#
#  Options are forwarded to every module runner. See Scripts/RunSuites.sh.
# ----------------------------------------------------------------------------

export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

NO_PAUSE=0
for arg in "$@"; do
    if [ "$arg" = "--no-pause" ]; then
        NO_PAUSE=1
    fi
done
if [ -n "$TESTS_NO_PAUSE" ]; then
    NO_PAUSE=1
fi

# The modules that currently have benchmarks. Add an entry here when a new
# Run<Module>Benchmarks.command is added.
MODULES="Core"

TOTAL=0
FAILED=0

for MOD in $MODULES; do
    TOTAL=$((TOTAL + 1))

    echo
    echo "============================================================"
    echo " Module: $MOD"
    echo "============================================================"

    # The child never pauses; this script pauses once at the end instead.
    "$DIR/Run${MOD}Benchmarks.command" --no-pause "$@"
    if [ $? -ne 0 ]; then
        echo "[RESULT] $MOD benchmarks FAILED"
        FAILED=$((FAILED + 1))
    else
        echo "[RESULT] $MOD benchmarks PASSED"
    fi
done

echo
echo "============================================================"
echo " Benchmark summary: $TOTAL module(s) run, $FAILED failed."
echo "============================================================"

if [ $FAILED -gt 0 ]; then
    echo "One or more modules FAILED."
    RC=1
else
    echo "All modules completed."
    RC=0
fi

if [ "$NO_PAUSE" -eq 0 ] && [ -t 0 ]; then
    echo
    echo "Press any key to close..."
    read -n 1 -s
fi

exit $RC
