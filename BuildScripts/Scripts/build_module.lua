include "build_rule.lua"

-- Module build rules
function module_build_rules(name)
    log_highlight("Creating Module '%s'", name)

    -- Initialize parent class
    local self = build_rules(name)
    if self == nil then
        log_error("Failed to create BuildRule")
        return nil
    end

    -- Ensure that module does not already exist
    if is_module(name) then
        log_warning("Module is already created")
        return get_module(name)
    end

    -- Determines if the module should be dynamic; overridden by monolithic build
    self.is_dynamic = true

    -- Determines if linking should be performed at runtime (ignored if is_dynamic is false)
    -- Set to true to enable hot-reloading
    self.runtime_linking = false

    -- Generate the module
    local base_generate = self.generate
    function self.generate()
        if self.workspace == nil then
            log_error("Workspace cannot be nil when generating Module")
            return
        end

        log_info("\n--- Generating Module '%s' ---", self.name)

        -- Handle monolithic build
        self.is_monolithic = is_global_monolithic()
        if self.is_monolithic then
            log_info("    Build is monolithic")

            self.is_dynamic      = false
            self.runtime_linking = false
        else
            log_info("    Build is NOT monolithic")
        end

        -- Dynamic or static
        local module_api_name = self.name:upper() .. "_API"
        if self.is_dynamic then
            self.kind = "SharedLib"

            -- Add define to control the module implementation (for export/import)
            module_api_name = module_api_name .. "=MODULE_EXPORT"
        else
            self.kind = "StaticLib"

            -- When a module is not dynamic we treat it as monolithic
            self.add_defines({ "MONOLITHIC_BUILD=(1)" })
        end

        -- Always add module name as a define
        self.add_defines({ 'MODULE_NAME="' .. self.name .. '"' })
        self.add_defines({ module_api_name })

        -- Generate the project
        base_generate()
    end

    -- Add module to global list
    add_module(self.name, self)
    return self
end
