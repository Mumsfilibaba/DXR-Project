include "../../BuildScripts/Scripts/build_module.lua"

-- NullRHI Module

local null_rhi_module = module_build_rules("NullRHI")
null_rhi_module.runtime_linking = true

null_rhi_module.add_module_dependencies
{
    "Core",
    "RHI",
}
