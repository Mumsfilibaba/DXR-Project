include "BuildTool_Module.lua"

-- Engine Module

local EngineModule = ModuleBuildRules("Engine")
EngineModule.bUsePrecompiledHeaders = true

EngineModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("stb_image"),
    CreateExternalThirdpartyPath("tinyobjloader"),
    CreateExternalThirdpartyPath("tinyddsloader"),
    CreateExternalThirdpartyPath("OpenFBX/src"),
})

EngineModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
    "ImGui",
    "ImGuiPlugin",
})

EngineModule.AddLinkLibraries({
    "tinyobjloader",
    "OpenFBX",
})
