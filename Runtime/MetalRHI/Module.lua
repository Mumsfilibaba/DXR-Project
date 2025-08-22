include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- MetalRHI Module

if IsPlatformMac() then
    local MetalRHI = ModuleBuildRules("MetalRHI")
    MetalRHI.bRuntimeLinking = true
    
    MetalRHI.AddModules
    ({
        "Core",
        "CoreApplication",
        "RHI",
    })

    MetalRHI.AddFrameworks
    ({
        "Metal",
        "QuartzCore",
    })
end
