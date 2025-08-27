include "BuildTool_Module.lua"

-- Renderer Module

local RendererModule = ModuleBuildRules("Renderer")
RendererModule.bUsePrecompiledHeaders = true

RendererModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Engine",
    "RendererCore",
    "ImGui",
    "ImGuiPlugin",
})
