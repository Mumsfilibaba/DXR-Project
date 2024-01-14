include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- RendererCore Module

local RendererCoreModule = FModuleBuildRules("RendererCore")

RendererCoreModule.AddModuleDependencies( 
{
    "Core",
    "RHI",
})
