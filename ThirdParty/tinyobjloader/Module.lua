include "BuildTool.lua"

-- ThirdParty tinyobjloader Module
local TinyObjModule = ModuleBuildRules("tinyobjloader")
TinyObjModule.bIsLibrary             = true
TinyObjModule.bIsDynamic             = false
TinyObjModule.bUsePrecompiledHeaders = false
TinyObjModule.bEnableRuntimeTypeInfo = false
TinyObjModule.bEnableEditAndContinue = false
TinyObjModule.bEnableIntrinsics      = true
TinyObjModule.bOptimizeDebugBuild    = true
TinyObjModule.bSilenceWarnings       = true
TinyObjModule.ExceptionHandling      = "Off"
TinyObjModule.FloatingPoint          = "Fast"
TinyObjModule.VectorExtensions       = "Default"
TinyObjModule.Language               = "C++"
TinyObjModule.CppVersion             = "C++20"
TinyObjModule.SystemVersion          = "latest"
TinyObjModule.CharacterSet           = "Ascii"

TinyObjModule.AddFlags({
    "MultiProcessorCompile",
    "NoIncrementalLink",
})

-- Add the correct folder as an include dir to make includes simpler (#include <tiny_obj_loader.h> instead of #include <tinyobjloader/tiny_obj_loader.h>)
TinyObjModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("tinyobjloader/tinyobjloader/"),
})

-- Sources
TinyObjModule.SetFiles({
    CreateExternalThirdpartyPath("tinyobjloader/tinyobjloader/tiny_obj_loader.h"),
    CreateExternalThirdpartyPath("tinyobjloader/tinyobjloader/tiny_obj_loader.cc"),
})
