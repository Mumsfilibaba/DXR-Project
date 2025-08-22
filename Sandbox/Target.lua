include "../SetupScripts/Scripts/BuildTool_Workspace.lua"

-- Sandbox Project

local Workspace = WorkspaceRules("DXR-Engine Sandbox")

local Sandbox = TargetBuildRules("Sandbox", Workspace)
Sandbox.AddModules
({
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
})

if IsPlatformMac() then
    Sandbox.AddModules
    ({ 
        "MetalRHI"
    })
elseif IsPlatformWindows() then
    Sandbox.AddModules
    ({ 
        "D3D12RHI"
    })
end

Workspace.AddTarget(Sandbox)
Workspace.Generate()
