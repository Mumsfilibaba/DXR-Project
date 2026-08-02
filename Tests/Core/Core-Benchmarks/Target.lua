include "BuildTool.lua"

-- Core Benchmarks
local CoreBenchmarks = TargetBuildRules("Core-Benchmarks")
CoreBenchmarks.TargetType = ETargetType.Program
CoreBenchmarks.Kind       = "ConsoleApp"

-- The benchmarks measure against the standard library equivalents
CoreBenchmarks.ExceptionHandling = "On"

CoreBenchmarks.AddModules({
    "Core",
    "TestCommon",
})
