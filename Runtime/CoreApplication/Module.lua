include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- CoreApplication Module

local CoreApplicationModule = FModuleBuildRules("CoreApplication")
CoreApplicationModule.bUsePrecompiledHeaders = true

CoreApplicationModule.AddModuleDependencies({ "Core" })

if IsPlatformMac() then
    CoreApplicationModule.AddFrameWorks({ "Cocoa", "AppKit", "IOKit", "GameController" })
elseif IsPlatformWindows() then
    CoreApplicationModule.AddLinkLibraries({ "Shcore.lib" })
end
