@echo off
REM  Builds and runs every Core test suite. See Scripts\RunSuites.bat for options.
call "%~dp0Scripts\RunSuites.bat" Core tests %*
exit /b %errorlevel%
