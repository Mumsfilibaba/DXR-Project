include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- MetalRHI Module

if BuildWithXcode() then
    local MetalRHI = CModuleBuildRules('MetalRHI')
    MetalRHI.bRuntimeLinking = false
    
    MetalRHI.AddModuleDependencies( 
    {
        'Core',
        'CoreApplication',
        'RHI',
    })

    MetalRHI.AddFrameWorks(
    {
        'Metal'
    })

    MetalRHI.Generate()
end