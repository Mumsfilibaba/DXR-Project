include "Build_Rule.lua"

-- Module build rules
function FModuleBuildRules(InName)
    printf("Creating Module \'%s\'", InName)

    -- Init parent class
    local self = FBuildRules(InName)
    if self == nil then
        printf("ERROR: Failed to create BuildRule")
        return nil
    end

    -- Ensure that module does not already exist
    if IsModule(NewModuleName) then
        printf("ERROR: Module is already created")
        return GetModule(NewModuleName)
    end
       
    -- @brief - Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()
    
    -- @brief - Determines of the module should be dynamic otherwise static, this is overridden by monolithic build, which forces all modules to be linked statically
    self.bIsDynamic = true
	
    -- @brief - Determines if linking should be performed at compile time (Ignored if bIsDynamic is false). Set to true to enable hot-reloading.
    self.bRuntimeLinking = false
    
    -- Generate the module
    local BaseGenerate = self.Generate 
    function self.Generate()
        printf("    Generating Module \'%s\'\n", self.Name)
  
        -- Handle Monolithic build
        local bIsMonolithic = IsMonolithic()
        if bIsMonolithic then
            printf("    Build is monolithic\n")
            
            self.bIsDynamic      = false
            self.bRuntimeLinking = false
        else
            printf("    Build is NOT monolithic\n")
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
            self.AddDefines(
            {
                "MONOLITHIC_BUILD=(1)"
            })
        end
        
        -- Always add module name as a define
        self.AddDefines(
        {
            ("MODULE_NAME=" .. "\"" .. self.Name .. "\""),
            ModuleApiName
        })

        -- Generate the project
        BaseGenerate()
    end

    -- Add module among global list
    AddModule(self.Name, self)
    return self
end