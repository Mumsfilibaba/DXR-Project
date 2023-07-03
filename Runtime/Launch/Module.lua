include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Launch Module

local LaunchModule = FModuleBuildRules("Launch")
LaunchModule.bIsDynamic = false

LaunchModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
    "Project",
})
