include '../../BuildScripts/Scripts/build_module.lua'

local InterfaceRendererModule = CModuleBuildRules('InterfaceRenderer')
InterfaceRendererModule.bRuntimeLinking = false

InterfaceRendererModule.AddSystemIncludes( 
{
    MakeExternalDependencyPath('imgui'),
})

InterfaceRendererModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'Interface',
    'Engine',
    'RHI',
})

InterfaceRendererModule.AddLinkLibraries( 
{
    'ImGui',
})

if BuildWithXcode() then
    InterfaceRendererModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

InterfaceRendererModule.Generate()