include "BuildTool.lua"

-- Sandbox Project
local Sandbox = TargetBuildRules("Sandbox")
Sandbox.TargetType = ETargetType.Game

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

-- Sandbox Project
local SandboxEditor = TargetBuildRules("SandboxEditor")
SandboxEditor.TargetType = ETargetType.Editor

SandboxEditor.AddModules({
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
    SandboxEditor.AddModules({ 
        "MetalRHI"
    })
elseif IsPlatformWindows() then
    SandboxEditor.AddModules({ 
        "D3D12RHI"
    })
end
