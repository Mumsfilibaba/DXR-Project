include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- MetalRHI Module

if is_platform_mac() then
    local metal_rhi = module_build_rules("MetalRHI")
    metal_rhi.runtime_linking = true
    
    metal_rhi.add_module_thirdparties
    {
        "Core",
        "CoreApplication",
        "RHI",
    }

    metal_rhi.add_frameworks
    {
        "Metal",
        "QuartzCore",
    }
end
