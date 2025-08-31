-- Resolve from the root script location (robust even if CWD changes)
local ScriptsDir = path.join(_MAIN_SCRIPT_DIR, "SetupScripts/Scripts")

-- Tell Premake to search here when you call include()/dofile()/require()
premake.path = premake.path .. ";" .. ScriptsDir

-- Common
include "BuildTool.lua"

-- Project
include "Sandbox/Target.lua"