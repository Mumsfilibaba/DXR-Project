@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio 2026 solution for the engine and sandbox.
REM
REM  Premake writes VS2026 solutions in Microsoft's XML .slnx format, so this
REM  leaves "Solutions\DXR-Engine Sandbox.slnx" rather than the .sln that
REM  Generate_VS2022.bat produces.
REM
REM  Both generations write the same .vcxproj files into the same folder but ask
REM  for different toolsets, v145 here against v143 for VS2022. The .sln from an
REM  earlier VS2022 run is therefore deleted: leaving it would let someone open a
REM  solution whose projects now demand a toolset their IDE does not have.
REM ----------------------------------------------------------------------------
pushd "%~dp0"

.\Premake\premake5.exe vs2026 --file=../build.lua --platform=Windows
if errorlevel 1 (
    echo [ERROR] Failed to generate the solution.
    popd
    pause
    exit /b 1
)

if exist "..\Solutions\DXR-Engine Sandbox.sln" del /q "..\Solutions\DXR-Engine Sandbox.sln"

popd
pause
