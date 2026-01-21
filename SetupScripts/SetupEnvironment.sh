#!/usr/bin/env bash
set -euo pipefail

# ------------------------------------------------------------------------------------
# Args:
#   --nopause : do not wait for keypress at the end
# ------------------------------------------------------------------------------------
NO_PAUSE=0
for arg in "$@"; do
  if [[ "$arg" == "--nopause" ]]; then
    NO_PAUSE=1
  fi
done

# Repo root = parent folder of SetupScripts
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_DIR"

LOGFILE="$REPO_DIR/SetupEnvironment_Log.txt"
SUBMODULE_COUNT=0

START_TS="$(date +"%Y-%m-%d %H:%M:%S")"
START_EPOCH="$(date +%s)"

# ------------------------------------------------------------------------------------
# Logging helpers
# ------------------------------------------------------------------------------------
log_line() {
  local ts
  ts="$(date +"%Y-%m-%d %H:%M:%S")"
  echo "[$ts] $*" >> "$LOGFILE"
}

run_and_tee() {
  local cmd="$1"
  log_line "[CMD] $cmd"

  # Tee output, preserve the command exit code
  bash -lc "$cmd" 2>&1 | tee -a "$LOGFILE"
  local rc=${PIPESTATUS[0]}
  return $rc
}

pause_if_needed() {
  if [[ "$NO_PAUSE" -eq 1 ]]; then
    return 0
  fi

  # Only pause if interactive (prevents CI hanging)
  if [[ -t 0 ]]; then
    echo
    echo "Press any key to close..."
    read -n 1 -s || true
  fi
}

# ------------------------------------------------------------------------------------
# Start log fresh
# ------------------------------------------------------------------------------------
: > "$LOGFILE"

log_line "------------------------------------------------------------------------------------"
log_line "Setup script started"
log_line "Repo: $REPO_DIR/"
log_line "Current Dir: $(pwd)"
log_line "LogFile: $LOGFILE"
log_line "------------------------------------------------------------------------------------"

echo
echo "------------------------------------------------------------------------------------"
echo "Setup script started"
echo "Repo: $REPO_DIR/"
echo "Current Dir: $(pwd)"
echo "LogFile: $LOGFILE"
echo "------------------------------------------------------------------------------------"
echo

# ------------------------------------------------------------------------------------
# Sanity checks
# ------------------------------------------------------------------------------------
echo "[INFO] Checking Git..."
if ! command -v git >/dev/null 2>&1; then
  echo "[ERROR] Git is not installed or not in PATH."
  log_line "[ERROR] Git is not installed or not in PATH."
  pause_if_needed
  exit 1
fi

git --version | tee -a "$LOGFILE"
log_line "[OK] Git found."

# ------------------------------------------------------------------------------------
# 1) Update submodules / dependencies
# ------------------------------------------------------------------------------------
echo
echo "[INFO] Updating submodules..."
log_line "[INFO] Updating submodules..."

if [[ -x "$SCRIPT_DIR/UpdateDependencies.sh" ]]; then
  run_and_tee "bash \"$SCRIPT_DIR/UpdateDependencies.sh\""
else
  echo "[ERROR] Missing SetupScripts/UpdateDependencies.sh"
  log_line "[ERROR] Missing SetupScripts/UpdateDependencies.sh"
  pause_if_needed
  exit 1
fi

log_line "[OK] Submodules updated."

# ------------------------------------------------------------------------------------
# 2) Git LFS in repo root
# ------------------------------------------------------------------------------------
echo
echo "[INFO] Checking Git LFS..."
if ! git lfs version >/dev/null 2>&1; then
  echo "[ERROR] Git LFS is not installed. Install it (e.g. 'brew install git-lfs') and re-run."
  log_line "[ERROR] Git LFS not installed."
  pause_if_needed
  exit 1
fi

git lfs version | tee -a "$LOGFILE"
log_line "[OK] Git LFS found."

unset GIT_LFS_SKIP_SMUDGE 2>/dev/null || true

echo
echo "[INFO] Updating Git LFS hooks in repo root..."
log_line "[INFO] Updating Git LFS hooks in repo root..."

run_and_tee "git lfs install --local --force"
if [[ $? -ne 0 ]]; then
  echo "[ERROR] git lfs install failed in repo root."
  log_line "[ERROR] git lfs install failed in repo root."
  pause_if_needed
  exit 1
fi

echo "[INFO] Pulling LFS files in repo root..."
log_line "[INFO] Pulling LFS files in repo root..."

run_and_tee "git lfs pull"
if [[ $? -ne 0 ]]; then
  echo "[ERROR] git lfs pull failed in repo root."
  log_line "[ERROR] git lfs pull failed in repo root."
  pause_if_needed
  exit 1
fi

# ------------------------------------------------------------------------------------
# 3) Submodules: LFS update + pull
# ------------------------------------------------------------------------------------
echo
echo "[INFO] Updating Git LFS hooks and pulling in submodules..."
log_line "[INFO] Updating Git LFS hooks + pulling in submodules..."

if [[ -f ".gitmodules" ]]; then
  while IFS= read -r path; do
    SUBMODULE_COUNT=$((SUBMODULE_COUNT + 1))

    echo
    echo "------------------------------------------------------------------------------------"
    echo "Submodule: $path"
    echo "------------------------------------------------------------------------------------"
    log_line "Submodule: $path"

    if [[ -d "$path" ]]; then
      run_and_tee "cd \"$REPO_DIR/$path\" && git lfs install --local --force && git lfs pull"
      if [[ $? -ne 0 ]]; then
        echo "[ERROR] Git LFS update/pull failed in submodule: $path"
        log_line "[ERROR] Git LFS update/pull failed in submodule: $path"
        pause_if_needed
        exit 1
      fi
    else
      echo "[WARN] Submodule folder missing: $path"
      log_line "[WARN] Submodule folder missing: $path"
    fi
  done < <(git config -f .gitmodules --get-regexp '^submodule\..*\.path$' 2>/dev/null | awk '{print $2}')
fi

# ------------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------------
END_TS="$(date +"%Y-%m-%d %H:%M:%S")"
END_EPOCH="$(date +%s)"
ELAPSED=$((END_EPOCH - START_EPOCH))

echo
echo "------------------------------------------------------------------------------------"
echo "SUCCESS: Environment setup complete."
echo "Log file: $LOGFILE"
echo "Submodules processed: $SUBMODULE_COUNT"
echo "Started: $START_TS"
echo "Ended  : $END_TS"
echo "Elapsed: ${ELAPSED}s"
echo "------------------------------------------------------------------------------------"

log_line "SUCCESS: Environment setup complete."
log_line "Log file: $LOGFILE"
log_line "Submodules processed: $SUBMODULE_COUNT"
log_line "Started: $START_TS"
log_line "Ended  : $END_TS"
log_line "Elapsed: ${ELAPSED}s"

pause_if_needed
exit 0
