include "BuildTool.lua"

-- Core Template Tests
local TemplatesTests = TargetBuildRules("Core-Templates-Tests")
TemplatesTests.TargetType = ETargetType.Program
TemplatesTests.Kind       = "ConsoleApp"

-- The template tests compare behavior against the standard library equivalents
TemplatesTests.ExceptionHandling = "On"

TemplatesTests.AddModules({
    "Core",
    "TestCommon",
})
