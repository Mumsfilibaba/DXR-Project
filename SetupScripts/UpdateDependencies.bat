@echo Updating submodules

git submodule sync --recursive || exit /b 1
git submodule update --init --recursive || exit /b 1

@echo Finished updating submodules

REM pause