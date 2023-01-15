
include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Application Module

local ApplicationModule = FModuleBuildRules("Application")
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
})

ApplicationModule.AddLinkLibraries( 
{
    "ImGui",
})

if BuildWithXcode() then
    ApplicationModule.AddFrameWorks( 
    {
        "AppKit",
    })
end

ApplicationModule.Generate()