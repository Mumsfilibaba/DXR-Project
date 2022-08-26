include '../BuildScripts/Scripts/build_workspace.lua'

---------------------------------------------------------------------------------------------------
-- Sanbox Project

local SandboxProject = FTargetBuildRules('Sandbox')
SandboxProject.AddModuleDependencies(
{
    'Core',
    'CoreApplication',
    'Application',
    'RHI',
    'Engine',
    'Renderer',
    'InterfaceRenderer',
    'NullRHI',
})

if BuildWithXcode() then
    SandboxProject.AddModuleDependencies(
    { 
        'MetalRHI'
    })
else
    SandboxProject.AddModuleDependencies(
    { 
        'D3D12RHI'
    })
end

GenerateWorkspace('DXR-Engine Sandbox', { SandboxProject })