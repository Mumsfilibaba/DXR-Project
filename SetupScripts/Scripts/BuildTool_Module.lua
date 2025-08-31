include "BuildTool_Rule.lua"

-- Module build rules
function ModuleBuildRules(Name)
    
    -- Ensure that module does not already exist
    if IsModule(Name) then
        LogHighlightWarning("Module '%s' is already created. Returning existing instance", Name)
        return GetModule(Name)
    end
    
    LogHighlight("Creating Module '%s'", Name)

    -- Initialize parent class
    local self = BuildRules(Name)
    if not self then
        LogError("Parent function BuildRules failed")
        return nil
    end

    -- Determines if the module should be dynamic; overridden by monolithic build
    self.bIsDynamic = true

    -- Determines if linking should be performed at runtime (ignored if bIsDynamic is false)
    -- Set to true to enable hot-reloading
    self.bRuntimeLinking = false

    -- Set this to true if this is a library and not a module. Then this module is simply built
    -- as a library, either static or dynamic depending on bIsDynamic.
    self.bIsLibrary = false

    -- Generate the module
    local BaseGenerate = self.Generate
    function self.Generate()
        if not self.Workspace then
            LogError("Workspace cannot be nil when generating Module")
            return
        end

        LogInfo("--- Generating Module '%s' ---", self.Name)

        -- Handle monolithic build
        if IsBuildMonolithic() and (not self.bIsLibrary) then
            self.bIsDynamic      = false
            self.bRuntimeLinking = false
            
            LogInfo("Build is monolithic")
        else
            LogInfo("Build is NOT monolithic")
        end

        -- Dynamic or static
        if self.bIsLibrary then
            self.Kind = self.bIsDynamic and "SharedLib" or "StaticLib"
        else
            local ModuleApiName = self.Name:upper() .. "_API"
            if self.bIsDynamic then
                -- Add define to control the module implementation (for export/import)
                ModuleApiName = ModuleApiName .. "=MODULE_EXPORT"

                self.Kind = "SharedLib"
            else
                self.Kind = "StaticLib"
            end

            -- Always add module name and API as defines
            self.AddDefines({
                'MODULE_NAME="' .. self.Name .. '"',
                ModuleApiName
            })
        end

        -- Generate the project
        BaseGenerate()

        LogInfo("--- Finished Generating Module '%s' ---", self.Name)
    end

    -- Add module to global list
    AddModule(self.Name, self)
    return self
end
