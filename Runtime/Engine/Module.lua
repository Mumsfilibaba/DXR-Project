include "../../BuildScripts/Scripts/build_module.lua"

-- Engine Module

local engine_module = module_build_rules("Engine")
engine_module.use_precompiled_headers = true

engine_module.add_external_include_dirs
{
    create_external_dependency_path("imgui"),
    create_external_dependency_path("stb_image"),
    create_external_dependency_path("tinyobjloader"),
    create_external_dependency_path("tinyddsloader"),
    create_external_dependency_path("OpenFBX/src"),
}

engine_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
    "Project",
    "ImGuiPlugin",
}

engine_module.add_link_libraries
{
    "ImGui",
    "tinyobjloader",
    "OpenFBX",
}
