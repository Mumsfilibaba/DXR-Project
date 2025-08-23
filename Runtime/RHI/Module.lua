include "BuildTool_Module.lua"

-- RHI Module

local RhiModule = ModuleBuildRules("RHI")
RhiModule.bUsePrecompiledHeaders = true

RhiModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("DXC/include"),
    CreateExternalThirdpartyPath("SPIRV-Cross"),
    CreateExternalThirdpartyPath("glslang"),
})

RhiModule.AddModules({
    "Core",
    "CoreApplication",
})

RhiModule.AddLinkLibraries({
    "SPIRV",
    "MachineIndependent",
    "SPVRemapper",
    "glslang-default-resource-limits",
    "glslang",
    "SPIRV-Cross",
})

-- Copy dynamic libraries from thirdparties folder
if IsPlatformWindows() then
    RhiModule.AddPostBuildCommands({
        "copy " .. CreateExternalThirdpartyPath("DXC/bin/dxil.dll") .. " " .. RhiModule.GetTargetFolderPath(),
        "copy " .. CreateExternalThirdpartyPath("DXC/bin/dxcompiler.dll") .. " " .. RhiModule.GetTargetFolderPath(),
    })
elseif IsPlatformMac() then
    RhiModule.AddPostBuildCommands({
        "cp " .. CreateExternalThirdpartyPath("DXC/bin/libdxcompiler.dylib") .. " " .. RhiModule.GetTargetFolderPath(),
    })
end
