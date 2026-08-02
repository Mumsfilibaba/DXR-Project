@echo off
REM  Builds and runs every RHI test suite. See Scripts\RunSuites.bat for options.
call "%~dp0Scripts\RunSuites.bat" RHI tests %*
exit /b %errorlevel%
