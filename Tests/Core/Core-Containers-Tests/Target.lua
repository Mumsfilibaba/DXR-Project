include "BuildTool.lua"

-- Core Container Tests
local ContainersTests = TargetBuildRules("Core-Containers-Tests")
ContainersTests.TargetType = ETargetType.Program
ContainersTests.Kind       = "ConsoleApp"

-- The container tests compare behavior against the standard library equivalents
ContainersTests.ExceptionHandling = "On"

ContainersTests.AddModules({
    "Core",
    "TestCommon",
})
