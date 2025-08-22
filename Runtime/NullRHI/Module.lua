include "BuildTool_Module.lua"

-- NullRHI Module

local NullRHIModule = ModuleBuildRules("NullRHI")
NullRHIModule.bRuntimeLinking = true

NullRHIModule.AddModules({
    "Core",
    "RHI",
})
