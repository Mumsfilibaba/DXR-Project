@echo off
REM  Builds and runs every Application test suite. See Scripts\RunSuites.bat for options.
call "%~dp0Scripts\RunSuites.bat" Application tests %*
exit /b %errorlevel%
