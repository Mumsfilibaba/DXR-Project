@echo off
REM ----------------------------------------------------------------------------
REM  Generates, builds and runs every engine test suite, then reports an
REM  aggregated pass/fail result. Returns a non-zero exit code if any suite
REM  failed so it can be used in automation.
REM
REM  The window pauses at the end (on success or failure) so results stay
REM  readable when launched interactively. For automation, pass --no-pause or
REM  set TESTS_NO_PAUSE=1 to skip the pause.
REM
REM  The Containers-Tests suite also runs its benchmarks in the Development and
REM  Release configurations (gated by RUN_BENCHMARK in Config.h); they are off in
REM  Debug because those timings are misleading.
REM
REM  MathLib is run once per vector backend (scalar + SSE, SSE2, SSE3, SSSE3,
REM  SSE4.1, SSE4.2). Each variant is a separate executable that pins the math
REM  backend via PLATFORM_SUPPORT_*_INTRIN defines (see Tests/premake5.lua), so
REM  every SIMD and scalar code path is actually compiled and validated.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

REM --- Decide whether to wait for a keypress before closing -----------------
set "NO_PAUSE=0"
if /i "%~1"=="--no-pause" set "NO_PAUSE=1"
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"
set "RC=0"

set "ROOT=%~dp0"
set "PREMAKE=%ROOT%SetupScripts\Premake\premake5.exe"
set "SOLUTION=%ROOT%Tests\EngineTests.sln"

echo ------------------------------------------------------------
echo  Generating test solution...
echo ------------------------------------------------------------
"%PREMAKE%" vs2022 --file="%ROOT%Tests\premake5.lua"
if errorlevel 1 (
    echo [ERROR] Failed to generate the test solution.
    set "RC=1" & goto Finish
)

REM --- Locate MSBuild through vswhere ---------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    set "RC=1" & goto Finish
)

set "MSBUILD="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
if not defined MSBUILD (
    echo [ERROR] Could not locate MSBuild.exe.
    set "RC=1" & goto Finish
)

set /a TOTAL=0
set /a FAILED=0

REM --- Start each run from a clean combined log -----------------------------
set "LOG=%ROOT%TestResults.log"
del /q "%LOG%" 2>nul

REM --- Build and run every suite under each configuration -------------------
for %%C in (Debug Development Release) do (
    echo.
    echo ------------------------------------------------------------
    echo  Building tests ^(%%C ^| x64^)...
    echo ------------------------------------------------------------
    "%MSBUILD%" "%SOLUTION%" /p:Configuration=%%C /p:Platform=x64 /m /nologo /verbosity:minimal
    if errorlevel 1 (
        echo [ERROR] Build failed for configuration %%C.
        set "RC=1" & goto Finish
    )

    set "BINDIR=%ROOT%Tests\Build\bin\%%C-windows-x64"

    echo ----- CONFIG: %%C ----->> "%LOG%"

    REM Run the suites from the repo root so each exe appends to the same
    REM %ROOT%\TestResults.log (the harness opens it with a relative path).
    pushd "%ROOT%"
    call :RunSuite "Containers-Tests" "%%C"
    REM MathLib runs once per vector backend (scalar + each SSE level) so every math
    REM code path is compiled and validated, not just the highest SSE level.
    for %%M in (Scalar SSE SSE2 SSE3 SSSE3 SSE4_1 SSE4_2) do (
        call :RunSuite "MathLib-Tests-%%M" "%%C"
    )
    call :RunSuite "Templates-Tests" "%%C"
    call :RunSuite "CoreTests" "%%C"
    popd
)

echo.
echo ------------------------------------------------------------
echo  Test summary: !TOTAL! ^(config x suite^) run, !FAILED! failed.
echo  Full output written to: %LOG%
echo ------------------------------------------------------------
if !FAILED! gtr 0 (
    echo One or more test suites FAILED.
    set "RC=1" & goto Finish
)

echo All test suites PASSED.
set "RC=0" & goto Finish

REM --- Pause (when interactive) and exit with the saved result code ---------
:Finish
if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

REM --- Runs a single suite executable and tallies the result ---------------
REM   %1 = suite name, %2 = configuration label (for reporting)
:RunSuite
set "NAME=%~1"
set "CONFIG=%~2"
set "EXE=!BINDIR!\%NAME%\%NAME%.exe"
set /a TOTAL+=1

echo.
echo ------------------------------------------------------------
echo  Running %NAME% ^(%CONFIG%^)...
echo ------------------------------------------------------------
if not exist "!EXE!" (
    echo [ERROR] Executable not found: !EXE!
    set /a FAILED+=1
    goto :eof
)

"!EXE!"
set "EC=!errorlevel!"
REM Any non-zero exit code is a failure, including negative crash codes
REM (e.g. -1073741819 / 0xC0000005 access violation), which "geq 1" would miss.
if !EC! neq 0 (
    echo [RESULT] %NAME% ^(%CONFIG%^) FAILED ^(exit code !EC!^)
    set /a FAILED+=1
) else (
    echo [RESULT] %NAME% ^(%CONFIG%^) PASSED
)

goto :eof
