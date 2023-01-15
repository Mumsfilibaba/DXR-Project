include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- MetalRHI Module

if BuildWithXcode() then
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
        "Cocoa",
        "QuartzCore"
    })

    MetalRHI.Generate()
end