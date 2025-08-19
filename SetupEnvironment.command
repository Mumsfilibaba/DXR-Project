#!/usr/bin/env bash
set -euo pipefail

# Always run from the folder this script lives in (repo root)
cd "$(dirname "$0")"

# --- Sanity checks ---
if ! command -v git >/dev/null 2>&1; then
  echo "[ERROR] Git is not installed or not in PATH."
  exit 1
fi

if [[ -x "SetupScripts/UpdateDependencies.command" ]]; then
  "SetupScripts/UpdateDependencies.command"
elif [[ -x "SetupScripts/UpdateDependencies.sh" ]]; then
  bash "SetupScripts/UpdateDependencies.sh"
elif [[ -f "SetupScripts/UpdateDependencies.bat" ]]; then
  echo "[ERROR] Found Windows-only script: SetupScripts/UpdateDependencies.bat"
  echo "Create a macOS version (UpdateDependencies.sh or .command) and re-run."
  exit 1
else
  echo "[ERROR] Missing SetupScripts/UpdateDependencies.(command|sh)"
  exit 1
fi

# --- Initialize Git LFS and pull large files (root + submodules) ---
if ! git lfs version >/dev/null 2>&1; then
  echo "[ERROR] Git LFS is not installed. Install it (e.g., 'brew install git-lfs') and re-run."
  exit 1
fi

# Ensure smudge isn't skipped by a user/global setting
unset GIT_LFS_SKIP_SMUDGE || true

echo "Setting up git lfs"
git lfs install --local
git lfs pull

# Do the same for all submodules
git submodule foreach --recursive 'git lfs install --local && git lfs pull'

echo "Finished setting up git lfs"
echo
echo "[OK] Environment set up. Dependencies and LFS content are up to date."