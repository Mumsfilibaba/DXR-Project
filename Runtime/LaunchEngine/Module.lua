include "BuildTool.lua"

-- LaunchEngine Module

local LaunchEngineModule = ModuleBuildRules("LaunchEngine")
LaunchEngineModule.bIsDynamic = false

if IsPlatformWindows() then
    local AgilitySDKScript = JoinPath(GetRuntimeFolderPath(), "D3D12RHI/D3D12AgilitySDK.lua")
    if os.isfile(AgilitySDKScript) then
        include(AgilitySDKScript)
        LaunchEngineModule.AddDefines(GetD3D12AgilitySDKDefines())
    else
        LogWarning("[LaunchEngine] '%s' not found, disabling the D3D12 Agility SDK exports.", AgilitySDKScript)
        LaunchEngineModule.AddDefines({ "D3D12_AGILITY_SDK_EXPORTS=(0)" })
    end
end

LaunchEngineModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "ApplicationRenderer",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
})
