include "../BuildScripts/Scripts/Build_Workspace.lua"

---------------------------------------------------------------------------------------------------
-- Sandbox Project

local SandboxProject = FTargetBuildRules("Sandbox")
SandboxProject.AddModuleDependencies(
{
    "Core",
    "CoreApplication",
    -- TODO: Look into injecting this into the executable
    "Launch",
    "Application",
    "RHI",
    "Engine",
    "Renderer",
    "NullRHI",
    "RendererCore",
    "Project",
})

if IsPlatformMac() then
    SandboxProject.AddModuleDependencies(
    { 
        "MetalRHI"
    })
elseif IsPlatformWindows() then
    SandboxProject.AddModuleDependencies(
    { 
        "D3D12RHI"
    })
end

FGenerateWorkspace("DXR-Engine Sandbox", { SandboxProject })