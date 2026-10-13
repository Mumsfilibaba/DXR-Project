include "BuildTool.lua"

-- AssetCore Tests
local AssetCoreTests = TargetBuildRules("AssetCore-Tests")
AssetCoreTests.TargetType = ETargetType.Program
AssetCoreTests.Kind       = "ConsoleApp"

AssetCoreTests.AddModules({
    "Core",
    "AssetCore",
    "TestCommon",
})
