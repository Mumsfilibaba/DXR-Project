#!/usr/bin/env bash
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./Premake/premake5 xcode4 --file=../Tests/build.lua --platform=macOS --monolithic --buildsuffix=Tests
RC=$?
if [ $RC -ne 0 ]; then
    echo "[ERROR] Failed to generate the Xcode test workspace."
fi
exit $RC
