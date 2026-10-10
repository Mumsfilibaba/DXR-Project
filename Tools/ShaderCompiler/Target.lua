include "BuildTool.lua"

-- The ShaderCompiler module already owns the project name, so only the executable is called ShaderCompiler
local ShaderCompilerTool = TargetBuildRules("ShaderCompilerTool")
ShaderCompilerTool.TargetType = ETargetType.Program
ShaderCompilerTool.Kind       = IsPlatformMac() and "WindowedApp" or "ConsoleApp"
ShaderCompilerTool.OutputName = "ShaderCompiler"

-- Its own folder, because cleaning a project deletes <OutDir>/<TargetName>.pdb, which next to the module would be
-- the debug information ShaderCompiler.lib refers to
ShaderCompilerTool.OutputPathOverride = "ShaderCompiler"

ShaderCompilerTool.AddModules({
    "Core",
    "CoreApplication",
    "LaunchProgram",
    "RHI",
    "ShaderCore",
    "ShaderCompiler",
})

-- The module only deploys DXC to the shared output folder
if IsPlatformWindows() then
    local Dest = ShaderCompilerTool.GetTargetFolderPath()
    ShaderCompilerTool.AddPostBuildCommands({
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxil.dll"), Dest),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxcompiler.dll"), Dest),
    })
end
