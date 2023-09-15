DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

git submodule init
git submodule update

cd ../Dependencies/imgui
git pull origin docking

cd ../OpenFBX
git pull origin master

cd ../SPIRV-Cross
git pull origin main

cd ../glslang
git pull origin main

cd ../tinyddsloader
git pull origin master

cd ../tinyobjloader
git pull origin release