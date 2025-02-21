include "../../BuildScripts/Scripts/build_module.lua"

-- RHI Module

local rhi_module = module_build_rules("RHI")
rhi_module.use_precompiled_headers = true

rhi_module.add_external_include_dirs
{
    create_external_dependency_path("DXC/include"),
    create_external_dependency_path("SPIRV-Cross"),
    create_external_dependency_path("glslang"),
}

rhi_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
}

rhi_module.add_link_libraries
{
    "SPIRV",
    "MachineIndependent",
    "SPVRemapper",
    "glslang-default-resource-limits",
    "glslang",
    "SPIRV-Cross",
}
