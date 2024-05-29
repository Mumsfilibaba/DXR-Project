git submodule init
git submodule update

cd ../Dependencies/imgui
git checkout docking
git pull origin docking
git status

cd ../OpenFBX
git checkout master
git pull origin master
git status

cd ../SPIRV-Cross
git checkout main
git pull origin main
git status

cd ../glslang
git checkout main
git pull origin main
git status

cd ../tinyddsloader
git checkout master
git pull origin master
git status

cd ../tinyobjloader
git checkout release
git pull origin release
git status

pause