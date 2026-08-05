-- Resolve from the root script location (robust even if CWD changes)
local ScriptsDir = path.join(_MAIN_SCRIPT_DIR, "../SetupScripts/Scripts")

-- Tell Premake to search here when you call include()/dofile()/require()
premake.path = premake.path .. ";" .. ScriptsDir

include "BuildTool.lua"

if not IsBuildMonolithic() then
    LogError("The tests workspace must be generated with --monolithic")
    error("Missing --monolithic", 0)
end

AddTargetSearchRoot(_MAIN_SCRIPT_DIR)
AddModuleSearchRoot(_MAIN_SCRIPT_DIR)

SearchForBuildFiles()

SetWorkspaceName("DXR-Engine Tests")

AddTarget("Core-Tests")
AddTarget("Core-Containers-Tests")
AddTarget("Core-Templates-Tests")
AddTarget("Core-Benchmarks")

AddTarget("Core-Math-Tests-Scalar")

if TargetsX86() then
    AddTarget("Core-Math-Tests-SSE")
    AddTarget("Core-Math-Tests-SSE2")
    AddTarget("Core-Math-Tests-SSE3")
    AddTarget("Core-Math-Tests-SSSE3")
    AddTarget("Core-Math-Tests-SSE4_1")
    AddTarget("Core-Math-Tests-SSE4_2")
else
    AddTarget("Core-Math-Tests-NEON")
end

AddTarget("RHI-Tests")

-- Generate the workspace
GenerateWorkspace()
