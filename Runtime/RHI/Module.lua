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

-- Copy dynamic libraries from thirdparties folder
local Dest = RhiModule.GetTargetFolderPath()
if IsPlatformWindows() then
    RhiModule.AddPostBuildCommands({
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxil.dll"), Dest),
        ('copy /Y "%s" "%s"\\'):format(CreateExternalThirdpartyPath("DXC/bin/dxcompiler.dll"), Dest),
    })
elseif IsPlatformMac() then
    RhiModule.AddPostBuildCommands({
        ('cp -f "%s" "%s"'):format(CreateExternalThirdpartyPath("DXC/bin/libdxcompiler.dylib"), Dest),
    })
end
