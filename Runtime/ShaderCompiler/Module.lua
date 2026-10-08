include "BuildTool.lua"

-- ShaderCompiler Module

local ShaderCompilerModule = ModuleBuildRules("ShaderCompiler")
ShaderCompilerModule.bUsePrecompiledHeaders = true

ShaderCompilerModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("DXC/include"),
})

ShaderCompilerModule.AddModules({
    "Core",
    "CoreApplication",
    "ShaderCore",
    "SPIRV-Cross",
})

-- Deploy DXC from the thirdparties folder
if IsPlatformWindows() then
    local Dest = ShaderCompilerModule.GetTargetFolderPath()
    ShaderCompilerModule.AddPostBuildCommands({
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxil.dll"), Dest),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxcompiler.dll"), Dest),
    })
elseif IsPlatformMac() then
    -- Copied into Contents/Frameworks by the executable that ends up using this module
    ShaderCompilerModule.AddExtraRuntimeLibraries({
        CreateExternalThirdpartyPath("DXC/bin/libdxcompiler.dylib"),
    })
end
