include "BuildTool.lua"

local PlaygroundModules =
{
    "Core",
    "CoreApplication",
    "Application",
    "ApplicationRenderer",
    "RHI",
    "RendererCore",
    "NullRHI",
    "VulkanRHI",
}

if IsPlatformMac() then
    table.insert(PlaygroundModules, "MetalRHI")
elseif IsPlatformWindows() then
    table.insert(PlaygroundModules, "D3D12RHI")
end

-- Application Playground

local ApplicationPlayground = TargetBuildRules("Application-Playground")
ApplicationPlayground.TargetType = ETargetType.Program
ApplicationPlayground.Kind       = "WindowedApp"

ApplicationPlayground.AddModules(PlaygroundModules)

ApplicationPlayground.AddIncludeDirs({
    _SCRIPT_DIR
})

if IsPlatformMac() then
    ApplicationPlayground.AddFrameworks({
        "AppKit"
    })
end
