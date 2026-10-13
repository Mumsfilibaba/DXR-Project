include "BuildTool.lua"

-- Engine Tests
local EngineTests = TargetBuildRules("Engine-Tests")
EngineTests.TargetType = ETargetType.Program
EngineTests.Kind       = "ConsoleApp"

EngineTests.AddModules({
    "Core",
    "AssetCore",
    "Engine",
    "TestCommon",
})
