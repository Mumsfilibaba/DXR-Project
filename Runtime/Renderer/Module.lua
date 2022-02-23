include '../../BuildScripts/Scripts/build_module.lua'

local RendererModule = CModuleBuildRules('Renderer')
RendererModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath('imgui'),
})

RendererModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'Application',
    'RHI',
    'Engine',
})

RendererModule.AddLinkLibraries( 
{
    'ImGui',
})

if BuildWithXcode() then
    RendererModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

RendererModule.Generate()
