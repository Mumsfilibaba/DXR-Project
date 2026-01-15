@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM ------------------------------------------------------------------------------------
REM Args:
REM   --keepopen : internal flag that means we are already running in the spawned window
REM   --nopause  : never pause at the end
REM ------------------------------------------------------------------------------------
set "KEEP_OPEN=0"
set "NO_PAUSE=0"

if /I "%~1"=="--keepopen" set "KEEP_OPEN=1"
if /I "%~1"=="--nopause"  set "NO_PAUSE=1"
if /I "%~2"=="--nopause"  set "NO_PAUSE=1"

REM ------------------------------------------------------------------------------------
REM If launched by double-click (no --keepopen), run in a new cmd window.
REM Use cmd /c so the window closes AFTER the script finishes.
REM ------------------------------------------------------------------------------------
if "%KEEP_OPEN%"=="0" (
    start "SetupEnvironment" cmd /c ""%~f0" --keepopen %*"
    exit /b
)

REM Always run from the folder this script lives in (repo root)
pushd "%~dp0" >nul

set "LOGFILE=%CD%\SetupEnvironment_Log.txt"
set "SUBMODULE_COUNT=0"
set "FAILED_RUN=0"
set "START_TIME=%TIME%"

REM ------------------------------------------------------------------------------------
REM Create/clear log file as UTF-8
REM ------------------------------------------------------------------------------------
powershell -NoProfile -Command ^
  "$p='%LOGFILE%'; [System.IO.File]::WriteAllText($p, '', (New-Object System.Text.UTF8Encoding($true)))" >nul 2>&1

call :LogLine "------------------------------------------------------------------------------------"
call :LogLine "Setup script started"
call :LogLine "Repo: %CD%\"
call :LogLine "Current Dir: %CD%"
call :LogLine "LogFile: %LOGFILE%"
call :LogLine "TEMP=%TEMP%"
call :LogLine "TMP=%TMP%"
call :LogLine "------------------------------------------------------------------------------------"
echo.

REM --- Sanity checks ---
echo [INFO] Checking Git...
git --version
if errorlevel 1 (
  echo [ERROR] Git is not installed or not in PATH.
  call :LogLine "[ERROR] Git is not installed or not in PATH."
  goto :Fail
)
call :LogLine "[OK] Git found."

if not exist "SetupScripts\UpdateDependencies.bat" (
  echo [ERROR] Missing SetupScripts\UpdateDependencies.bat
  echo Current folder: "%CD%"
  call :LogLine "[ERROR] Missing SetupScripts\UpdateDependencies.bat"
  call :LogLine "Current folder: %CD%"
  goto :Fail
)

REM --- 1) Update submodules/third-party dependencies ---
echo.
echo [INFO] Updating submodules...
call :LogLine "[INFO] Updating submodules..."

call :RunAndTee "call SetupScripts\UpdateDependencies.bat"
if errorlevel 1 (
  echo.
  echo [ERROR] UpdateDependencies.bat failed. See log: "%LOGFILE%"
  call :LogLine "[ERROR] UpdateDependencies.bat failed."
  goto :Fail
)
call :LogLine "[OK] Submodules updated."

REM --- 2) Git LFS ---
echo.
echo [INFO] Checking Git LFS...
git lfs version
if errorlevel 1 (
  echo.
  echo [ERROR] Git LFS is not installed. Install it and re-run.
  call :LogLine "[ERROR] Git LFS not installed."
  goto :Fail
)
call :LogLine "[OK] Git LFS found."

set GIT_LFS_SKIP_SMUDGE=

echo.
echo [INFO] Updating Git LFS hooks in repo root...
call :LogLine "[INFO] Updating Git LFS hooks in repo root..."

call :RunAndTee "git lfs update --force"
if errorlevel 1 (
  echo.
  echo [ERROR] git lfs update --force failed in repo root.
  call :LogLine "[ERROR] git lfs update --force failed in repo root."
  goto :Fail
)

