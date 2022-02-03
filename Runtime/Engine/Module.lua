include '../../BuildScripts/Scripts/build_module.lua'

local EngineModule = CModuleBuildRules('Engine')
EngineModule.AddSystemIncludes( 
{
    MakeExternalDependencyPath('imgui'),
    MakeExternalDependencyPath('stb_image'),
    MakeExternalDependencyPath('tinyobjloader'),
    MakeExternalDependencyPath('OpenFBX/src'),
})

EngineModule.ModuleDependencies = 
{
    'Core',
    'CoreApplication',
    'Application',
    'RHI',
}

EngineModule.LinkLibraries = 
{
    'ImGui',
    'tinyobjloader',
    'OpenFBX',
}

if BuildWithXcode() then
    EngineModule.FrameWorks = 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
end

EngineModule.Generate()
