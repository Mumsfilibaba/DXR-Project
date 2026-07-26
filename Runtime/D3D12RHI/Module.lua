include "BuildTool.lua"

-- D3D12RHI Module

if IsPlatformWindows() then
    local D3D12RHI = ModuleBuildRules("D3D12RHI")
    D3D12RHI.bRuntimeLinking = true
    D3D12RHI.bUsePrecompiledHeaders = true

    D3D12RHI.AddIncludeDirs({
        CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/include")
    })

    D3D12RHI.AddModules({
        "Core",
        "CoreApplication",
        "RHI",
    })

    -- Copy dynamic libraries from thirdparties folder
    local AgilitySdkFolder = JoinPath(D3D12RHI.GetTargetFolderPath(), "D3D12")
    D3D12RHI.AddPostBuildCommands({
        ('if not exist "%s" mkdir "%s"'):format(AgilitySdkFolder, AgilitySdkFolder), -- Ensure folder exists before copying
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/bin/x64/D3D12Core.dll"), AgilitySdkFolder),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/bin/x64/D3D12Core.pdb"), AgilitySdkFolder),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/bin/x64/d3d12SDKLayers.dll"), AgilitySdkFolder),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/bin/x64/d3d12SDKLayers.pdb"), AgilitySdkFolder),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/bin/x64/d3dconfig.exe"), AgilitySdkFolder),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.619.3/build/native/bin/x64/d3dconfig.pdb"), AgilitySdkFolder),
    })
end
