include "../../SetupScripts/Scripts/build_module.lua"

-- Engine Module

local engine_module = module_build_rules("Engine")
engine_module.use_precompiled_headers = true

engine_module.add_external_include_dirs
{
    create_external_thirdparty_path("imgui"),
    create_external_thirdparty_path("stb_image"),
    create_external_thirdparty_path("tinyobjloader"),
    create_external_thirdparty_path("tinyddsloader"),
    create_external_thirdparty_path("OpenFBX/src"),
}

engine_module.add_module_thirdparties
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "RendererCore",
    "ImGuiPlugin",
}

engine_module.add_link_libraries
{
    "ImGui",
    "tinyobjloader",
    "OpenFBX",
}
