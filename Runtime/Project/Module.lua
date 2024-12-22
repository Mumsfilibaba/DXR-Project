include "../../BuildScripts/Scripts/build_module.lua"

-- Project Module

local project_module = module_build_rules("Project")

project_module.add_module_dependencies
{
    "Core",
}

-- Base generate (generates project files)
local base_generate = project_module.generate

function project_module.generate()
    if project_module.workspace == nil then
        log_error("Workspace cannot be nil when generating Rule")
        return
    end

    local target_name = project_module.workspace.get_current_target_name()
    project_module.add_defines{ 'PROJECT_NAME="' .. target_name .. '"' }
    project_module.add_defines{ 'PROJECT_LOCATION="' .. join_path(project_module.workspace.get_engine_path(), target_name) .. '"' }

    base_generate()
end
