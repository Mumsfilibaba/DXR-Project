DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

./premake5 xcode4 --file=DXR-Engine-Tests/premake5.lua