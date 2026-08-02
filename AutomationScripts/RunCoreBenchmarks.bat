@echo off
REM  Builds and runs every Core benchmark. Development and Release only; see
REM  Scripts\RunSuites.bat for options.
call "%~dp0Scripts\RunSuites.bat" Core benchmarks %*
exit /b %errorlevel%
