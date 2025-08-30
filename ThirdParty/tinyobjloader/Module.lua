include "BuildTool_Module.lua"

-- ThirdParty tinyobjloader Module
local TinyObj = ModuleBuildRules("tinyobjloader")
TinyObj.bIsLibrary             = true
TinyObj.bIsDynamic             = false
TinyObj.bUsePrecompiledHeaders = false
TinyObj.bEnableRuntimeTypeInfo = false
TinyObj.bEnableEditAndContinue = false
TinyObj.bEnableIntrinsics      = true
TinyObj.bOptimizeDebugBuild    = true
TinyObj.bSilenceWarnings       = true
TinyObj.ExceptionHandling      = "Off"
TinyObj.FloatingPoint          = "Fast"
TinyObj.VectorExtensions       = "Default"
TinyObj.Language               = "C++"
TinyObj.CppVersion             = "C++20"
TinyObj.SystemVersion          = "latest"
TinyObj.CharacterSet           = "Ascii"

-- TinyObj.Group      = "ThirdParty"
-- TinyObj.OutputPath = "ThirdParty"

TinyObj.AddFlags({
    "MultiProcessorCompile",
    "NoIncrementalLink",
})

-- Add the correct folder as an include dir to make includes simpler (#include <tiny_obj_loader.h> instead of #include <tinyobjloader/tiny_obj_loader.h>)
TinyObj.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("tinyobjloader/tinyobjloader/"),
})

-- Sources
TinyObj.SetFiles({
    CreateExternalThirdpartyPath("tinyobjloader/tinyobjloader/tiny_obj_loader.h"),
    CreateExternalThirdpartyPath("tinyobjloader/tinyobjloader/tiny_obj_loader.cc"),
})
