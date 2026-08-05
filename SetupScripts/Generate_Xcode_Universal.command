#!/usr/bin/env bash
# Generates the workspace as a universal binary, carrying both an x86_64 and an
# arm64 slice. Every source is compiled twice, so this is the slowest of the
# three architectures to build.
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./Generate_Xcode.command universal
RC=$?
if [ $RC -ne 0 ]; then
    echo "[ERROR] Failed to generate the universal Xcode workspace."
fi
exit $RC
