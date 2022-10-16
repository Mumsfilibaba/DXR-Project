include "../BuildScripts/Scripts/build_workspace.lua"

---------------------------------------------------------------------------------------------------
-- Sandbox Project

local SandboxProject = FTargetBuildRules("Sandbox")
SandboxProject.AddModuleDependencies(
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Engine",
    "Renderer",
    "ViewportRenderer",
    "NullRHI",
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

GenerateWorkspace("DXR-Engine Sandbox", { SandboxProject })