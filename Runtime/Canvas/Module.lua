
include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Canvas Module

local CanvasModule = FModuleBuildRules('Canvas')
CanvasModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath('imgui')
})

CanvasModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
})

CanvasModule.AddLinkLibraries( 
{
    'ImGui',
})

if BuildWithXcode() then
    CanvasModule.AddFrameWorks( 
    {
        'AppKit',
    })
end

CanvasModule.Generate()