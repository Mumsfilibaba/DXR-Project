-- Resolve from the root script location (robust even if CWD changes)
local ScriptsDir = path.join(_MAIN_SCRIPT_DIR, "SetupScripts/Scripts")

-- Tell Premake to search here when you call include()/dofile()/require()
premake.path = premake.path .. ";" .. ScriptsDir

-- Common
include "BuildTool.lua"

-- Add project folder
AddTargetSearchRoot("Sandbox")

-- Search for targets
SearchForBuildFiles()

-- Set the name of the workspace
SetWorkspaceName("DXR-Engine Sandbox")

-- Add the sandbox target to the workspace
AddTarget("Sandbox")

-- Generate the workspace
GenerateWorkspace()