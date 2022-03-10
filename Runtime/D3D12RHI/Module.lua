include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- D3D12RHI Module

if not BuildWithXcode() then
    local D3D12RHI = CModuleBuildRules('D3D12RHI')
    D3D12RHI.bRuntimeLinking = false
    
    D3D12RHI.AddModuleDependencies( 
    {
        'Core',
        'CoreApplication',
        'RHI',
    })

    D3D12RHI.Generate()
end
