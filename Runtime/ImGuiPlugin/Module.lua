include "../../BuildScripts/Scripts/build_module.lua"

-- ImGuiPlugin Module

local imgui_plugin_module = module_build_rules("ImGuiPlugin")

imgui_plugin_module.add_external_include_dirs
{
    create_external_dependency_path("imgui"),
}

imgui_plugin_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
}

imgui_plugin_module.add_link_libraries
{
    "ImGui",
}
