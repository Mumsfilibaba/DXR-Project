include "BuildTool_Module.lua"

-- D3D12RHI Module

if IsPlatformWindows() then
    local D3D12RHI = ModuleBuildRules("D3D12RHI")
    D3D12RHI.bRuntimeLinking = true
    D3D12RHI.bUsePrecompiledHeaders = true

    D3D12RHI.AddIncludeDirs({
        CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/include")
    })

    D3D12RHI.AddModules({
        "Core",
        "CoreApplication",
        "RHI",
    })

    -- Copy dynamic libraries from thirdparties folder
    local AgilitySdkFolder = JoinPath(D3D12RHI.GetTargetFolderPath(), "D3D12")
    LogHighlight("AgilitySdkFolder: " .. AgilitySdkFolder)

    D3D12RHI.AddPostBuildCommands({
        "if not exist \"" .. AgilitySdkFolder .. "\" mkdir \"" .. AgilitySdkFolder .. "\"", -- Ensure folder exists before copying
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/D3D12Core.dll") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/D3D12Core.pdb") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3d12SDKLayers.dll") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3d12SDKLayers.pdb") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3dconfig.exe") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3dconfig.pdb") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/DirectSR.dll") .. " " .. AgilitySdkFolder,
        "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/DirectSR.pdb") .. " " .. AgilitySdkFolder,
    })
end
