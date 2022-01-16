include '../BuildScripts/Scripts/build_workspace.lua'

local SandboxProject = CTargetBuildRules('Sandbox')
SandboxProject.AddModuleDependencies(
{
	'Core',
	'CoreApplication',
	'Interface',
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
else
	SandboxProject.AddModuleDependencies(
	{ 
		'D3D12RHI'
	})
end

GenerateWorkspace('DXR-Engine Sandbox', { SandboxProject })