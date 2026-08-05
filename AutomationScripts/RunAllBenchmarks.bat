@echo off
REM ----------------------------------------------------------------------------
REM  Runs every module's benchmarks and reports an aggregated result. Returns a
REM  non-zero exit code if any module failed so it can be used in automation.
REM
REM  Each module writes its own BenchmarkResults_<Module>.log; this script only
REM  aggregates the outcome. Debug is skipped because those timings are
REM  misleading.
REM
REM  Usage:
REM    RunAllBenchmarks.bat [options]
REM
REM  Options are forwarded to every module runner. See Scripts\RunSuites.bat.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

set "NO_PAUSE=0"
for %%A in (%*) do (
    if /i "%%~A"=="--no-pause" set "NO_PAUSE=1"
)
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"

set "ARGS=%*"

set /a TOTAL=0
set /a FAILED=0

REM  The modules that currently have benchmarks. Add a line here when a new
REM  Run<Module>Benchmarks.bat is added.
call :RunModule Core

echo.
echo ============================================================
echo  Benchmark summary: %TOTAL% module^(s^) run, %FAILED% failed.
echo ============================================================

if %FAILED% gtr 0 (
    echo One or more modules FAILED.
    set "RC=1"
) else (
    echo All modules completed.
    set "RC=0"
)

if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

REM --- Runs one module's benchmark runner and tallies the result -----------
REM   %1 = module name
:RunModule
set "MOD=%~1"
set /a TOTAL+=1

echo.
echo ============================================================
echo  Module: %MOD%
echo ============================================================

REM  The child never pauses; this script pauses once at the end instead.
call "%~dp0Run%MOD%Benchmarks.bat" --no-pause %ARGS%
if errorlevel 1 (
    echo [RESULT] %MOD% benchmarks FAILED
    set /a FAILED+=1
) else (
    echo [RESULT] %MOD% benchmarks PASSED
)

goto :eof
