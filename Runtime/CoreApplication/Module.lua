include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- CoreApplication Module

local CoreApplicationModule = FModuleBuildRules("CoreApplication")
CoreApplicationModule.AddModuleDependencies( 
{
    "Core"
})

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
