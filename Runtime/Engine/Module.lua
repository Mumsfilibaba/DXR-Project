include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Engine Module

local EngineModule = CModuleBuildRules('Engine')
EngineModule.AddSystemIncludes( 
{
    MakeExternalDependencyPath('imgui'),
    MakeExternalDependencyPath('stb_image'),
    MakeExternalDependencyPath('tinyobjloader'),
    MakeExternalDependencyPath('OpenFBX/src'),
})

EngineModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'Canvas',
    'RHI',
})

EngineModule.AddLinkLibraries( 
{
    'ImGui',
    'tinyobjloader',
    'OpenFBX',
})

if BuildWithXcode() then
    EngineModule.AddFrameWorks( 
    {
        'AppKit',
    })
end

EngineModule.Generate()
