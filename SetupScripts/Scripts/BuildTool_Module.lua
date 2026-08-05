include "BuildTool_Rule.lua"

-- Module build rules
function ModuleBuildRules(Name)
    
    -- Ensure that module does not already exist
    if IsModuleRule(Name) then
        LogHighlightWarning("Module '%s' is already created. Returning existing instance", Name)
        return GetModuleRule(Name)
    end
    
    LogHighlight("Creating Module '%s'", Name)

    -- Initialize parent class
    local self = BuildRules(Name)
    if not self then
        LogError("Parent function BuildRules failed")
        return nil
    end

    -- Determines if the module should be dynamic. Only consulted for the modular layout;
    -- see IsDynamicIn() below.
    self.bIsDynamic = true

    -- Determines if linking should be performed at runtime (ignored if bIsDynamic is false)
    -- Set to true to enable hot-reloading.
    self.bRuntimeLinking = false

    -- Set this to true if this is a library and not a module. Then this module is simply built
    -- as a library, either static or dynamic depending on bIsDynamic.
    self.bIsLibrary = false

    -- Generate the module
    local BaseGenerate = self.Generate
    function self.Generate()
        if self.IsGenerated and self.IsGenerated() then
            LogHighlightWarning("Module '%s' already generated. Skipping ..", self.Name)
            return
        end

        LogInfo("--- Generating Module '%s' ---", self.Name)

        function self.IsDynamicIn(Layout)
            if self.bIsLibrary then
                return self.bIsDynamic
            end

            return Layout == ELayout.Modular and self.bIsDynamic or false
        end

        function self.IsRuntimeLinkedIn(Layout)
            if self.bIsLibrary then
                return self.bRuntimeLinking
            end

            return Layout == ELayout.Modular and self.bRuntimeLinking or false
        end

        function self.KindIn(Layout)
            return self.IsDynamicIn(Layout) and "SharedLib" or "StaticLib"
        end

        function self.ApiDefineIn(Layout)
            if self.bIsLibrary then
                return nil
            end

            local ModuleApiName = self.Name:upper() .. "_API"
            return self.IsDynamicIn(Layout) and (ModuleApiName .. "=MODULE_EXPORT")
                                             or (ModuleApiName .. "=")
        end

        -- The module name does not vary by layout
        if not self.bIsLibrary then
            self.AddDefines({
                'MODULE_NAME="' .. self.Name .. '"'
            })
        end

        self.Kind = self.KindIn(GetGeneratedLayouts()[1])

        -- Generate the project
        BaseGenerate()

        LogInfo("--- Finished Generating Module '%s' ---", self.Name)
    end

    -- Add module to global list
    AddModuleRule(self.Name, self)
    return self
end
