include "BuildTool_Module.lua"

-- Single root for the glslang repo (moved under ThirdParty/glslang/glslang)
local GlslangRoot = CreateExternalThirdpartyPath("glslang/glslang")

-- Version & build_info.h generation (header-only, no Python required)
local function GlslangDeduceSoftwareVersion(Directory)
    local ChangesFile = JoinPath(Directory, "CHANGES.md")
    
    -- e.g. "14.3.0 2024-06-01" or "# 14.3.0 -beta 2024-06-01"
    local Pattern = "^#*%s*(%d+)%.(%d+)%.(%d+)%s*(-?[%w]*)%s*(%d%d%d%d%-%d%d%-%d%d)%s*"

    local File = io.open(ChangesFile, "r")
    if not File then
        LogError("Failed to open '%s' to deduce glslang version", ChangesFile)
        return nil
    end

    for Line in File:lines() do
        local Major, Minor, Patch, Flavor, Date = Line:match(Pattern)
        if Major then
            File:close()
            Flavor = Flavor:gsub("^%-", "") -- strip leading "-"
            return { Major = Major, Minor = Minor, Patch = Patch, Flavor = Flavor, Date = Date }
        end
    end

    File:close()

    LogError("No version number found in '%s'", ChangesFile)
    return nil
end

local function GlslangGenerateBuildTimeHeaders()
    LogInfo("Generating BuildTime Headers for 'glslang'")

    local TemplateFilePath = JoinPath(GlslangRoot, "build_info.h.tmpl")
    local OutputFileDir    = JoinPath(GlslangRoot, "glslang/include/glslang")
    local OutputFilePath   = JoinPath(OutputFileDir, "build_info.h")

    local Tmpl = io.open(TemplateFilePath, "r")
    if not Tmpl then
        LogError("Failed to open template file '%s'", TemplateFilePath)
        return
    end

    local Template = Tmpl:read("*a"); Tmpl:close()

    local Ver = GlslangDeduceSoftwareVersion(GlslangRoot)
    if not Ver then
        return
    end

    LogInfo("SoftwareVersion @major@ '%s'", Ver.Major)
    LogInfo("SoftwareVersion @minor@ '%s'", Ver.Minor)
    LogInfo("SoftwareVersion @patch@ '%s'", Ver.Patch)
    LogInfo("SoftwareVersion @flavor@ '%s'", Ver.Flavor)
    LogInfo("SoftwareVersion @date@ '%s'", Ver.Date)

    local Output = Template
    Output = Output:gsub("@major@", Ver.Major)
    Output = Output:gsub("@minor@", Ver.Minor)
    Output = Output:gsub("@patch@", Ver.Patch)
    Output = Output:gsub("@flavor@", Ver.Flavor)
    Output = Output:gsub("@date@", Ver.Date)

    if not os.isdir(OutputFileDir) then
        local Ok, Err = os.mkdir(OutputFileDir)
        if not Ok then
            LogError("Failed to create output directory '%s': %s", OutputFileDir, Err)
            return
        end
    else
        local Existing = io.open(OutputFilePath, "r")
        if Existing then
            local ExistingText = Existing:read("*a")
            Existing:close()
            if ExistingText == Output then
                LogInfo("'build_info.h' unchanged; skipping")
                return
            end
        else
            LogInfo("'build_info.h' does not exist yet, creating file...")
        end
    end

    local Out, WriteErr = io.open(OutputFilePath, "w")
    if not Out then
        LogError("Failed to open output file '%s': %s", OutputFilePath, WriteErr)
        return
    end

    Out:write(Output)
    Out:close()
    
    LogInfo("... finished creating 'build_info.h'")
end

-- Generate build-time header now (safe to call every run)
GlslangGenerateBuildTimeHeaders()

-- Common setup helper for all glslang sub-libraries
local function CommonSetup(Module)
    Module.bIsLibrary             = true
    Module.bIsDynamic             = false
    Module.bUsePrecompiledHeaders = false
    Module.bEnableRuntimeTypeInfo = false
    Module.bEnableEditAndContinue = false
    Module.bEnableIntrinsics      = true
    Module.bOptimizeDebugBuild    = true
    Module.bSilenceWarnings       = true
    Module.ExceptionHandling      = "On"
    Module.FloatingPoint          = "Fast"
    Module.VectorExtensions       = "Default"
    Module.Language               = "C++"
    Module.CppVersion             = "C++20"
    Module.SystemVersion          = "latest"
    Module.CharacterSet           = "Ascii"
    -- Module.Group                  = "ThirdParty/glslang"
    -- Module.OutputPath             = "ThirdParty/glslang/" .. Module.Name

    Module.AddFlags({
        "MultiProcessorCompile",
        "NoIncrementalLink",
    })

    if IsPlatformWindows() then
        Module.AddDefines({ "GLSLANG_OSINCLUDE_WIN32" })
        -- If you add BuildOptions support to BuildRules, you can also do:
        -- Module.AddBuildOptions({ "/Zc:threadSafeInit-" })
    else
        Module.AddDefines({ "GLSLANG_OSINCLUDE_UNIX" })
    end
