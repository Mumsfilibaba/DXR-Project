include "build_module.lua"

-- Target types
ETargetType = 
{
    Client      = 1,
    WindowedApp = 2,
    ConsoleApp  = 3,
}

-- Target build rules
function target_build_rules(name, workspace)
    -- Needs to have a valid module name
    if name == nil then
        log_error("BuildRule failed due to invalid name")
        return nil
    end
    
    -- Needs to have a valid workspace
    if workspace == nil then
        log_error("Workspace cannot be nil")
        return nil
    end

    log_highlight("Creating Target '%s'", name)

    -- Initialize parent class
    local self = build_rules(name)
    if self == nil then
        log_error("Failed to create BuildRule")
        return nil
    end

    self.workspace = workspace

    -- Ensure that target does not already exist
    if self.workspace.is_target(name) then
        log_error("Target is already created")
        return nil
    end

    -- Folder path for engine modules
    local runtime_folder_path = get_runtime_folder_path()

    -- @brief - The type of target; decides if there should be a Standalone and DLL or if the application should be a ConsoleApp
    self.target_type = ETargetType.Client
    
    -- @brief - Whether or not the build should be forced monolithic
    self.is_monolithic = global_is_monolithic()

    -- @brief - Helper function for retrieving path
    local path_to_target = join_path(self.workspace.get_engine_path(), self.name)
    function self.get_path()
        return path_to_target
    end

    -- @brief - Inject module into the current module (i.e., put the files into the executable)
    local function inject_launch_module(rule)
        for index = 1, #rule.module_dependencies do
            local current_module_name = rule.module_dependencies[index]
            if current_module_name == "Launch" then
                if is_module("Launch") then
                    local launch_module = get_module("Launch")
                    launch_module.kind = "None"

                    rule.add_files(launch_module.files)
                    rule.add_exclude_files(launch_module.exclude_files)
                else
                    log_error("Found the Launch Module among dependencies, but it has not been initialized")
                end
                break
            end
        end
    end

    -- @brief - Generate target
    local base_generate = self.generate
    function self.generate()
        if self.workspace == nil then
            log_error("Workspace cannot be nil when generating Target")
            return
        end

        log_info("\n--- Generating Target '%s' ---", self.name)
  
        if self.is_monolithic then
            log_info("    Target '%s' is monolithic", self.name)
        else
            log_info("    Target '%s' is NOT monolithic", self.name)
        end
        
        -- Generate the project based on type
        if self.target_type == ETargetType.Client then
            log_info("    TargetType=Client")

            -- Always add module name as a define
            self.add_defines({ 'MODULE_NAME="' .. self.name .. '"' })

            local upper_case_name = self.name:upper()
            local module_api_name = upper_case_name .. "_API"

            -- TODO: Should this be created as a module instead? 
            -- In a monolithic build, the client should be linked statically 
            if self.is_monolithic then                
                self.kind               = "WindowedApp"
                self.runtime_linking    = false
                self.is_dynamic         = false
                self.embed_dependencies = true

                -- TODO: These should be handled via file and loaded into the Project-Module
                -- Defines
                self.add_defines({ 'PROJECT_NAME="' .. self.name .. '"' })
                self.add_defines({ 'PROJECT_LOCATION="' .. self.get_path() .. '"' })
                self.add_defines({ module_api_name })

                -- Generate the project
                log_info("\n--- Generating project for target '%s' ---", self.name)
                base_generate()
                inject_launch_module(self)
                log_info("\n--- Finished generating project for target '%s' ---", self.name)
            else
                self.kind            = "SharedLib"
                self.runtime_linking = true
                self.is_dynamic      = true
                
                self.add_defines({ module_api_name .. "=MODULE_EXPORT" })
                
                -- Generate the project
                log_info("\n--- Generating project for target '%s' ---", self.name)
                base_generate()
                log_info("\n--- Finished generating project for target '%s' ---", self.name)
                
                -- Standalone executable
                log_info("\n--- Generating Standalone client executable project for target '%s' ---", self.name)
                
                local executable = build_rules(self.name .. "Standalone")
                executable.kind               = "WindowedApp"
                executable.embed_dependencies = true

                -- Setup the workspace
                executable.workspace = self.workspace

                -- Link the module
                executable.add_link_libraries({ self.name })
                executable.add_extra_embed_names({ self.name })
                executable.add_module_dependencies(self.module_dependencies)
                
                if is_platform_mac() then
                    executable.add_frameworks({ "AppKit" })
                end

                -- Setup Defines
                executable.add_defines({ module_api_name })

                -- Overwrite all exclude files
                executable.exclude_files = {}
        
                -- System includes can be included in a dependency header and therefore necessary in this module as well
                executable.add_system_includes(self.system_includes)

                -- Generate Standalone executable
                executable.generate()
                inject_launch_module(executable)

                log_info("\n--- Finished generating standalone client executable project for target '%s' ---", self.name)
            end
        elseif self.target_type == ETargetType.WindowedApp then
            log_error("    TargetType=WindowedApp is not implemented yet")
            -- TODO: Handle this case properly
        elseif self.target_type == ETargetType.ConsoleApp then
            log_error("    TargetType=ConsoleApp is not implemented yet")
            -- TODO: Handle this case properly
        end
    end

    return self
end
