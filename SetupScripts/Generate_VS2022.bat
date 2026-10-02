@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio 2022 solution for the engine and sandbox.
REM
REM  The .slnx from an earlier VS2026 run is deleted. Both generations write the
REM  same .vcxproj files into the same folder but ask for different toolsets, v143
REM  here against v145 for VS2026, so a solution left over from the other one
REM  would open projects demanding a toolset its IDE does not have.
REM ----------------------------------------------------------------------------
pushd "%~dp0"

.\Premake\premake5.exe vs2022 --file=../build.lua --platform=Windows
if errorlevel 1 (
    echo [ERROR] Failed to generate the solution.
    popd
    pause
    exit /b 1
)

if exist "..\Solutions\DXR-Engine Sandbox.slnx" del /q "..\Solutions\DXR-Engine Sandbox.slnx"

popd
pause
