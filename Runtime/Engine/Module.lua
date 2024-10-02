include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Engine Module

local EngineModule = FModuleBuildRules("Engine")
EngineModule.bUsePrecompiledHeaders = true

EngineModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath("imgui"),
    CreateExternalDependencyPath("stb_image"),
    CreateExternalDependencyPath("tinyobjloader"),
    CreateExternalDependencyPath("tinyddsloader"),
    CreateExternalDependencyPath("OpenFBX/src"),
})

EngineModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
    "Project",
    "ImGuiPlugin",
})

EngineModule.AddLinkLibraries( 
{
    "ImGui",
    "tinyobjloader",
    "OpenFBX",
})
