@echo off
REM ----------------------------------------------------------------------------
REM  Shared body of the Compile_VS*.bat scripts. Not meant to be run directly.
REM
REM  Everything that differs between Visual Studio versions arrives as an
REM  environment variable from the wrapper, so the argument parsing and the build
REM  itself are written once:
REM
REM    VS_ACTION         Premake action, for example vs2022 or vs2026.
REM    VS_SOLUTION_EXT   Solution extension the action writes, sln or slnx.
REM    VS_VERSION_RANGE  vswhere version range pinning the IDE, "[17.0,18.0)".
REM    VS_PRERELEASE     1 to let vswhere consider prerelease installs.
REM    VS_DISPLAY_NAME   What to call the IDE in messages.
REM
REM  The version range matters: vswhere -latest alone would hand a VS2022 build
REM  over to whichever Visual Studio is newest, and the generated projects ask for
REM  a specific toolset, so each wrapper pins the IDE it generated for.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

REM  This script lives in SetupScripts\Scripts, so premake and the solution are
REM  found relative to its parent rather than the caller's working directory.
pushd "%~dp0.."
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"

if not defined VS_ACTION        set "VS_ACTION=vs2022"
if not defined VS_SOLUTION_EXT  set "VS_SOLUTION_EXT=sln"
if not defined VS_DISPLAY_NAME  set "VS_DISPLAY_NAME=Visual Studio"

set "PROJECT=SandboxStandalone"
set "CONFIG=Development Editor"
set "SUFFIX="
set "LOGFILE="
set "NO_PAUSE=0"
set "SKIP_GENERATE=0"
set "FATAL_WARNINGS=0"
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
    echo [ERROR] --monolithic is no longer a generation flag. Build one of the
    echo         "* Monolithic" configurations instead.
    set "RC=2" & goto Finish
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
set "SOLUTION=%ROOT%\Solutions\DXR-Engine Sandbox.%VS_SOLUTION_EXT%"
if defined SUFFIX set "SOLUTION=%ROOT%\Solutions\%SUFFIX%\DXR-Engine Sandbox.%VS_SOLUTION_EXT%"

REM  Generating rewrites the .vcxproj files for this action's toolset, which
REM  leaves the other format's solution pointing at projects it can no longer
REM  open. The Generate_* scripts drop it for the same reason.
set "STALE_EXT=slnx"
if /i "%VS_SOLUTION_EXT%"=="slnx" set "STALE_EXT=sln"

set "STALE_SOLUTION=%ROOT%\Solutions\DXR-Engine Sandbox.%STALE_EXT%"
if defined SUFFIX set "STALE_SOLUTION=%ROOT%\Solutions\%SUFFIX%\DXR-Engine Sandbox.%STALE_EXT%"

set "PREMAKE_ARGS=%VS_ACTION% --file=../build.lua --platform=Windows"
if defined SUFFIX set "PREMAKE_ARGS=%PREMAKE_ARGS% --buildsuffix=%SUFFIX%"
if "%FATAL_WARNINGS%"=="1" set "PREMAKE_ARGS=%PREMAKE_ARGS% --fatalwarnings"

REM --- Generate the solution -------------------------------------------------
if "%SKIP_GENERATE%"=="0" (
    echo ------------------------------------------------------------
    echo  Generating %VS_DISPLAY_NAME% solution...
    echo ------------------------------------------------------------
    .\Premake\premake5.exe %PREMAKE_ARGS%
    if errorlevel 1 (
        echo [ERROR] Failed to generate the solution.
        set "RC=1" & goto Finish
    )

    if exist "!STALE_SOLUTION!" del /q "!STALE_SOLUTION!"
)

if not exist "%SOLUTION%" (
    echo [ERROR] Solution not found: %SOLUTION%
    set "RC=1" & goto Finish
)

REM --- Locate MSBuild through vswhere ---------------------------------------
REM  vswhere ships beside the Visual Studio installer under the 32-bit program
REM  files root. ProgramFiles(x86) is the documented way to name it, but that
REM  variable is missing from some environments, so the 64-bit root and the
REM  literal default are tried after it. These are checked through a subroutine
REM  rather than a for loop, because the ")" inside "ProgramFiles(x86)" would
REM  close a parenthesised block early.
set "VSWHERE="
call :UseVsWhereIfPresent "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
call :UseVsWhereIfPresent "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
call :UseVsWhereIfPresent "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

if not defined VSWHERE (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    set "RC=1" & goto Finish
)

REM  Pinned to the IDE this generation targeted, so a newer Visual Studio sitting
REM  beside it cannot pick up projects built for an older toolset.
set "VSWHERE_ARGS=-requires Microsoft.Component.MSBuild"
if defined VS_VERSION_RANGE set "VSWHERE_ARGS=%VSWHERE_ARGS% -version "%VS_VERSION_RANGE%""
if "%VS_PRERELEASE%"=="1"   set "VSWHERE_ARGS=%VSWHERE_ARGS% -prerelease"

REM  The result comes back through a file rather than a piped for /f, because the
REM  ")" that ends a version range like "[18.0,19.0)" would close the for's own
REM  parenthesis and leave nothing behind.
set "VSWHERE_OUT=%TEMP%\dxr-vswhere-msbuild.txt"
"%VSWHERE%" %VSWHERE_ARGS% -find MSBuild\**\Bin\MSBuild.exe > "%VSWHERE_OUT%" 2>nul

set "MSBUILD="
if exist "%VSWHERE_OUT%" for /f "usebackq delims=" %%i in ("%VSWHERE_OUT%") do set "MSBUILD=%%i"
del /q "%VSWHERE_OUT%" 2>nul

if not defined MSBUILD (
    echo [ERROR] Could not locate MSBuild.exe for %VS_DISPLAY_NAME%.
    echo         Install it, or use the Compile script for the version you have.
    set "RC=1" & goto Finish
)

REM --- Build -----------------------------------------------------------------
REM  Without /t: MSBuild builds every project in the solution.
REM  AutomationScripts\RunCompileTests.bat relies on that, since ImGuiPlugin is loaded
REM  at runtime rather than linked and so is nothing else's dependency.
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

REM  Takes the first candidate path that exists, so the calls above read as a
REM  list of places to look in order of preference.
:UseVsWhereIfPresent
if defined VSWHERE goto :eof
if exist "%~1" set "VSWHERE=%~1"
goto :eof
