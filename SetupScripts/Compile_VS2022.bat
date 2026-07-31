@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio solution and then compiles a project from the
REM  command line. The Generate_* scripts only produce project files; this one
REM  builds.
REM
REM    Compile_VS2022.bat [project] [configuration] [--no-pause] [--skip-generate]
REM
REM  Defaults to the SandboxStandalone executable in the "Development Editor"
REM  configuration. SandboxStandalone is the startup executable; the bare
REM  Sandbox project builds only the game module.
REM
REM  Returns a non-zero exit code if the build fails, so it can be used in
REM  automation. The window pauses at the end when interactive; pass --no-pause
REM  or set TESTS_NO_PAUSE=1 to skip that.
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion
pushd "%~dp0"

set "ROOT=%~dp0.."
set "SOLUTION=%ROOT%\Solutions\DXR-Engine Sandbox.sln"

set "PROJECT=SandboxStandalone"
set "CONFIG=Development Editor"
set "NO_PAUSE=0"
set "SKIP_GENERATE=0"
set "POSITIONAL=0"
set "RC=0"

REM --- Parse arguments -------------------------------------------------------
:ParseArgs
if "%~1"=="" goto AfterArgs
if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else if /i "%~1"=="--skip-generate" (
    set "SKIP_GENERATE=1"
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

REM --- Generate the solution -------------------------------------------------
if "%SKIP_GENERATE%"=="0" (
    echo ------------------------------------------------------------
    echo  Generating Visual Studio solution...
    echo ------------------------------------------------------------
    .\Premake\premake5.exe vs2022 --file=../build.lua --platform=Windows
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
echo.
echo ------------------------------------------------------------
echo  Building %PROJECT% ^(%CONFIG% ^| x64^)...
echo ------------------------------------------------------------

"%MSBUILD%" "%SOLUTION%" /t:"%PROJECT%" /p:Configuration="%CONFIG%" /p:Platform=x64 /m /nologo /verbosity:minimal
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
