include "BuildTool.lua"

-- RHI Module

local RhiModule = ModuleBuildRules("RHI")
RhiModule.bUsePrecompiledHeaders = true

RhiModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("DXC/include"),
})

RhiModule.AddModules({
    "Core",
    "CoreApplication",
    "SPIRV-Cross",
    "SPIRV",
    "MachineIndependent",
    "SPVRemapper",
    "glslang-default-resource-limits",
    "glslang",
})

-- Deploy DXC from the thirdparties folder
if IsPlatformWindows() then
    local Dest = RhiModule.GetTargetFolderPath()
    RhiModule.AddPostBuildCommands({
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxil.dll"), Dest),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxcompiler.dll"), Dest),
    })
elseif IsPlatformMac() then
    -- Copied into Contents/Frameworks by the executable that ends up using this module
    RhiModule.AddExtraRuntimeLibraries({
        CreateExternalThirdpartyPath("DXC/bin/libdxcompiler.dylib"),
    })
end
