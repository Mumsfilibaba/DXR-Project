@echo off
REM ----------------------------------------------------------------------------
REM  Generates, builds and runs the test or benchmark suites belonging to a
REM  single engine module, then reports an aggregated pass/fail result. Returns a
REM  non-zero exit code if any suite failed so it can be used in automation.
REM
REM  Usage:
REM    RunSuites.bat <Module> <tests^|benchmarks> [options]
REM
REM  Options:
REM    --no-pause          Never wait for a keypress before closing.
REM    --config <name>     Run only the named configuration, e.g. "Release".
REM
REM  The window pauses at the end (on success or failure) so results stay
REM  readable when launched interactively. For automation, pass --no-pause or
REM  set TESTS_NO_PAUSE=1 to skip the pause.
REM
REM  Benchmarks skip Debug entirely; those timings are misleading. This is the
REM  configuration gate that used to live behind RUN_BENCHMARK in Config.h.
REM
REM  Core's math tests run once per vector backend (scalar + SSE, SSE2, SSE3,
REM  SSSE3, SSE4.1, SSE4.2). Each variant is a separate executable that pins the
REM  math backend via PLATFORM_SUPPORT_*_INTRIN defines (see Tests/premake5.lua),
REM  so every SIMD and scalar code path is actually compiled and validated.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

REM  Resolve everything derived from %0 before the first shift, because shift
REM  moves %0 along with the rest of the arguments.
REM
REM  This script sits two levels below the repo root; resolve it to a full path
REM  so logged paths do not carry a "..\..\" through every message.
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI\"
set "SCRIPT_NAME=%~nx0"

set "MODULE=%~1"
set "MODE=%~2"
shift /1
shift /1

set "NO_PAUSE=0"
set "ONLY_CONFIG="
set "RC=0"

if not defined MODULE (
    echo [ERROR] Usage: RunSuites.bat ^<Module^> ^<tests^|benchmarks^> [options]
    exit /b 2
)

REM --- Parse arguments -------------------------------------------------------
:ParseArgs
if "%~1"=="" goto AfterArgs
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--config" (
    if "%~2"=="" (
        echo [ERROR] --config requires a configuration name.
        set "RC=2" & goto Finish
    )
    set "ONLY_CONFIG=%~2"
    shift /1
) else (
    echo [ERROR] Unexpected argument: %~1
    set "RC=2" & goto Finish
)
shift /1
goto ParseArgs

:AfterArgs
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"

if defined ONLY_CONFIG if "!ONLY_CONFIG:~0,2!"=="--" (
    echo [ERROR] --config requires a configuration name, got: !ONLY_CONFIG!
    set "RC=2" & goto Finish
)

set "PREMAKE=%ROOT%SetupScripts\Premake\premake5.exe"
set "SOLUTION=%ROOT%Tests\EngineTests.sln"

REM --- Resolve the suites and configurations for this module and mode --------
set "TARGETS="
set "CONFIGS=Debug Development Release"
set "KIND=test"

if /i "%MODE%"=="tests" (
    set "LOG=%ROOT%TestResults_%MODULE%.log"
    if /i "%MODULE%"=="Core" set "TARGETS=Core-Tests Core-Containers-Tests Core-Templates-Tests Core-Math-Tests-Scalar Core-Math-Tests-SSE Core-Math-Tests-SSE2 Core-Math-Tests-SSE3 Core-Math-Tests-SSSE3 Core-Math-Tests-SSE4_1 Core-Math-Tests-SSE4_2"
    if /i "%MODULE%"=="RHI" set "TARGETS=RHI-Tests"
) else if /i "%MODE%"=="benchmarks" (
    set "KIND=benchmark"
    set "CONFIGS=Development Release"
    set "LOG=%ROOT%BenchmarkResults_%MODULE%.log"
    if /i "%MODULE%"=="Core" set "TARGETS=Core-Benchmarks"
) else (
    echo [ERROR] Unknown mode '%MODE%'. Expected 'tests' or 'benchmarks'.
    set "RC=2" & goto Finish
)

if not defined TARGETS (
    echo [ERROR] No %MODE% are registered for module '%MODULE%'.
    echo         Add it to the dispatch block in %SCRIPT_NAME%.
    set "RC=2" & goto Finish
)

REM  MSBuild takes the projects to build as a semicolon-separated list.
set "MSBUILD_TARGETS=!TARGETS: =;!"

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
set "BUILD_FAILED=0"

REM --- Start each run from a clean combined log -----------------------------
del /q "%LOG%" 2>nul

REM --- Build and run every suite under each configuration -------------------
for %%C in (%CONFIGS%) do (
    set "SKIP=0"
    if defined ONLY_CONFIG if /i not "%%C"=="!ONLY_CONFIG!" set "SKIP=1"
    if "!SKIP!"=="0" if "!BUILD_FAILED!"=="0" call :RunConfig "%%C"
)

if "%BUILD_FAILED%"=="1" (
    set "RC=1" & goto Finish
)

if %TOTAL% equ 0 (
    echo.
    echo [ERROR] Nothing was run. Check --config against the configuration names.
    set "RC=1" & goto Finish
)

echo.
echo ------------------------------------------------------------
echo  %MODULE% %KIND% summary: %TOTAL% ^(config x suite^) run, %FAILED% failed.
echo  Full output written to: %LOG%
echo ------------------------------------------------------------
if %FAILED% gtr 0 (
    echo One or more %MODULE% %KIND% suites FAILED.
    set "RC=1" & goto Finish
)

echo All %MODULE% %KIND% suites PASSED.
set "RC=0" & goto Finish

REM --- Pause (when interactive) and exit with the saved result code ---------
:Finish
if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

REM --- Builds this module's suites in one configuration and runs them -------
REM   %1 = configuration name
:RunConfig
set "CONFIG=%~1"

echo.
echo ------------------------------------------------------------
echo  Building %MODULE% %KIND%s ^(%CONFIG% ^| x64^)...
echo ------------------------------------------------------------

REM  Only this module's projects are built, so running one module does not pay
REM  for the rest of the solution.
"%MSBUILD%" "%SOLUTION%" /t:%MSBUILD_TARGETS% /p:Configuration=%CONFIG% /p:Platform=x64 /m /nologo /verbosity:minimal
if errorlevel 1 (
    echo [ERROR] Build failed for configuration %CONFIG%.
    set "BUILD_FAILED=1"
    goto :eof
)

set "BINDIR=%ROOT%Tests\Build\bin\%CONFIG%-windows-x64"

echo ----- MODULE: %MODULE% ^| CONFIG: %CONFIG% ----->> "%LOG%"

REM  Run the suites from the repo root so each exe appends to the same log
REM  beside it (the harness opens the log with a relative path).
pushd "%ROOT%"
for %%T in (%TARGETS%) do call :RunSuite "%%T" "%CONFIG%"
popd

goto :eof

REM --- Runs a single suite executable and tallies the result ---------------
REM   %1 = suite name, %2 = configuration label (for reporting)
:RunSuite
set "NAME=%~1"
set "CONFIG=%~2"
set "EXE=%BINDIR%\%NAME%\%NAME%.exe"
set /a TOTAL+=1

echo.
echo ------------------------------------------------------------
echo  Running %NAME% ^(%CONFIG%^)...
echo ------------------------------------------------------------
if not exist "%EXE%" (
    echo [ERROR] Executable not found: %EXE%
    set /a FAILED+=1
    goto :eof
)

"%EXE%"
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
