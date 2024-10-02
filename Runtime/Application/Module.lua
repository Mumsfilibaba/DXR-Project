
include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Application Module

local ApplicationModule = FModuleBuildRules("Application")
ApplicationModule.bUsePrecompiledHeaders = true

ApplicationModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "RHI",
    "RendererCore",
    "Project",
})