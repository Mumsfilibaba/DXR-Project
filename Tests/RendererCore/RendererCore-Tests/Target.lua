include "BuildTool.lua"

-- RendererCore Tests
local RendererCoreTests = TargetBuildRules("RendererCore-Tests")
RendererCoreTests.TargetType = ETargetType.Program
RendererCoreTests.Kind       = "ConsoleApp"

-- NullRHI is linked rather than loaded at runtime, since these tests bring up a real device
RendererCoreTests.AddModules({
    "Core",
    "RHI",
    "RendererCore",
    "NullRHI",
    "TestCommon",
})
