#!/usr/bin/env bash
#  Builds and runs every Core benchmark. Development and Release only; see
#  Scripts/RunSuites.sh for options.
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
exec "$DIR/Scripts/RunSuites.sh" Core benchmarks "$@"
