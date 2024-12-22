include "../../BuildScripts/Scripts/build_module.lua"

-- RendererCore Module

local renderer_core_module = module_build_rules("RendererCore")

renderer_core_module.add_module_dependencies
{
    "Core",
    "RHI",
}
