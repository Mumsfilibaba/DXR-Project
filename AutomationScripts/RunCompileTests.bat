@echo off
REM ----------------------------------------------------------------------------
REM  Compiles every engine configuration and reports an aggregated pass/fail
REM  result. Returns a non-zero exit code if any configuration failed so it can
REM  be used in automation. Windows counterpart to RunCompileTests.command.
REM
REM  Warnings are errors here (engine modules only; thirdparty modules set
REM  bSilenceWarnings and stay exempt), so this catches the warnings that only
REM  appear in configurations nobody builds day to day.
REM
REM  Everything runs twice, because "Monolithic" is only a configuration *name*
REM  in the generated solution. What actually links the modules statically is
REM  the --monolithic flag passed to premake at generation time, so the two
REM  layouts need two separate generations:
REM
REM    Pass 1  modular     -> Solutions\CompileTest      + Build\bin\*-CompileTest
REM    Pass 2  monolithic  -> Solutions\CompileTestMono  + Build\bin\*-CompileTestMono
REM
REM  Both live beside the solution you work in rather than replacing it, so this
REM  is safe to run with Visual Studio open. The flip side is that the first run
REM  is a cold build; later runs are incremental.
REM
REM  Usage:
REM    RunCompileTests.bat [options]
REM
REM  Options:
REM    --no-pause          Never wait for a keypress before closing.
REM    --clean             Delete the isolated solutions and binaries first.
REM    --modular-only      Skip the monolithic pass.
REM    --monolithic-only   Skip the modular pass.
REM    --config <name>     Build only the named configuration, e.g. "Release".
REM
REM  The window pauses at the end (on success or failure) so results stay
REM  readable when launched interactively. For automation, pass --no-pause or
REM  set TESTS_NO_PAUSE=1 to skip the pause.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

REM  This script sits one level below the repo root; resolve it to a full path
REM  so logged paths do not carry a "..\" through every message.
for %%I in ("%~dp0..") do set "ROOT=%%~fI\"
set "LOG=%ROOT%CompileResults.log"
set "COMPILE=%ROOT%SetupScripts\Compile_VS2022.bat"

set "NO_PAUSE=0"
set "CLEAN=0"
set "RUN_MODULAR=1"
set "RUN_MONOLITHIC=1"
set "ONLY_CONFIG="
set "RC=0"

REM --- Parse arguments -------------------------------------------------------
:ParseArgs
if "%~1"=="" goto AfterArgs
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--clean" (
    set "CLEAN=1"
) else if /i "%~1"=="--modular-only" (
    set "RUN_MONOLITHIC=0"
) else if /i "%~1"=="--monolithic-only" (
    set "RUN_MODULAR=0"
) else if /i "%~1"=="--config" (
    if "%~2"=="" (
        echo [ERROR] --config requires a configuration name.
        set "RC=2" & goto Finish
    )
    set "ONLY_CONFIG=%~2"
    shift
) else (
    echo [ERROR] Unexpected argument: %~1
    set "RC=2" & goto Finish
)
shift
goto ParseArgs

:AfterArgs
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"

if defined ONLY_CONFIG if "!ONLY_CONFIG:~0,2!"=="--" (
    echo [ERROR] --config requires a configuration name, got: !ONLY_CONFIG!
    set "RC=2" & goto Finish
)

if not exist "%COMPILE%" (
    echo [ERROR] Compile script not found: %COMPILE%
    set "RC=1" & goto Finish
)

set /a TOTAL=0
set /a FAILED=0

REM --- Optionally start from scratch ----------------------------------------
if "%CLEAN%"=="1" (
    echo ------------------------------------------------------------
    echo  Removing previous compile-test output...
    echo ------------------------------------------------------------
    call :CleanFolder "%ROOT%Solutions"
    call :CleanFolder "%ROOT%Build\bin"
    call :CleanFolder "%ROOT%Build\bin-int"
)

REM --- Start each run from a clean combined log -----------------------------
del /q "%LOG%" 2>nul

if "%RUN_MODULAR%"=="1"    call :RunPass "Modular"    "CompileTest"     ""
if "%RUN_MONOLITHIC%"=="1" call :RunPass "Monolithic" "CompileTestMono" "--monolithic"

