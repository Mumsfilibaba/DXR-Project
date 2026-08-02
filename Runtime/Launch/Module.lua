include "BuildTool.lua"

-- Launch Module

local LaunchModule = ModuleBuildRules("Launch")
LaunchModule.bIsDynamic = false

if IsPlatformWindows() then
    local AgilitySDKScript = JoinPath(GetRuntimeFolderPath(), "D3D12RHI/D3D12AgilitySDK.lua")
    if os.isfile(AgilitySDKScript) then
        include(AgilitySDKScript)
        LaunchModule.AddDefines(GetD3D12AgilitySDKDefines())
    else
        LogWarning("[Launch] '%s' not found, disabling the D3D12 Agility SDK exports.", AgilitySDKScript)
        LaunchModule.AddDefines({ "D3D12_AGILITY_SDK_EXPORTS=(0)" })
    end
end

LaunchModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
})
