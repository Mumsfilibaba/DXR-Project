include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- NullRHI Module

local NullRHIModule = FModuleBuildRules("NullRHI")
NullRHIModule.bRuntimeLinking = false

NullRHIModule.AddModuleDependencies( 
{
    "Core",
    "RHI",
})

NullRHIModule.Generate()