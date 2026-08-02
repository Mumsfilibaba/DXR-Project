include "BuildTool.lua"

-- Core Tests
local CoreTests = TargetBuildRules("Core-Tests")
CoreTests.TargetType = ETargetType.Program
CoreTests.Kind       = "ConsoleApp"

CoreTests.AddModules({
    "Core",
    "TestCommon",
})
