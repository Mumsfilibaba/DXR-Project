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

if BuildWithXcode() then
    SandboxProject.AddModuleDependencies(
    { 
        "MetalRHI"
    })
else
    SandboxProject.AddModuleDependencies(
    { 
        "D3D12RHI"
    })
end

FGenerateWorkspace("DXR-Engine Sandbox", { SandboxProject })