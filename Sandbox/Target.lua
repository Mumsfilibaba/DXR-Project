include "../SetupScripts/Scripts/build_workspace.lua"

-- Sandbox Project

local workspace = workspace_rules("DXR-Engine Sandbox")

local sandbox = target_build_rules("Sandbox", workspace)
sandbox.add_module_thirdparties
{
    "Core",
    "CoreApplication",
    "Launch",
    "Application",
    "RHI",
    "Engine",
    "Renderer",
    "NullRHI",
    "VulkanRHI",
    "RendererCore",
}

if is_platform_mac() then
    sandbox.add_module_thirdparties
    { 
        "MetalRHI"
    }
elseif is_platform_windows() then
    sandbox.add_module_thirdparties
    { 
        "D3D12RHI"
    }
end

workspace.add_target(sandbox)
workspace.generate()
