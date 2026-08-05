#!/usr/bin/env bash
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

# Optional: x86_64, arm64 or universal. Defaults to the host architecture.
ARCH="${1:-}"

./Premake/premake5 xcode4 --file=../build.lua --platform=macOS --monolithic ${ARCH:+--architecture="$ARCH"}
RC=$?
if [ $RC -ne 0 ]; then
    echo "[ERROR] Failed to generate the monolithic Xcode workspace."
fi
exit $RC
