#!/usr/bin/env bash
set -euo pipefail

echo "Updating submodules..."

# If the repo has no .gitmodules, there is nothing to do
if [[ ! -f ".gitmodules" ]]; then
  echo "No .gitmodules file found. Skipping submodule update."
  exit 0
fi

git submodule sync --recursive

# Update/init with progress (retry once)
if ! git submodule update --init --recursive --progress; then
  echo "[WARN] git submodule update failed. Retrying once..."
  git submodule update --init --recursive --progress
fi

echo "Finished updating submodules."
exit 0
