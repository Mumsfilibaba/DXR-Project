include "BuildTool.lua"

-- Core Container Tests
local ContainersTests = TargetBuildRules("Core-Containers-Tests")
ContainersTests.TargetType = ETargetType.Program
ContainersTests.Kind       = "ConsoleApp"

-- The container tests compare behavior against the standard library equivalents
ContainersTests.ExceptionHandling = "On"

-- DynamicCastSharedPtr resolves to a real dynamic_cast, which is unpredictable under /GR-
ContainersTests.bEnableRuntimeTypeInfo = true

ContainersTests.AddModules({
    "Core",
    "TestCommon",
})
