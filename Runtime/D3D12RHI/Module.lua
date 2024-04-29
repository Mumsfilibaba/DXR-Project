include "../../BuildScripts/Scripts/Build_Module.lua"

---------------------------------------------------------------------------------------------------
-- D3D12RHI Module

if IsPlatformWindows() then
    local D3D12RHI = FModuleBuildRules("D3D12RHI")
    D3D12RHI.bRuntimeLinking        = true
    D3D12RHI.bUsePrecompiledHeaders = true
    
    D3D12RHI.AddModuleDependencies( 
    {
        "Core",
        "CoreApplication",
        "RHI",
        'Project',
    })
end