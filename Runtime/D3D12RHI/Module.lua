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

        -- Copy dynamic libraries from thirdparties folder into the folder that
        -- D3D12_AGILITY_SDK_PATH points the D3D12 loader at
        local AgilitySdkFolder  = JoinPath(D3D12RHI.GetTargetFolderPath(), GetD3D12AgilitySDKRedistFolderName())
        local PostBuildCommands = {
            ('if not exist "%s" mkdir "%s"'):format(AgilitySdkFolder, AgilitySdkFolder), -- Ensure folder exists before copying
        }

        for _, RedistFile in ipairs(GetD3D12AgilitySDKRedistFiles()) do
            table.insert(PostBuildCommands, ('copy /Y "%s" "%s"\\'):format(RedistFile, AgilitySdkFolder))
        end

        D3D12RHI.AddPostBuildCommands(PostBuildCommands)
    end
end
