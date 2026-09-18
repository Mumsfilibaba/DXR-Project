include "BuildTool.lua"

-- Boots each platform RHI far enough to own a device, without a window or swap chain.
local RHIBootTests = TargetBuildRules("RHI-Boot-Tests")
RHIBootTests.TargetType = ETargetType.Program
RHIBootTests.Kind       = "ConsoleApp"

RHIBootTests.AddModules({
    "Core",
    "CoreApplication",
    "RHI",
    "NullRHI",
    "TestCommon",
})

if IsPlatformMac() then
    RHIBootTests.AddModules({
        "MetalRHI",
        "VulkanRHI",
    })
elseif IsPlatformWindows() then
    RHIBootTests.AddModules({
        "D3D12RHI",
        "VulkanRHI",
    })
end
