include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- RendererCore Module

local RendererCoreModule = FModuleBuildRules("RendererCore")

RendererCoreModule.AddModuleDependencies( 
{
    "Core",
    "RHI",
})

RendererCoreModule.Generate()