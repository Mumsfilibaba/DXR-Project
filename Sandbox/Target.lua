include '../BuildScripts/Scripts/build_workspace.lua'

local SandboxProject = CTargetBuildRules('Sandbox')
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
	'VulkanRHI'
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