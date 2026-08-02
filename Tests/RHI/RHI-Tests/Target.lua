include "BuildTool.lua"

-- RHI Tests
local RHITests = TargetBuildRules("RHI-Tests")
RHITests.TargetType = ETargetType.Program
RHITests.Kind       = "ConsoleApp"

RHITests.AddModules({
    "Core",
    "RHI",
    "TestCommon",
})
