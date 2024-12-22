include "../../BuildScripts/Scripts/build_module.lua"

-- MetalRHI Module

if is_platform_mac() then
    local metal_rhi = module_build_rules("MetalRHI")
    metal_rhi.runtime_linking = true
    
    metal_rhi.add_module_dependencies
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
