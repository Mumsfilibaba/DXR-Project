include "../../BuildScripts/Scripts/build_module.lua"

-- Renderer Module

local renderer_module = module_build_rules("Renderer")
renderer_module.use_precompiled_headers = true

renderer_module.add_system_includes
{
    create_external_dependency_path("imgui"),
}

renderer_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Engine",
    "RendererCore",
    "ImGuiPlugin",
}

renderer_module.add_link_libraries
{
    "ImGui",
}
