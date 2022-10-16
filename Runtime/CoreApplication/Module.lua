include "../../BuildScripts/Scripts/build_module.lua"

---------------------------------------------------------------------------------------------------
-- CoreApplication Module

local CoreApplicationModule = FModuleBuildRules("CoreApplication")
CoreApplicationModule.AddModuleDependencies( 
{
    "Core"
})

if BuildWithXcode() then
    CoreApplicationModule.AddFrameWorks( 
    {
        "Cocoa",
        "AppKit",
    })
end

CoreApplicationModule.Generate()