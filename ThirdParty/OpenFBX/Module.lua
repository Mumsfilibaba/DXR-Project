include "BuildTool_Module.lua"

-- ThirdParty OpenFBX Module
local OpenFBXModule = ModuleBuildRules("OpenFBX")
OpenFBXModule.bIsLibrary             = true
OpenFBXModule.bIsDynamic             = false
OpenFBXModule.bRuntimeLinking        = false
OpenFBXModule.bUsePrecompiledHeaders = false
OpenFBXModule.bEnableRuntimeTypeInfo = false
OpenFBXModule.bEnableEditAndContinue = false
OpenFBXModule.bEnableIntrinsics      = true
OpenFBXModule.bOptimizeDebugBuild    = true
OpenFBXModule.bSilenceWarnings       = true
OpenFBXModule.ExceptionHandling      = "Off"
OpenFBXModule.FloatingPoint          = "Fast"
OpenFBXModule.VectorExtensions       = "Default"
OpenFBXModule.Language               = "C++"
OpenFBXModule.CppVersion             = "C++20"
OpenFBXModule.SystemVersion          = "latest"
OpenFBXModule.CharacterSet           = "Ascii"

OpenFBXModule.AddFlags({
    "MultiProcessorCompile",
    "NoIncrementalLink",
})

-- Add the correct folder as an include dir to make includes simpler (#include <ofbx.h> instead of #include <OpenFBX/src/ofbx.h>)
OpenFBXModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("OpenFBX/OpenFBX/src"),
})

-- Source files
OpenFBXModule.AddFiles({
    CreateExternalThirdpartyPath("OpenFBX/OpenFBX/src/ofbx.h"),
    CreateExternalThirdpartyPath("OpenFBX/OpenFBX/src/ofbx.cpp"),
    CreateExternalThirdpartyPath("OpenFBX/OpenFBX/src/libdeflate.h"),
    CreateExternalThirdpartyPath("OpenFBX/OpenFBX/src/libdeflate.c"),
})
