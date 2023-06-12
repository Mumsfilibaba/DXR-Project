include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Launch Module

local LaunchModule = FModuleBuildRules("Launch")
LaunchModule.bIsDynamic = false

LaunchModule.AddModuleDependencies( 
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
    "Project",
})

-- TODO: Ensure that frameworks gets propagated up with dependencies
if IsPlatformMac() then
    LaunchModule.AddFrameWorks( 
    {
        "Cocoa",
        "AppKit",
    })
end

LaunchModule.Generate()