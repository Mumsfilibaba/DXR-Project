#!/usr/bin/env bash
#  Builds and runs every Core test suite. See Scripts/RunSuites.sh for options.
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
exec "$DIR/Scripts/RunSuites.sh" Core tests "$@"