if !TOTAL! equ 0 (
    echo.
    echo [ERROR] Nothing was built. Check --config against the configuration names.
    set "RC=1" & goto Finish
)

echo.
echo ------------------------------------------------------------
echo  Compile summary: !TOTAL! ^(pass x config^) built, !FAILED! failed.
echo  Full output written to: %LOG%
echo ------------------------------------------------------------
echo Compile summary: !TOTAL! built, !FAILED! failed.>> "%LOG%"

if !FAILED! gtr 0 (
    echo One or more configurations FAILED to compile.
    set "RC=1" & goto Finish
)

echo All configurations compiled successfully.
set "RC=0" & goto Finish

REM --- Pause (when interactive) and exit with the saved result code ---------
:Finish
if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

REM --- Deletes every *CompileTest* folder directly inside the given folder --
REM   %1 = parent folder
:CleanFolder
if not exist "%~1" goto :eof
pushd "%~1"
for /d %%D in (*CompileTest*) do (
    echo   Deleting %~1\%%D
    rmdir /s /q "%%D"
)
popd
goto :eof

REM --- Generates one layout and builds every configuration in it -----------
REM   %1 = pass label, %2 = build suffix, %3 = extra premake arguments
:RunPass
set "PASS_NAME=%~1"
set "PASS_SUFFIX=%~2"
set "PASS_EXTRA=%~3"

REM  Only the first configuration regenerates; the other eight reuse the result.
set "PASS_GENERATED=0"

echo.
echo ============================================================
echo  Pass: %PASS_NAME% ^(Solutions\%PASS_SUFFIX%^)
echo ============================================================

for %%C in (
    "Debug" "Development" "Release"
    "Debug Monolithic" "Development Monolithic" "Release Monolithic"
    "Debug Editor" "Development Editor" "Release Editor"
) do (
    if defined ONLY_CONFIG (
        if /i "%%~C"=="!ONLY_CONFIG!" call :BuildConfig "%%~C"
    ) else (
        call :BuildConfig "%%~C"
    )
)

goto :eof

REM --- Builds a single configuration and tallies the result ----------------
REM   %1 = configuration name
:BuildConfig
set "CFG_NAME=%~1"

set "CFG_GENERATE=--skip-generate"
if "!PASS_GENERATED!"=="0" set "CFG_GENERATE="

set /a TOTAL+=1
call :Now CFG_START

echo.
echo ------------------------------------------------------------
echo  Compiling %CFG_NAME% ^| x64 ^(%PASS_NAME%^)...
echo ------------------------------------------------------------
echo ----- PASS: %PASS_NAME% ^| CONFIG: %CFG_NAME% ----->> "%LOG%"

call "%COMPILE%" all "%CFG_NAME%" --suffix "%PASS_SUFFIX%" --log "%LOG%" --fatal-warnings %PASS_EXTRA% %CFG_GENERATE% --no-pause
set "CFG_RESULT=!errorlevel!"

REM  Even a failed build leaves usable project files, so never regenerate twice.
set "PASS_GENERATED=1"

call :Now CFG_END
set /a CFG_ELAPSED=CFG_END-CFG_START
if !CFG_ELAPSED! lss 0 set /a CFG_ELAPSED+=8640000
set /a CFG_MINS=CFG_ELAPSED/6000
set /a CFG_SECS=CFG_ELAPSED/100-CFG_MINS*60

if !CFG_RESULT! neq 0 (
    echo [RESULT] %CFG_NAME% ^(%PASS_NAME%^) FAILED ^(exit code !CFG_RESULT!, !CFG_MINS!m !CFG_SECS!s^)
    set /a FAILED+=1
) else (
    echo [RESULT] %CFG_NAME% ^(%PASS_NAME%^) PASSED ^(!CFG_MINS!m !CFG_SECS!s^)
)

goto :eof

REM --- Stores the current time as hundredths of a second since midnight ----
REM   %1 = name of the variable to write
:Now
setlocal
set "T=%TIME: =0%"
set /a "V=(((1%T:~0,2%-100)*60+(1%T:~3,2%-100))*60+(1%T:~6,2%-100))*100+(1%T:~9,2%-100)"
endlocal & set "%~1=%V%"
goto :eof
