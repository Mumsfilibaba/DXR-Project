@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio 2022 solution for the test suites, into
REM  Solutions\Tests so it sits beside the engine solution rather than replacing
REM  it. The test workspace has to be monolithic.
REM
REM  The .slnx from an earlier VS2026 run is deleted, because both generations
REM  share the same .vcxproj files and ask for different toolsets.
REM ----------------------------------------------------------------------------
pushd "%~dp0"

.\Premake\premake5.exe vs2022 --file=../Tests/build.lua --platform=Windows --monolithic --buildsuffix=Tests
if errorlevel 1 (
    echo [ERROR] Failed to generate the test solution.
    popd
    pause
    exit /b 1
)

if exist "..\Solutions\Tests\DXR-Engine Tests.slnx" del /q "..\Solutions\Tests\DXR-Engine Tests.slnx"

popd
pause
