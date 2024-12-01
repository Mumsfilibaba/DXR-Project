include "../BuildScripts/Scripts/build_workspace.lua"

-- Sandbox Project

local workspace = workspace_rules("DXR-Engine Sandbox")

local sandbox = target_build_rules("Sandbox", workspace)
sandbox.add_module_dependencies
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
    "Project",
}

if is_platform_mac() then
    sandbox.add_module_dependencies
    { 
        "MetalRHI"
    }
elseif is_platform_windows() then
    sandbox.add_module_dependencies
    { 
        "D3D12RHI"
    }
end

workspace.add_target(sandbox)
workspace.generate()
