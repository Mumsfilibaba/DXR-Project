include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- InterfaceRenderer Module

local InterfaceRendererModule = CModuleBuildRules('InterfaceRenderer')
InterfaceRendererModule.bRuntimeLinking = false

InterfaceRendererModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath('imgui'),
})

InterfaceRendererModule.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'Canvas',
    'Engine',
    'RHI',
})

InterfaceRendererModule.AddLinkLibraries( 
{
    'ImGui',
})

InterfaceRendererModule.Generate()