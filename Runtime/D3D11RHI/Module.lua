include "BuildTool.lua"

-- D3D11RHI Module

if IsPlatformWindows() then
    local D3D11RHI = ModuleBuildRules("D3D11RHI")
    D3D11RHI.bRuntimeLinking = true
    D3D11RHI.bUsePrecompiledHeaders = true

    D3D11RHI.AddModules({
        "Core",
        "CoreApplication",
        "RHI",
        "ShaderCore",
    })
end
