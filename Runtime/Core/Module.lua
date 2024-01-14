include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Core Module

local CoreModule = FModuleBuildRules("Core")
CoreModule.bUsePrecompiledHeaders = true

if IsPlatformMac() then
    CoreModule.AddFrameWorks( 
    {
        "AppKit",
    })
elseif IsPlatformWindows() then
    CoreModule.AddLinkLibraries(
    {
        "Dbghelp.lib",
        "shlwapi.lib"
    })
end
