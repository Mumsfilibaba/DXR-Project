include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- RendererCore Module

local renderer_core_module = module_build_rules("RendererCore")

renderer_core_module.add_module_thirdparties
{
    "Core",
    "RHI",
}
