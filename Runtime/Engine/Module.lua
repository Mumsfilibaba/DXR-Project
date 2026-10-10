include "BuildTool.lua"

-- Engine Module

local EngineModule = ModuleBuildRules("Engine")
EngineModule.bUsePrecompiledHeaders = true

EngineModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("stb_image"),
    CreateExternalThirdpartyPath("tinyddsloader"),
})

EngineModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
    "ShaderCompiler",
    "ImGui",
    "ImGuiPlugin",
    "OpenFBX",
    "tinyobjloader",
    "RemoteConsole",
})
