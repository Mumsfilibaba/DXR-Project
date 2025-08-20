include "../../SetupScripts/Scripts/BuildTool_Module.lua"

-- Renderer Module

local RendererModule = ModuleBuildRules("Renderer")
RendererModule.bUsePrecompiledHeaders = true

RendererModule.AddExternalIncludeDirs
{
    CreateExternalThirdpartyPath("imgui"),
}

RendererModule.AddModuleThirdparties
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Engine",
    "RendererCore",
    "ImGuiPlugin",
}

RendererModule.AddLinkLibraries
{
    "ImGui",
}
