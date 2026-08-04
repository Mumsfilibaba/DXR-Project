#!/usr/bin/env bash
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./Premake/premake5 xcode4 --file=../build.lua --platform=macOS
RC=$?
if [ $RC -ne 0 ]; then
    echo "[ERROR] Failed to generate the Xcode workspace."
fi
exit $RC
