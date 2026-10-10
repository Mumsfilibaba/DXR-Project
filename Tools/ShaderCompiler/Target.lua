include "BuildTool.lua"

-- The ShaderCompiler module already owns the project name, so only the executable is called ShaderCompiler
local ShaderCompilerTool = TargetBuildRules("ShaderCompilerTool")
ShaderCompilerTool.TargetType = ETargetType.Program
ShaderCompilerTool.Kind       = IsPlatformMac() and "WindowedApp" or "ConsoleApp"
ShaderCompilerTool.OutputName = "ShaderCompiler"

ShaderCompilerTool.AddModules({
    "Core",
    "CoreApplication",
    "LaunchProgram",
    "RHI",
    "ShaderCore",
    "ShaderCompiler",
})
