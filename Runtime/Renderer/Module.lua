include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- Renderer Module

local RendererModule = FModuleBuildRules("Renderer")
RendererModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath("imgui"),
})

RendererModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Engine",
})

RendererModule.AddLinkLibraries( 
{
    "ImGui",
})

RendererModule.Generate()