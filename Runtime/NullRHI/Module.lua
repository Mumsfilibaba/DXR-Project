include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- NullRHI Module

local null_rhi_module = module_build_rules("NullRHI")
null_rhi_module.runtime_linking = true

null_rhi_module.add_module_thirdparties
{
    "Core",
    "RHI",
}
