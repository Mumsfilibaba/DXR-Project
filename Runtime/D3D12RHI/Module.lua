include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- D3D12RHI Module

if IsPlatformWindows() then
    local D3D12RHI = ModuleBuildRules("D3D12RHI")
    D3D12RHI.bRuntimeLinking = true
    D3D12RHI.bUsePrecompiledHeaders = true

    D3D12RHI.AddIncludeDirs
    {
        CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/include")
    }

    D3D12RHI.AddModuleThirdparties
    {
        "Core",
        "CoreApplication",
        "RHI",
    }
end
