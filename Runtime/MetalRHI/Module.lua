include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- MetalRHI Module

if BuildWithXcode() then
    local MetalRHI = FModuleBuildRules('MetalRHI')
    MetalRHI.bRuntimeLinking = false
    
    MetalRHI.AddModuleDependencies( 
    {
        'Core',
        'CoreApplication',
        'RHI',
    })

    MetalRHI.AddFrameWorks(
    {
        'Metal',
        'Cocoa',
        'QuartzCore'
    })

    MetalRHI.Generate()
end