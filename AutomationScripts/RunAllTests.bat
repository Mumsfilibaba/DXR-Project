@echo off
REM ----------------------------------------------------------------------------
REM  Runs every module's test suites and reports an aggregated result. Returns a
REM  non-zero exit code if any module failed so it can be used in automation.
REM
REM  Each module writes its own TestResults_<Module>.log; this script only
REM  aggregates the pass/fail outcome.
REM
REM  Usage:
REM    RunAllTests.bat [options]
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

REM  The modules that currently have test suites. Add a line here when a new
REM  Run<Module>Tests.bat is added.
call :RunModule Core
call :RunModule RHI

echo.
echo ============================================================
echo  Test summary: %TOTAL% module^(s^) run, %FAILED% failed.
echo ============================================================

if %FAILED% gtr 0 (
    echo One or more modules FAILED.
    set "RC=1"
) else (
    echo All modules PASSED.
    set "RC=0"
)

if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

REM --- Runs one module's test runner and tallies the result ----------------
REM   %1 = module name
:RunModule
set "MOD=%~1"
set /a TOTAL+=1

echo.
echo ============================================================
echo  Module: %MOD%
echo ============================================================

REM  The child never pauses; this script pauses once at the end instead.
call "%~dp0Run%MOD%Tests.bat" --no-pause %ARGS%
if errorlevel 1 (
    echo [RESULT] %MOD% tests FAILED
    set /a FAILED+=1
) else (
    echo [RESULT] %MOD% tests PASSED
)

goto :eof
