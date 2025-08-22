include "BuildTool_Module.lua"

-- ImGuiPlugin Module

local ImGuiPluginModule = ModuleBuildRules("ImGuiPlugin")

ImGuiPluginModule.AddExternalIncludeDirs({
    CreateExternalThirdpartyPath("imgui"),
})

ImGuiPluginModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
})

ImGuiPluginModule.AddLinkLibraries({
    "ImGui",
})
