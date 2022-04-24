include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- CoreApplication Module

local CoreApplicationModule = CModuleBuildRules('CoreApplication')
CoreApplicationModule.AddModuleDependencies( 
{
    'Core'
})

if BuildWithXcode() then
    CoreApplicationModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

CoreApplicationModule.Generate()