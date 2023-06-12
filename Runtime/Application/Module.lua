
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
    "Project",
})

ApplicationModule.AddLinkLibraries( 
{
    "ImGui",
})

-- TODO: Ensure that frameworks gets propagated up with dependencies
if IsPlatformMac() then
    ApplicationModule.AddFrameWorks( 
    {
        "AppKit",
    })
end

ApplicationModule.Generate()