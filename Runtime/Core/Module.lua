include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- Core Module

local CoreModule = FModuleBuildRules("Core")
CoreModule.bUsePrecompiledHeaders = true

-- TODO: Ensure that frameworks gets propagated up with dependencies
if IsPlatformMac() then
    CoreModule.AddFrameWorks( 
    {
        "AppKit",
    })
end

CoreModule.Generate()