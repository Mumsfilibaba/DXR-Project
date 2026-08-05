include "BuildTool.lua"

local SandboxModules =
{
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
}

if IsPlatformMac() then
    table.insert(SandboxModules, "MetalRHI")
elseif IsPlatformWindows() then
    table.insert(SandboxModules, "D3D12RHI")
end

-- Sandbox Project
local Sandbox = TargetBuildRules("Sandbox")
Sandbox.TargetType = ETargetType.Game
Sandbox.AddModules(SandboxModules)

-- Sandbox Editor Project
local SandboxEditor = TargetBuildRules("SandboxEditor")
SandboxEditor.TargetType = ETargetType.Editor
SandboxEditor.AddModules(SandboxModules)
