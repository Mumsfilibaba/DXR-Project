@echo off
REM ----------------------------------------------------------------------------
REM  Generates the Visual Studio 2022 solution and then compiles a project from
REM  the command line. The Generate_* scripts only produce project files; this one
REM  builds.
REM
REM    Compile_VS2022.bat [project] [configuration] [options]
REM
REM  Defaults to the SandboxStandalone executable in the "Development Editor"
REM  configuration. SandboxStandalone is the startup executable; the bare
REM  Sandbox project builds only the game module. Pass "all" as the project to
REM  build every project in the solution instead.
REM
REM  Options:
REM    --no-pause        Never wait for a keypress before closing.
REM    --skip-generate   Build the existing solution without regenerating it.
REM    --fatal-warnings  Treat compiler warnings as errors (engine modules only).
REM    --suffix <name>   Generate into Solutions\<name> and write binaries to
REM                      Build\bin\<config>-windows-x64-<name>, so a build can
REM                      run without disturbing the normal solution.
REM    --log <path>      Append the build transcript to <path> as well, while
REM                      still printing it to the console.
REM
REM  Static linking is a configuration here rather than an option: build one of
REM  the "* Monolithic" configurations instead of passing a flag at generation
REM  time. One solution carries both layouts.
REM
REM  Returns a non-zero exit code if the build fails, so it can be used in
REM  automation. The window pauses at the end when interactive; pass --no-pause
REM  or set TESTS_NO_PAUSE=1 to skip that.
REM
REM  See Compile_VS2026.bat for the Visual Studio 2026 equivalent. The body of
REM  both lives in Scripts\CompileVisualStudio.bat.
REM ----------------------------------------------------------------------------
setlocal

set "VS_ACTION=vs2022"
set "VS_SOLUTION_EXT=sln"
set "VS_VERSION_RANGE=[17.0,18.0)"
set "VS_PRERELEASE=0"
set "VS_DISPLAY_NAME=Visual Studio 2022"

call "%~dp0Scripts\CompileVisualStudio.bat" %*
exit /b %errorlevel%
