include "../../BuildScripts/Scripts/build_module.lua"

-- Core Module

local core_module = module_build_rules("Core")
core_module.use_precompiled_headers = true

if is_platform_mac() then
    core_module.add_frameworks(
    {
        "AppKit",
    })
elseif is_platform_windows() then
    core_module.add_link_libraries(
    {
        "Dbghelp.lib",
        "shlwapi.lib",
    })
end
