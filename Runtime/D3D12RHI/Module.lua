include "BuildTool.lua"
include(JoinPath(GetRuntimeFolderPath(), "D3D12RHI/D3D12AgilitySDK.lua"))

-- D3D12RHI Module

if IsPlatformWindows() then
    local D3D12RHI = ModuleBuildRules("D3D12RHI")
    D3D12RHI.bRuntimeLinking = true
    D3D12RHI.bUsePrecompiledHeaders = true

    D3D12RHI.AddModules({
        "Core",
        "CoreApplication",
        "RHI",
    })

    -- The Agility SDK is optional, without it we build against the Windows SDK headers
    if HasD3D12AgilitySDK() then
        D3D12RHI.AddIncludeDirs({
            GetD3D12AgilitySDKIncludePath()
        })

        -- Copy dynamic libraries from thirdparties folder
        local AgilitySdkFolder   = JoinPath(D3D12RHI.GetTargetFolderPath(), GetD3D12AgilitySDKRedistFolderName())
        local AgilitySdkBinaries = GetD3D12AgilitySDKBinaryPath()
        D3D12RHI.AddPostBuildCommands({
            ('if not exist "%s" mkdir "%s"'):format(AgilitySdkFolder, AgilitySdkFolder), -- Ensure folder exists before copying
            ('copy /Y "%s" "%s"\\'):format(JoinPath(AgilitySdkBinaries, "D3D12Core.dll"), AgilitySdkFolder),
            ('copy /Y "%s" "%s"\\'):format(JoinPath(AgilitySdkBinaries, "D3D12Core.pdb"), AgilitySdkFolder),
            ('copy /Y "%s" "%s"\\'):format(JoinPath(AgilitySdkBinaries, "d3d12SDKLayers.dll"), AgilitySdkFolder),
            ('copy /Y "%s" "%s"\\'):format(JoinPath(AgilitySdkBinaries, "d3d12SDKLayers.pdb"), AgilitySdkFolder),
            ('copy /Y "%s" "%s"\\'):format(JoinPath(AgilitySdkBinaries, "d3dconfig.exe"), AgilitySdkFolder),
            ('copy /Y "%s" "%s"\\'):format(JoinPath(AgilitySdkBinaries, "d3dconfig.pdb"), AgilitySdkFolder),
        })
    end
end
