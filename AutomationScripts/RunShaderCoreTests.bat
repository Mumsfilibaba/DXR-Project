@echo off
REM  Builds and runs every ShaderCore test suite. See Scripts\RunSuites.bat for options.
call "%~dp0Scripts\RunSuites.bat" ShaderCore tests %*
exit /b %errorlevel%
