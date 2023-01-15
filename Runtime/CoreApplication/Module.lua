include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- CoreApplication Module

local CoreApplicationModule = FModuleBuildRules("CoreApplication")
CoreApplicationModule.AddModuleDependencies( 
{
    "Core"
})

-- TODO: Ensure that frameworks gets propagated up with dependencies
if BuildWithXcode() then
    CoreApplicationModule.AddFrameWorks( 
    {
        "Cocoa",
        "AppKit",
    })
end

CoreApplicationModule.Generate()