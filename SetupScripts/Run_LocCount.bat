@echo off
REM ----------------------------------------------------------------------------
REM  Generates the tools workspace, builds LocCount, and launches it.
REM
REM    Run_LocCount.bat [options] [-- loccount-args]
REM
REM  Options:
REM    --config <name>   Build configuration. Defaults to Development.
REM    --build-only      Build and stop, without launching.
REM    --no-pause        Never wait for a keypress before closing.
REM
REM  Anything after -- is passed through to LocCount:
REM    Run_LocCount.bat -- --root=C:\develop\Github\DXR-Project
REM ----------------------------------------------------------------------------
setlocal EnableDelayedExpansion

pushd "%~dp0"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"

set "CONFIG=Development"
set "BUILD_ONLY=0"
set "NO_PAUSE=0"
set "PASSTHROUGH="
set "EXPECT_VALUE="
set "RC=0"

:ParseArgs
if "%~1"=="" goto AfterArgs
if /i "%~1"=="--" (
    shift
    :CollectPassthrough
    if "%~1"=="" goto AfterArgs
    if defined PASSTHROUGH (
        set "PASSTHROUGH=!PASSTHROUGH! %1"
    ) else (
        set "PASSTHROUGH=%1"
    )
    shift
    goto CollectPassthrough
)
if defined EXPECT_VALUE (
    if /i "!EXPECT_VALUE!"=="--config" set "CONFIG=%~1"
    set "EXPECT_VALUE="
    shift
    goto ParseArgs
)
if /i "%~1"=="--config" (
    set "EXPECT_VALUE=--config"
) else if /i "%~1"=="--build-only" (
    set "BUILD_ONLY=1"
) else if /i "%~1"=="--no-pause" (
    set "NO_PAUSE=1"
) else (
    echo [ERROR] Unexpected argument: %~1
    set "RC=2" & goto Finish
)
shift
goto ParseArgs

:AfterArgs
if defined EXPECT_VALUE (
    echo [ERROR] !EXPECT_VALUE! requires a value.
    set "RC=2" & goto Finish
)
if defined TESTS_NO_PAUSE set "NO_PAUSE=1"

set "SOLUTION=%ROOT%\Solutions\Tools\DXR-Engine Tools.slnx"
set "STALE_SOLUTION=%ROOT%\Solutions\Tools\DXR-Engine Tools.sln"
set "EXE=%ROOT%\Build\bin\%CONFIG%-windows-x64-Monolithic-Tools\LocCount.exe"

echo ------------------------------------------------------------
echo  Generating tools solution...
echo ------------------------------------------------------------
.\Premake\premake5.exe vs2026 --file=../Tools/build.lua --platform=Windows --monolithic --buildsuffix=Tools
if errorlevel 1 (
    echo [ERROR] Failed to generate the tools solution.
    set "RC=1" & goto Finish
)
if exist "%STALE_SOLUTION%" del /q "%STALE_SOLUTION%"

if not exist "%SOLUTION%" (
    echo [ERROR] Solution not found: %SOLUTION%
    set "RC=1" & goto Finish
)

set "VSWHERE="
call :UseVsWhereIfPresent "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
call :UseVsWhereIfPresent "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
call :UseVsWhereIfPresent "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

if not defined VSWHERE (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    set "RC=1" & goto Finish
)

set "VSWHERE_OUT=%TEMP%\dxr-vswhere-msbuild.txt"
"%VSWHERE%" -requires Microsoft.Component.MSBuild -version "[18.0,19.0)" -prerelease -find MSBuild\**\Bin\MSBuild.exe > "%VSWHERE_OUT%" 2>nul

set "MSBUILD="
if exist "%VSWHERE_OUT%" for /f "usebackq delims=" %%i in ("%VSWHERE_OUT%") do set "MSBUILD=%%i"
del /q "%VSWHERE_OUT%" 2>nul

if not defined MSBUILD (
    echo [ERROR] Could not locate MSBuild.exe for Visual Studio 2026.
    set "RC=1" & goto Finish
)

echo.
echo ------------------------------------------------------------
echo  Building LocCount ^(%CONFIG% ^| x64^)...
echo ------------------------------------------------------------

"%MSBUILD%" "%SOLUTION%" /t:"LocCount" /p:Configuration="%CONFIG%" /p:Platform=x64 /m /nologo /verbosity:minimal
if errorlevel 1 (
    echo.
    echo Build FAILED.
    set "RC=1" & goto Finish
)

echo.
echo Build SUCCEEDED.

if "%BUILD_ONLY%"=="1" (
    echo Built: %EXE%
    goto Finish
)

if not exist "%EXE%" (
    echo [ERROR] Executable not found: %EXE%
    set "RC=1" & goto Finish
)

echo.
echo ------------------------------------------------------------
echo  Launching LocCount...
echo ------------------------------------------------------------
pushd "%ROOT%"
"%EXE%" %PASSTHROUGH%
set "RC=%ERRORLEVEL%"
popd
echo.
echo [RESULT] LocCount exited with code %RC%

:Finish
popd
if "%NO_PAUSE%"=="0" (
    echo.
    pause
)
exit /b %RC%

:UseVsWhereIfPresent
if defined VSWHERE goto :eof
if exist "%~1" set "VSWHERE=%~1"
goto :eof
