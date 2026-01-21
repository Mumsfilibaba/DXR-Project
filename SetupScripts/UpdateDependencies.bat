@echo off
setlocal EnableExtensions

echo Updating submodules...

REM If the repo has no .gitmodules, there is nothing to do
if not exist ".gitmodules" (
  echo No .gitmodules file found. Skipping submodule update.
  exit /b 0
)

REM Sync URLs in case .gitmodules changed
git submodule sync --recursive
if errorlevel 1 (
  echo [ERROR] git submodule sync failed.
  exit /b 1
)

REM Init/update (with progress)
git submodule update --init --recursive --progress
if errorlevel 1 (
  echo [WARN] git submodule update failed. Retrying once...
  git submodule update --init --recursive --progress
  if errorlevel 1 (
    echo [ERROR] git submodule update failed again.
    exit /b 1
  )
)

echo Finished updating submodules.
exit /b 0
