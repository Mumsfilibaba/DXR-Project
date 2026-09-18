@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio 2026 solution for the test suites, into
REM  Solutions\Tests so it sits beside the engine solution rather than replacing
REM  it. The test workspace has to be monolithic.
REM
REM  The .sln from an earlier VS2022 run is deleted, because both generations
REM  share the same .vcxproj files and ask for different toolsets.
REM
REM  Note that AutomationScripts\RunApplicationTests.bat and the other suite
REM  runners generate their own VS2022 solution, so they are unaffected by this.
REM ----------------------------------------------------------------------------
pushd "%~dp0"

.\Premake\premake5.exe vs2026 --file=../Tests/build.lua --platform=Windows --monolithic --buildsuffix=Tests
if errorlevel 1 (
    echo [ERROR] Failed to generate the test solution.
    popd
    pause
    exit /b 1
)

if exist "..\Solutions\Tests\DXR-Engine Tests.sln" del /q "..\Solutions\Tests\DXR-Engine Tests.sln"

popd
pause
