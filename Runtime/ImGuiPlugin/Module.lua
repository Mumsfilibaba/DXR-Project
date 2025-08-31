include "BuildTool.lua"

-- ImGuiPlugin Module

local ImGuiPluginModule = ModuleBuildRules("ImGuiPlugin")

ImGuiPluginModule.AddModules({
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
    "ImGui",
})
