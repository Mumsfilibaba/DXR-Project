include "BuildTool_Module.lua"

-- Renderer Module

local RendererModule = ModuleBuildRules("Renderer")
RendererModule.bUsePrecompiledHeaders = true

RendererModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("ImGui/imgui"),
})

RendererModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Engine",
    "RendererCore",
    "ImGuiPlugin",
})

RendererModule.AddLinkLibraries({
    "ImGui",
})
