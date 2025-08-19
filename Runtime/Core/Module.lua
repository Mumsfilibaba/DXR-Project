include "../../SetupScripts/Scripts/build_module.lua"

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

-- Add project name to the core module
local base_generate = core_module.generate
function core_module.generate()
    if core_module.workspace == nil then
        log_error("Workspace cannot be nil when generating Rule")
        return
    end

    local target_name = core_module.workspace.get_current_target_name()
    core_module.add_defines{ 'PROJECT_NAME="' .. target_name .. '"' }

    local unix_project_path = path.translate(join_path(core_module.workspace.get_engine_path(), target_name), "/")
    local project_location = 'PROJECT_LOCATION="' .. unix_project_path .. '"'
    core_module.add_defines{ project_location }

    base_generate()
end