end

-- GenericCodeGen
local GenericCodeGenModule = ModuleBuildRules("GenericCodeGen")
CommonSetup(GenericCodeGenModule)

GenericCodeGenModule.SetFiles({
    JoinPath(GlslangRoot, "glslang/GenericCodeGen/CodeGen.cpp"),
    JoinPath(GlslangRoot, "glslang/GenericCodeGen/Link.cpp"),
})

-- OSDependent
local OSDependentModule = ModuleBuildRules("OSDependent")
CommonSetup(OSDependentModule)

OSDependentModule.SetFiles({
    JoinPath(GlslangRoot, "glslang/OSDependent/osinclude.h"),
})

if IsPlatformWindows() then
    OSDependentModule.SetFiles({
        JoinPath(GlslangRoot, "glslang/OSDependent/Windows/ossource.cpp"),
    })
else
    OSDependentModule.SetFiles({
        JoinPath(GlslangRoot, "glslang/OSDependent/Unix/ossource.cpp"),
    })
end

-- MachineIndependent
local MachineIndependentModule = ModuleBuildRules("MachineIndependent")
CommonSetup(MachineIndependentModule)

MachineIndependentModule.AddExternalIncludeDirs({
    GlslangRoot,
    JoinPath(GlslangRoot, "glslang/include"),
})

MachineIndependentModule.SetFiles({
    -- Generated/parser bits
    JoinPath(GlslangRoot, "glslang/MachineIndependent/glslang.y"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/glslang_tab.cpp"),

    -- Sources
    JoinPath(GlslangRoot, "glslang/MachineIndependent/attribute.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Constant.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/InfoSink.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Initialize.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/intermOut.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/IntermTraverse.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/iomapper.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Intermediate.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/limits.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/linkValidate.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/parseConst.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/ParseContextBase.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/ParseHelper.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/PoolAlloc.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/propagateNoContraction.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/reflection.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/RemoveTree.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Scan.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/ShaderLang.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/SpirvIntrinsics.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/SymbolTable.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Versions.cpp"),

    -- Preprocessor
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/Pp.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/PpAtom.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/PpContext.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/PpScanner.cpp"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/PpTokens.cpp"),

    -- Headers
    JoinPath(GlslangRoot, "glslang/MachineIndependent/glslang_tab.cpp.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/gl_types.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/attribute.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Initialize.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/iomapper.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/LiveTraverser.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/localintermediate.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/ParseHelper.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/parseVersions.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/propagateNoContraction.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/reflection.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/RemoveTree.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Scan.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/ScanContext.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/span.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/SymbolTable.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/Versions.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/PpContext.h"),
    JoinPath(GlslangRoot, "glslang/MachineIndependent/preprocessor/PpTokens.h"),
})

MachineIndependentModule.AddModules({
    "OSDependent",
    "GenericCodeGen",
})

-- glslang (C interface module)
local GlslangModule = ModuleBuildRules("glslang")
CommonSetup(GlslangModule)

GlslangModule.AddExternalIncludeDirs({
    GlslangRoot,
})

GlslangModule.SetFiles({
    -- C interface
    JoinPath(GlslangRoot, "glslang/CInterface/glslang_c_interface.cpp"),

    -- Public headers (IDE visibility)
    JoinPath(GlslangRoot, "glslang/Public/ShaderLang.h"),
    JoinPath(GlslangRoot, "glslang/Include/arrays.h"),
    JoinPath(GlslangRoot, "glslang/Include/BaseTypes.h"),
    JoinPath(GlslangRoot, "glslang/Include/Common.h"),
    JoinPath(GlslangRoot, "glslang/Include/ConstantUnion.h"),
    JoinPath(GlslangRoot, "glslang/Include/glslang_c_interface.h"),
    JoinPath(GlslangRoot, "glslang/Include/glslang_c_shader_types.h"),
    JoinPath(GlslangRoot, "glslang/Include/InfoSink.h"),
    JoinPath(GlslangRoot, "glslang/Include/InitializeGlobals.h"),
    JoinPath(GlslangRoot, "glslang/Include/intermediate.h"),
    JoinPath(GlslangRoot, "glslang/Include/PoolAlloc.h"),
    JoinPath(GlslangRoot, "glslang/Include/ResourceLimits.h"),
    JoinPath(GlslangRoot, "glslang/Include/ShHandle.h"),
    JoinPath(GlslangRoot, "glslang/Include/SpirvIntrinsics.h"),
    JoinPath(GlslangRoot, "glslang/Include/Types.h"),
})

