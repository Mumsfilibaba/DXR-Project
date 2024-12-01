include "../../BuildScripts/Scripts/build_module.lua"

-- CoreApplication Module

local core_application_module = module_build_rules("CoreApplication")
core_application_module.use_precompiled_headers = true

core_application_module.add_module_dependencies{ "Core" }

if is_platform_mac() then
    core_application_module.add_frameworks
    {
        "Cocoa",
        "AppKit",
        "IOKit",
        "GameController",
    }
elseif is_platform_windows() then
    core_application_module.add_link_libraries{ "Shcore.lib" }
end
