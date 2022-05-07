include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Renderer Module

local RendererModule = CModuleBuildRules('Renderer')
RendererModule.AddSystemIncludes( 
{
    MakeExternalDependencyPath('imgui'),
})

RendererModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'Canvas',
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