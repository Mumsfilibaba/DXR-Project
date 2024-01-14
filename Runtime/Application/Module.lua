
include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Application Module

local ApplicationModule = FModuleBuildRules("Application")
ApplicationModule.bUsePrecompiledHeaders = true

ApplicationModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath("imgui")
})

ApplicationModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "RHI",
    "RendererCore",
    "Project",
})

ApplicationModule.AddLinkLibraries( 
{
    "ImGui",
})
