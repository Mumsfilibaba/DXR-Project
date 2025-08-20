include "../SetupScripts/Scripts/BuildTool_Workspace.lua"

-- Sandbox Project

local Workspace = WorkspaceRules("DXR-Engine Sandbox")

local Sandbox = TargetBuildRules("Sandbox", Workspace)
Sandbox.AddModuleThirdparties
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

if IsPlatformMac() then
    Sandbox.AddModuleThirdparties
    { 
        "MetalRHI"
    }
elseif IsPlatformWindows() then
    Sandbox.AddModuleThirdparties
    { 
        "D3D12RHI"
    }
end

Workspace.AddTarget(Sandbox)
Workspace.Generate()
