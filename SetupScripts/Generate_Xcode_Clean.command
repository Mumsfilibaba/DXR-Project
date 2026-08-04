#!/usr/bin/env bash
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./Premake/premake5 clean
RC=$?
if [ $RC -ne 0 ]; then
    echo "[ERROR] Failed to clean the generated project files."
fi
exit $RC
