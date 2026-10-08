@echo off
REM  Builds and runs every ShaderCompiler test suite. See Scripts\RunSuites.bat for options.
call "%~dp0Scripts\RunSuites.bat" ShaderCompiler tests %*
exit /b %errorlevel%
