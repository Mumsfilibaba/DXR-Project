include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- RHI Module

local RhiModule = ModuleBuildRules("RHI")
RhiModule.bUsePrecompiledHeaders = true

RhiModule.AddExternalIncludeDirs
({
    CreateExternalThirdpartyPath("DXC/include"),
    CreateExternalThirdpartyPath("SPIRV-Cross"),
    CreateExternalThirdpartyPath("glslang"),
})

RhiModule.AddModules
({
    "Core",
    "CoreApplication",
})

RhiModule.AddLinkLibraries
({
    "SPIRV",
    "MachineIndependent",
    "SPVRemapper",
    "glslang-default-resource-limits",
    "glslang",
    "SPIRV-Cross",
})
