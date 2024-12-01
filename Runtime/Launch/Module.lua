include "../../BuildScripts/Scripts/build_module.lua"

-- Launch Module

local launch_module = module_build_rules("Launch")
launch_module.is_dynamic = false

launch_module.add_module_dependencies
{
    "Core",
    "CoreApplication",
    "Application",
    "RHI",
    "Renderer",
    "RendererCore",
    "Engine",
    "Project",
}
