include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- MetalRHI Module

if IsPlatformMac() then
    local MetalRHI = FModuleBuildRules("MetalRHI")
    MetalRHI.bRuntimeLinking = true
    
    MetalRHI.AddModuleDependencies( 
    {
        "Core",
        "CoreApplication",
        "RHI",
    })

    MetalRHI.AddFrameWorks(
    {
        "Metal",
        "QuartzCore"
    })
end