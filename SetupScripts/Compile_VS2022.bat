@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio solution and then compiles a project from the
REM  command line. The Generate_* scripts only produce project files; this one
REM  builds.
REM
REM    Compile_VS2022.bat [project] [configuration] [options]
REM
REM  Defaults to the SandboxStandalone executable in the "Development Editor"
REM  configuration. SandboxStandalone is the startup executable; the bare
REM  Sandbox project builds only the game module. Pass "all" as the project to
REM  build every project in the solution instead.
REM
REM  Options:
REM    --no-pause        Never wait for a keypress before closing.
REM    --skip-generate   Build the existing solution without regenerating it.
REM    --fatal-warnings  Treat compiler warnings as errors (engine modules only).
REM    --monolithic      Link all modules statically into the executable.
REM    --suffix <name>   Generate into Solutions\<name> and write binaries to
REM                      Build\bin\<config>-windows-x64-<name>, so a build can
REM                      run without disturbing the normal solution.
REM    --log <path>      Append the build transcript to <path> as well, while
REM                      still printing it to the console.
REM
REM  Returns a non-zero exit code if the build fails, so it can be used in
REM  automation. The window pauses at the end when interactive; pass --no-pause
REM  or set TESTS_NO_PAUSE=1 to skip that.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion
pushd "%~dp0"

set "ROOT=%~dp0.."

set "PROJECT=SandboxStandalone"
set "CONFIG=Development Editor"
set "SUFFIX="
set "LOGFILE="
set "NO_PAUSE=0"
set "SKIP_GENERATE=0"
set "FATAL_WARNINGS=0"
set "MONOLITHIC=0"
set "POSITIONAL=0"
set "RC=0"

REM --- Parse arguments -------------------------------------------------------
:ParseArgs
if "%~1"=="" goto AfterArgs
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--skip-generate" (
    set "SKIP_GENERATE=1"
) else if /i "%~1"=="--fatal-warnings" (
    set "FATAL_WARNINGS=1"
) else if /i "%~1"=="--monolithic" (
    set "MONOLITHIC=1"
) else if /i "%~1"=="--suffix" (
    if "%~2"=="" (
        echo [ERROR] --suffix requires a name.
        set "RC=2" & goto Finish
    )
    set "SUFFIX=%~2"
    shift
) else if /i "%~1"=="--log" (
    if "%~2"=="" (
        echo [ERROR] --log requires a path.
        set "RC=2" & goto Finish
    )
    set "LOGFILE=%~2"
    shift
) else if "!POSITIONAL!"=="0" (
    set "PROJECT=%~1"
    set "POSITIONAL=1"
) else if "!POSITIONAL!"=="1" (
    set "CONFIG=%~1"
    set "POSITIONAL=2"
) else (
    echo [ERROR] Unexpected argument: %~1
    set "RC=2" & goto Finish
)
shift
goto ParseArgs

:AfterArgs
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"

REM  Catches "--suffix --no-pause", where the option swallows the following flag
REM  and would otherwise generate into a folder literally named "--no-pause".
if defined SUFFIX if "!SUFFIX:~0,2!"=="--" (
    echo [ERROR] --suffix requires a name, got: !SUFFIX!
    set "RC=2" & goto Finish
)
if defined LOGFILE if "!LOGFILE:~0,2!"=="--" (
    echo [ERROR] --log requires a path, got: !LOGFILE!
    set "RC=2" & goto Finish
)

REM --- A suffix moves the whole generation into its own folder ---------------
set "SOLUTION=%ROOT%\Solutions\DXR-Engine Sandbox.sln"
if defined SUFFIX set "SOLUTION=%ROOT%\Solutions\%SUFFIX%\DXR-Engine Sandbox.sln"

set "PREMAKE_ARGS=vs2022 --file=../build.lua --platform=Windows"
if defined SUFFIX set "PREMAKE_ARGS=%PREMAKE_ARGS% --buildsuffix=%SUFFIX%"
if "%FATAL_WARNINGS%"=="1" set "PREMAKE_ARGS=%PREMAKE_ARGS% --fatalwarnings"
if "%MONOLITHIC%"=="1" set "PREMAKE_ARGS=%PREMAKE_ARGS% --monolithic"

REM --- Generate the solution -------------------------------------------------
if "%SKIP_GENERATE%"=="0" (
    echo ------------------------------------------------------------
    echo  Generating Visual Studio solution...
    echo ------------------------------------------------------------
    .\Premake\premake5.exe %PREMAKE_ARGS%
    if errorlevel 1 (
        echo [ERROR] Failed to generate the solution.
        set "RC=1" & goto Finish
    )
)

if not exist "%SOLUTION%" (
    echo [ERROR] Solution not found: %SOLUTION%
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

REM --- Build -----------------------------------------------------------------
REM  Without /t: MSBuild builds every project in the solution. RunCompileTests.bat
REM  relies on that, since SandboxStandalone is only generated for a non-monolithic
REM  build and ImGuiPlugin is loaded at runtime rather than linked.
set "TARGET_ARG=/t:"%PROJECT%""
set "WHAT=%PROJECT%"
if /i "%PROJECT%"=="all" (
    set "TARGET_ARG="
    set "WHAT=all projects"
)

REM  MSBuild's own file logger keeps the console output live while still capturing
REM  the transcript, which a redirect into a temp file would not.
set "LOG_ARG="
if defined LOGFILE set "LOG_ARG=/flp:LogFile="%LOGFILE%";Append;Verbosity=minimal"

echo.
echo ------------------------------------------------------------
echo  Building !WHAT! ^(%CONFIG% ^| x64^)...
echo ------------------------------------------------------------

"%MSBUILD%" "%SOLUTION%" !TARGET_ARG! /p:Configuration="%CONFIG%" /p:Platform=x64 /m /nologo /verbosity:minimal !LOG_ARG!
if errorlevel 1 (
    echo.
    echo Build FAILED.
    set "RC=1" & goto Finish
)

echo.
echo Build SUCCEEDED.
set "RC=0" & goto Finish

REM --- Pause (when interactive) and exit with the saved result code ---------
:Finish
popd
if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%