GlslangModule.AddModules({
    "OSDependent",
    "MachineIndependent",
})

-- glslang-default-resource-limits
local GlslangDefaultResourceLimitsModule = ModuleBuildRules("glslang-default-resource-limits")
CommonSetup(GlslangDefaultResourceLimitsModule)

GlslangDefaultResourceLimitsModule.AddExternalIncludeDirs({
    GlslangRoot,
})

GlslangDefaultResourceLimitsModule.SetFiles({
    JoinPath(GlslangRoot, "glslang/ResourceLimits/ResourceLimits.cpp"),
    JoinPath(GlslangRoot, "glslang/ResourceLimits/resource_limits_c.cpp"),
    JoinPath(GlslangRoot, "glslang/Public/ResourceLimits.h"),
    JoinPath(GlslangRoot, "glslang/Public/resource_limits_c.h"),
})

-- SPIRV
local SPIRVModule = ModuleBuildRules("SPIRV")
CommonSetup(SPIRVModule)

SPIRVModule.AddExternalIncludeDirs({
    GlslangRoot,
    JoinPath(GlslangRoot, "glslang/include"),
})

SPIRVModule.SetFiles({
    -- Sources
    JoinPath(GlslangRoot, "SPIRV/GlslangToSpv.cpp"),
    JoinPath(GlslangRoot, "SPIRV/InReadableOrder.cpp"),
    JoinPath(GlslangRoot, "SPIRV/Logger.cpp"),
    JoinPath(GlslangRoot, "SPIRV/SpvBuilder.cpp"),
    JoinPath(GlslangRoot, "SPIRV/SpvPostProcess.cpp"),
    JoinPath(GlslangRoot, "SPIRV/doc.cpp"),
    JoinPath(GlslangRoot, "SPIRV/SpvTools.cpp"),
    JoinPath(GlslangRoot, "SPIRV/disassemble.cpp"),
    JoinPath(GlslangRoot, "SPIRV/CInterface/spirv_c_interface.cpp"),

    -- Headers (IDE visibility)
    JoinPath(GlslangRoot, "SPIRV/bitutils.h"),
    JoinPath(GlslangRoot, "SPIRV/spirv.hpp"),
    JoinPath(GlslangRoot, "SPIRV/GLSL.std.450.h"),
    JoinPath(GlslangRoot, "SPIRV/GLSL.ext.EXT.h"),
    JoinPath(GlslangRoot, "SPIRV/GLSL.ext.KHR.h"),
    JoinPath(GlslangRoot, "SPIRV/GlslangToSpv.h"),
    JoinPath(GlslangRoot, "SPIRV/hex_float.h"),
    JoinPath(GlslangRoot, "SPIRV/Logger.h"),
    JoinPath(GlslangRoot, "SPIRV/SpvBuilder.h"),
    JoinPath(GlslangRoot, "SPIRV/spvIR.h"),
    JoinPath(GlslangRoot, "SPIRV/doc.h"),
    JoinPath(GlslangRoot, "SPIRV/SpvTools.h"),
    JoinPath(GlslangRoot, "SPIRV/disassemble.h"),
    JoinPath(GlslangRoot, "SPIRV/GLSL.ext.AMD.h"),
    JoinPath(GlslangRoot, "SPIRV/GLSL.ext.NV.h"),
    JoinPath(GlslangRoot, "SPIRV/GLSL.ext.ARM.h"),
    JoinPath(GlslangRoot, "SPIRV/NonSemanticDebugPrintf.h"),
    JoinPath(GlslangRoot, "SPIRV/NonSemanticShaderDebugInfo100.h"),
})

SPIRVModule.AddModules({
    "MachineIndependent",
})

-- SPVRemapper
local SPVRemapperModule = ModuleBuildRules("SPVRemapper")
CommonSetup(SPVRemapperModule)

SPVRemapperModule.SetFiles({
    JoinPath(GlslangRoot, "SPIRV/SPVRemapper.cpp"),
    JoinPath(GlslangRoot, "SPIRV/doc.cpp"),
    JoinPath(GlslangRoot, "SPIRV/SPVRemapper.h"),
    JoinPath(GlslangRoot, "SPIRV/doc.h"),
})
