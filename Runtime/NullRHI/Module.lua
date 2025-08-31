include "BuildTool.lua"

-- NullRHI Module

local NullRHIModule = ModuleBuildRules("NullRHI")
NullRHIModule.bRuntimeLinking = true

NullRHIModule.AddModules({
    "Core",
    "RHI",
})