echo [INFO] Pulling LFS files in repo root...
call :LogLine "[INFO] Pulling LFS files in repo root..."
call :RunAndTee "git lfs pull"
if errorlevel 1 (
  echo.
  echo [ERROR] git lfs pull failed in repo root.
  call :LogLine "[ERROR] git lfs pull failed in repo root."
  goto :Fail
)

REM --- 3) Submodules: LFS update and pull ---
echo.
echo [INFO] Updating Git LFS hooks and pulling in submodules...
call :LogLine "[INFO] Updating Git LFS hooks + pulling in submodules..."

for /f "tokens=2 delims= " %%P in ('git config -f .gitmodules --get-regexp ^submodule\..*\.path$ 2^>nul') do (
  set /a SUBMODULE_COUNT+=1

  echo.
  echo ------------------------------------------------------------------------------------
  echo Submodule: %%P
  echo ------------------------------------------------------------------------------------
  call :LogLine "Submodule: %%P"

  if exist "%%P" (
    pushd "%%P" >nul

    call :RunAndTee "git lfs update --force"
    if errorlevel 1 (
      popd >nul
      echo [ERROR] git lfs update --force failed in %%P
      call :LogLine "[ERROR] git lfs update --force failed in %%P"
      goto :Fail
    )

    call :RunAndTee "git lfs pull"
    if errorlevel 1 (
      popd >nul
      echo [ERROR] git lfs pull failed in %%P
      call :LogLine "[ERROR] git lfs pull failed in %%P"
      goto :Fail
    )

    popd >nul
  ) else (
    echo [WARN] Submodule folder missing: %%P
    call :LogLine "[WARN] Submodule folder missing: %%P"
  )
)

call :PrintSummary 0
goto :End


:Fail
set "FAILED_RUN=1"
call :PrintSummary 1

:End
echo.

if "%NO_PAUSE%"=="0" (
  echo Press any key to close...
  pause >nul
)

popd >nul
endlocal
exit /b %FAILED_RUN%


REM ------------------------------------------------------------------------------------
REM Helper: log a single line to file
REM ------------------------------------------------------------------------------------
:LogLine
set "TS=%DATE% %TIME:~0,8%"
>> "%LOGFILE%" echo [%TS%] %~1
exit /b 0


REM ------------------------------------------------------------------------------------
REM Helper: Run a command, show output in console, and append to log
REM ------------------------------------------------------------------------------------
:RunAndTee
set "CMD=%~1"
set "TMPFILE=%CD%\.__setup_capture_%RANDOM%_%RANDOM%.log"

cmd /c %CMD% > "%TMPFILE%" 2>&1
set "RC=%errorlevel%"

if not exist "%TMPFILE%" (
  echo [ERROR] Could not create capture file: "%TMPFILE%"
  call :LogLine "[ERROR] Could not create capture file: %TMPFILE%"
  call :LogLine "[ERROR] Command was: %CMD%"
  exit /b 1
)

type "%TMPFILE%"
type "%TMPFILE%" >> "%LOGFILE%"

del /f /q "%TMPFILE%" >nul 2>&1

exit /b %RC%

REM ------------------------------------------------------------------------------------
REM Helper: Print summary (success/failure) + timing
REM ------------------------------------------------------------------------------------
:PrintSummary
set "FAILED=%~1"
set "END_TIME=%TIME%"

echo.
echo ------------------------------------------------------------------------------------
if "%FAILED%"=="0" (
  echo SUCCESS: Environment setup complete.
  call :LogLine "SUCCESS: Environment setup complete."
) else (
  echo FAILED: Setup script encountered an error.
  call :LogLine "FAILED: Setup script encountered an error."
)
echo Log file: "%LOGFILE%"
echo Submodules processed: %SUBMODULE_COUNT%
echo Started: %START_TIME%
echo Ended  : %END_TIME%
echo ------------------------------------------------------------------------------------

call :LogLine "Log file: %LOGFILE%"
call :LogLine "Submodules processed: %SUBMODULE_COUNT%"
call :LogLine "Started: %START_TIME%"
call :LogLine "Ended  : %END_TIME%"

exit /b 0
