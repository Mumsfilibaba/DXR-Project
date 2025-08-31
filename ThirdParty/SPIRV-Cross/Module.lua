include "BuildTool.lua"

-- ThirdParty SPIRV-Cross Module
local SpirvCrossModule = ModuleBuildRules("SPIRV-Cross")
SpirvCrossModule.bIsLibrary             = true
SpirvCrossModule.bIsDynamic             = false
SpirvCrossModule.bUsePrecompiledHeaders = false
SpirvCrossModule.bEnableRuntimeTypeInfo = false
SpirvCrossModule.bEnableEditAndContinue = false
SpirvCrossModule.bEnableIntrinsics      = true
SpirvCrossModule.bOptimizeDebugBuild    = true
SpirvCrossModule.bSilenceWarnings       = true
SpirvCrossModule.ExceptionHandling      = "On"
SpirvCrossModule.FloatingPoint          = "Fast"
SpirvCrossModule.VectorExtensions       = "Default"
SpirvCrossModule.Language               = "C++"
SpirvCrossModule.CppVersion             = "C++20"
SpirvCrossModule.SystemVersion          = "latest"
SpirvCrossModule.CharacterSet           = "Ascii"

SpirvCrossModule.AddFlags({
    "MultiProcessorCompile",
    "NoIncrementalLink",
})

-- Add the correct folder as an include dir to make includes simpler (#include <spirv_cross_c.h> instead of #include <SPIRV-Cross/spirv_cross_c.h>)
SpirvCrossModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/"),
})

-- Public C API toggles used by SPIRV-Cross
SpirvCrossModule.AddDefines({
    "SPIRV_CROSS_C_API_MSL=(1)",
    "SPIRV_CROSS_C_API_HLSL=(1)",
    "SPIRV_CROSS_C_API_GLSL=(1)",
})

-- Source files
SpirvCrossModule.SetFiles({
    -- C interface
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/GLSL.std.450.h"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv.h"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_c.h"),

    -- C++ headers
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cfg.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_common.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cpp.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_containers.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_error_handling.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_parsed_ir.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_util.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_glsl.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_hlsl.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_msl.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_parser.hpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_reflect.hpp"),

    -- C++ Sources
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cfg.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cpp.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_c.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_parsed_ir.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_cross_util.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_glsl.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_hlsl.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_msl.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_parser.cpp"),
    CreateExternalThirdpartyPath("SPIRV-Cross/SPIRV-Cross/spirv_reflect.cpp"),
})