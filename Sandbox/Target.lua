include "BuildTool.lua"

-- Sandbox Project
local Sandbox = TargetBuildRules("Sandbox")

Sandbox.AddModules({
    "Core",
    "CoreApplication",
    "Launch",
    "Application",
    "RHI",
    "Engine",
    "Renderer",
    "NullRHI",
    "VulkanRHI",
    "RendererCore",
})

if IsPlatformMac() then
    Sandbox.AddModules({ 
        "MetalRHI"
    })
elseif IsPlatformWindows() then
    Sandbox.AddModules({ 
        "D3D12RHI"
    })
end
