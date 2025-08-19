@echo off
setlocal

REM Always run from the folder this script lives in (repo root)
pushd "%~dp0" >nul

REM --- Sanity checks ---
git --version >nul 2>&1 || (echo [ERROR] Git is not installed or not in PATH.& exit /b 1)
if not exist "SetupScripts\UpdateDependencies.bat" (
  echo [ERROR] Missing SetupScripts\UpdateDependencies.bat
  exit /b 1
)

REM --- 1) Update submodules/third-party deps ---
call "SetupScripts\UpdateDependencies.bat" || (
  echo [ERROR] UpdateDependencies.bat failed.
  exit /b 1
)

REM --- 2) Initialize Git LFS and pull large files (root + submodules) ---
git lfs version >nul 2>&1 || (
  echo [ERROR] Git LFS is not installed. Install it and re-run.
  exit /b 1
)

REM Ensure smudge isn't skipped by a user/global setting
set GIT_LFS_SKIP_SMUDGE=

@echo Setting up git lfs

git lfs install --local || exit /b 1
git lfs pull || exit /b 1

REM Do the same for all submodules
git submodule foreach --recursive "git lfs install --local && git lfs pull || exit 1" || (
  echo [ERROR] Git LFS pull failed in one or more submodules.
  exit /b 1
)

@echo Finished setting up git lfs

echo.
echo Environment set up. Dependencies and LFS content are up to date.

popd >nul
endlocal

pause
