include "BuildTool_Module.lua"

-- ThirdParty SPIRV-Cross Module
local SpirvCross = ModuleBuildRules("SPIRV-Cross")
SpirvCross.bIsLibrary             = true
SpirvCross.bIsDynamic             = false
SpirvCross.bUsePrecompiledHeaders = false
SpirvCross.bEnableRuntimeTypeInfo = false
SpirvCross.bEnableEditAndContinue = false
SpirvCross.bEnableIntrinsics      = true
SpirvCross.bOptimizeDebugBuild    = true
SpirvCross.bSilenceWarnings       = true
SpirvCross.ExceptionHandling      = "On"
SpirvCross.FloatingPoint          = "Fast"
SpirvCross.VectorExtensions       = "Default"
SpirvCross.Language               = "C++"
SpirvCross.CppVersion             = "C++20"
SpirvCross.SystemVersion          = "latest"
SpirvCross.CharacterSet           = "Ascii"

SpirvCross.AddFlags({
    "MultiProcessorCompile",
    "NoIncrementalLink",
})

-- Public C API toggles used by SPIRV-Cross
SpirvCross.AddDefines({
    "SPIRV_CROSS_C_API_MSL=(1)",
    "SPIRV_CROSS_C_API_HLSL=(1)",
    "SPIRV_CROSS_C_API_GLSL=(1)",
})

-- Sources/headers
SpirvCross.AddFiles({
    -- Public headers / C interface
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

    -- Sources
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