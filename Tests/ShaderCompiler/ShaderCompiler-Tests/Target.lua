include "BuildTool.lua"

-- ShaderCompiler Tests
local ShaderCompilerTests = TargetBuildRules("ShaderCompiler-Tests")
ShaderCompilerTests.TargetType = ETargetType.Program
ShaderCompilerTests.Kind       = "ConsoleApp"

ShaderCompilerTests.AddModules({
    "Core",
    "RHI",
    "ShaderCompiler",
    "TestCommon",
})
