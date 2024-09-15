
include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- ImGuiPlugin Module

local ImGuiPluginModule = FModuleBuildRules("ImGuiPlugin")

ImGuiPluginModule.AddSystemIncludes( 
{
    CreateExternalDependencyPath("imgui")
})

ImGuiPluginModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
})

ImGuiPluginModule.AddLinkLibraries( 
{
    "ImGui",
})
