include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- Launch Module

local LaunchModule = ModuleBuildRules("Launch")
LaunchModule.bIsDynamic = false

if IsPlatformWindows() then
    LaunchModule.AddDefines
    ({ 
        "D3D12_AGILITY_SDK_EXPORTS=(1)",
        "D3D12_AGILITY_SDK_VERSION=(716)",
        "D3D12_AGILITY_SDK_PATH=\".\\\\D3D12\\\\\""
    })
end

LaunchModule.AddModules
({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
})
