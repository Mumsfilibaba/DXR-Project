include "../../BuildScripts/Scripts/build_module.lua"

-- D3D12RHI Module

if is_platform_windows() then
    local d3d12rhi = module_build_rules("D3D12RHI")
    d3d12rhi.runtime_linking = true
    d3d12rhi.use_precompiled_headers = true
    
    d3d12rhi.add_module_dependencies
    {
        "Core",
        "CoreApplication",
        "RHI",
        "Project",
    }
end
