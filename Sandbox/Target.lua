include "../BuildScripts/Scripts/build_workspace.lua"

---------------------------------------------------------------------------------------------------
-- Sandbox Project

local Workspace = FWorkspaceRules("DXR-Engine Sandbox")

local Sandbox = FTargetBuildRules("Sandbox", Workspace)
Sandbox.AddModuleDependencies(
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
})

if IsPlatformMac() then
    Sandbox.AddModuleDependencies(
    { 
        "MetalRHI"
    })
elseif IsPlatformWindows() then
    Sandbox.AddModuleDependencies(
    { 
        "D3D12RHI"
    })
end

Workspace.AddTarget(Sandbox)
Workspace.Generate()
