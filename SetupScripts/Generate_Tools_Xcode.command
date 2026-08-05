DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./Premake/premake5 xcode4 --file=../Tools/build.lua --platform=macOS --monolithic --buildsuffix=Tools