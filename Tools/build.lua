-- Resolve from the root script location (robust even if CWD changes)
local ScriptsDir = path.join(_MAIN_SCRIPT_DIR, "../SetupScripts/Scripts")

-- Tell Premake to search here when you call include()/dofile()/require()
premake.path = premake.path .. ";" .. ScriptsDir

include "BuildTool.lua"

if not IsBuildMonolithic() then
    LogError("The tools workspace must be generated with --monolithic")
    error("Missing --monolithic", 0)
end

AddTargetSearchRoot(_MAIN_SCRIPT_DIR)

SearchForBuildFiles()

SetWorkspaceName("DXR-Engine Tools")

AddTarget("BlueNoiseGen")

-- Generate the workspace
GenerateWorkspace()
