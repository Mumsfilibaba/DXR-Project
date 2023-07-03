include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- RHI Module

local RHIModule = FModuleBuildRules("RHI")

RHIModule.AddSystemIncludes(
{
    CreateExternalDependencyPath("DXC/include"),
    CreateExternalDependencyPath("SPIRV-Cross")
})

RHIModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication"
})

RHIModule.AddLinkLibraries( 
{
    "SPIRV-Cross",
})
