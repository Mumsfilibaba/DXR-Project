include "../../BuildScripts/Scripts/build_module.lua"

-- Launch Module

local launch_module = module_build_rules("Launch")
launch_module.is_dynamic = false

if is_platform_windows() then
    launch_module.add_defines
    { 
        "D3D12_AGILITY_SDK_EXPORTS=(1)",
        "D3D12_AGILITY_SDK_VERSION=(716)",
        "D3D12_AGILITY_SDK_PATH=\".\\\\D3D12\\\\\""
    }
end

launch_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
    "Project",
}
