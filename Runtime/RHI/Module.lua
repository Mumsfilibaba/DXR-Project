include '../../BuildScripts/Scripts/build_module.lua'

local RHIModule = CModuleBuildRules('RHI')
RHIModule.AddModuleDependencies( 
{
    'Core',
})

if BuildWithXcode() then
    RHIModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

RHIModule.Generate()