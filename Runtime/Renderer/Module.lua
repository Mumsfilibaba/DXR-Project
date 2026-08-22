include "BuildTool.lua"

-- Renderer Module

local RendererModule = ModuleBuildRules("Renderer")
RendererModule.bUsePrecompiledHeaders = true

RendererModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "ApplicationRenderer",
    "RHI",
    "Engine",
    "RendererCore",
    "ImGui",
    "ImGuiPlugin",
})
