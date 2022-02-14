include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- ApplicationModule

local ApplicationModule = CModuleBuildRules('Application')
ApplicationModule.AddSystemIncludes( 
{
    MakeExternalDependencyPath('imgui'),
})

ApplicationModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
})

ApplicationModule.AddLinkLibraries( 
{
    'ImGui',
})

if BuildWithXcode() then
    ApplicationModule.AddFrameWorks( 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    })
end

ApplicationModule.Generate()
