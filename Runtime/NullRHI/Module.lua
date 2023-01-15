include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- NullRHI Module

local NullRHIModule = FModuleBuildRules("NullRHI")
NullRHIModule.bRuntimeLinking = true

NullRHIModule.AddModuleDependencies( 
{
    "Core",
    "RHI",
})

NullRHIModule.Generate()