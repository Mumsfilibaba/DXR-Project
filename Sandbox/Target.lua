include '../BuildScripts/Scripts/build_workspace.lua'

---------------------------------------------------------------------------------------------------
-- Sanbox Project

local SandboxProject = CTargetBuildRules('Sandbox')
SandboxProject.AddModuleDependencies(
{
    'Core',
    'CoreApplication',
    'Canvas',
    'RHI',
    'Engine',
    'Renderer',
    'InterfaceRenderer',
    'NullRHI',
})

if BuildWithXcode() then
    SandboxProject.AddFrameWorks(
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })

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