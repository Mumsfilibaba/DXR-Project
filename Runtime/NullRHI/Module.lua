include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- NullRHI Module

local NullRHIModule = CModuleBuildRules('NullRHI')
NullRHIModule.bRuntimeLinking = false

NullRHIModule.AddModuleDependencies( 
{
    'Core',
    'RHI',
})

if BuildWithXcode() then
    NullRHIModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

NullRHIModule.Generate()