include '../../BuildScripts/Scripts/build_module.lua'

local InterfaceModule = CModuleBuildRules('Interface')
InterfaceModule.AddSystemIncludes( 
{
    MakeExternalDependencyPath('imgui'),
})

InterfaceModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
})

InterfaceModule.AddLinkLibraries( 
{
    'ImGui',
})

if BuildWithXcode() then
    InterfaceModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

InterfaceModule.Generate()
