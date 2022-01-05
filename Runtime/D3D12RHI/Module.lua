include '../../BuildScripts/Scripts/enginebuild.lua'

if not BuildWithXcode() then
	local D3D12RHIModule = CreateModule( 'D3D12RHI' )
	D3D12RHIModule.ModuleDependencies = 
	{
		'Core',
		'CoreApplication',
		'RHI',
	}

	D3D12RHIModule:Generate()
end
