#!/usr/bin/env bash
set -euo pipefail

# Run from repo root (this script lives in SetupScripts/)
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${DIR}/.."

echo "Updating submodules"

git submodule sync --recursive
git submodule update --init --recursive --remote

echo "Finished updating submodules"
