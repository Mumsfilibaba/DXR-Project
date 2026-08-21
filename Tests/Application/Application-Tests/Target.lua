include "BuildTool.lua"

-- Application Tests
local ApplicationTests = TargetBuildRules("Application-Tests")
ApplicationTests.TargetType = ETargetType.Program
ApplicationTests.Kind       = "ConsoleApp"

-- The element and console code is exercised headlessly, so no RHI device or platform window is created
ApplicationTests.AddModules({
    "Core",
    "CoreApplication",
    "RHI",
    "Application",
    "TestCommon",
})
