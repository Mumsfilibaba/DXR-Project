include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- CoreApplication Module

local CoreApplicationModule = FModuleBuildRules("CoreApplication")
CoreApplicationModule.AddModuleDependencies( 
{
    "Core"
})

-- TODO: Ensure that frameworks gets propagated up with dependencies
if IsPlatformMac() then
    CoreApplicationModule.AddFrameWorks( 
    {
        "Cocoa",
        "AppKit",
    })
elseif IsPlatformWindows() then
    CoreApplicationModule.AddLinkLibraries(
    {
        "Shcore.lib"
    })
end

CoreApplicationModule.Generate()