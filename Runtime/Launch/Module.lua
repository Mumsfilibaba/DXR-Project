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
})

if BuildWithXcode() then
    LaunchModule.AddFrameWorks( 
    {
        "Cocoa",
        "AppKit",
    })
end

LaunchModule.Generate()