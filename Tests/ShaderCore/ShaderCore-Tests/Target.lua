include "BuildTool.lua"

-- ShaderCore Tests
local ShaderCoreTests = TargetBuildRules("ShaderCore-Tests")
ShaderCoreTests.TargetType = ETargetType.Program
ShaderCoreTests.Kind       = "ConsoleApp"

ShaderCoreTests.AddModules({
    "Core",
    "ShaderCore",
    "TestCommon",
})
