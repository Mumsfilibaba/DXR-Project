include '../../BuildScripts/Scripts/build_module.lua'

if not BuildWithXcode() then
	local D3D12RHIModule = CModuleBuildRules('D3D12RHI')
	D3D12RHIModule.bRuntimeLinking = false
	
	D3D12RHIModule.AddModuleDependencies( 
	{
		'Core',
		'CoreApplication',
		'RHI',
	})

	D3D12RHIModule.Generate()
end
