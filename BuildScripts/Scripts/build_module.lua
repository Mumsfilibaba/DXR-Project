include "build_rule.lua"

-- Module build rules
function FModuleBuildRules(InName)
    LogHighlight("Creating Module \'%s\'", InName)

    -- Init parent class
    local self = FBuildRules(InName)
    if self == nil then
        LogError("Failed to create BuildRule")
        return nil
    end

    -- Ensure that module does not already exist
    if IsModule(NewModuleName) then
        LogWarning("Module is already created")
        return GetModule(NewModuleName)
    end
    
    -- @brief - Determines of the module should be dynamic otherwise static, this is overridden by monolithic build, which forces all modules to be linked statically
    self.bIsDynamic = true
	
    -- @brief - Determines if linking should be performed at compile time (Ignored if bIsDynamic is false). Set to true to enable hot-reloading.
    self.bRuntimeLinking = false
    
    -- Generate the module
    local BuildRulesGenerate = self.Generate 
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating Module")
            return
        end

        LogInfo("\n--- Generating Module \'%s\' ---", self.Name)
  
        -- Handle Monolithic build
        self.bIsMonolithic = IsMonolithic()
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
            
            -- Add define to control the module implementation (For export/import)
            ModuleApiName = ModuleApiName .. "=MODULE_EXPORT"
        else
            self.Kind = "StaticLib"

            -- When a module is not dynamic we treat is as monolithic
            self.AddDefines({ "MONOLITHIC_BUILD=(1)" })
        end
        
        -- Always add module name as a define
        self.AddDefines({ "MODULE_NAME=" .. "\"" .. self.Name .. "\"" })
        self.AddDefines({ ModuleApiName })

        -- Generate the project
        BuildRulesGenerate()
    end

    -- Add module among global list
    AddModule(self.Name, self)
    return self
end