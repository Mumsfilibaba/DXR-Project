include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- ViewportRenderer Module

local ViewportRendererModule = FModuleBuildRules("ViewportRenderer")
ViewportRendererModule.bRuntimeLinking = false

ViewportRendererModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath("imgui"),
})

ViewportRendererModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "Application",
    "Engine",
    "RHI",
})

ViewportRendererModule.AddLinkLibraries( 
{
    "ImGui",
})

ViewportRendererModule.Generate()