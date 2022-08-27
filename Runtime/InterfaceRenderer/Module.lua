include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- InterfaceRenderer Module

local InterfaceRendererModule = FModuleBuildRules('InterfaceRenderer')
InterfaceRendererModule.bRuntimeLinking = false

InterfaceRendererModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath('imgui'),
})

InterfaceRendererModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'Application',
    'Engine',
    'RHI',
})

InterfaceRendererModule.AddLinkLibraries( 
{
    'ImGui',
})

InterfaceRendererModule.Generate()