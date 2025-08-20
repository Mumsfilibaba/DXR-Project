include "BuildTool_Rule.lua"

-- Module build rules
function ModuleBuildRules(Name)
    LogHighlight("Creating Module '%s'", Name)

    -- Initialize parent class
    local self = BuildRules(Name)
    if self == nil then
        LogError("Failed to create BuildRule")
        return nil
    end

    -- Ensure that module does not already exist
    if IsModule(Name) then
        LogWarning("Module is already created")
        return GetModule(Name)
    end

    -- Determines if the module should be dynamic; overridden by monolithic build
    self.bIsDynamic = true

    -- Determines if linking should be performed at runtime (ignored if bIsDynamic is false)
    -- Set to true to enable hot-reloading
    self.bRuntimeLinking = false

    -- Generate the module
    local BaseGenerate = self.Generate
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating Module")
            return
        end

        LogInfo("\n--- Generating Module '%s' ---", self.Name)

        -- Handle monolithic build
        self.bIsMonolithic = GlobalIsMonolithic()
        if self.bIsMonolithic then
            LogInfo("    Build is monolithic")

            self.bIsDynamic      = false
            self.bRuntimeLinking = false
        else
            LogInfo("    Build is NOT monolithic")
        end

        -- Dynamic or static
        local ModuleApiName = self.Name:upper() .. "_API"
        if self.bIsDynamic then
            self.Kind = "SharedLib"

            -- Add define to control the module implementation (for export/import)
            ModuleApiName = ModuleApiName .. "=MODULE_EXPORT"
        else
            self.Kind = "StaticLib"

            -- When a module is not dynamic we treat it as monolithic
            self.AddDefines({ "MONOLITHIC_BUILD=(1)" })
        end

        -- Always add module name as a define
        self.AddDefines({ 'MODULE_NAME="' .. self.Name .. '"' })
        self.AddDefines({ ModuleApiName })

        -- Generate the project
        BaseGenerate()
    end

    -- Add module to global list
    AddModule(self.Name, self)
    return self
end
