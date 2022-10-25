include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- Engine Module

local EngineModule = FModuleBuildRules("Engine")
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
})

EngineModule.AddLinkLibraries( 
{
    "ImGui",
    "tinyobjloader",
    "OpenFBX",
})

if BuildWithXcode() then
    EngineModule.AddFrameWorks( 
    {
        "AppKit",
    })
end

EngineModule.Generate()
