@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio 2026 solution for the tools, into Solutions\Tools
REM  so it sits beside the engine solution rather than replacing it. The tools
REM  workspace has to be monolithic.
REM
REM  The .sln from an earlier VS2022 run is deleted, because both generations
REM  share the same .vcxproj files and ask for different toolsets.
REM ----------------------------------------------------------------------------
pushd "%~dp0"

.\Premake\premake5.exe vs2026 --file=../Tools/build.lua --platform=Windows --monolithic --buildsuffix=Tools
if errorlevel 1 (
    echo [ERROR] Failed to generate the tools solution.
    popd
    pause
    exit /b 1
)

if exist "..\Solutions\Tools\DXR-Engine Tools.sln" del /q "..\Solutions\Tools\DXR-Engine Tools.sln"

popd
pause
