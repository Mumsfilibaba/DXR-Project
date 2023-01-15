include "../../BuildScripts/Scripts/Build_Module.lua"

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
    "RendererCore",
})

RendererModule.AddLinkLibraries( 
{
    "ImGui",
})

RendererModule.Generate()