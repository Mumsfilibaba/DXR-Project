include 'build_rule.lua'

-- Module build rules
function CModuleBuildRules(InName)
    printf('Creating Module \'%s\'', InName)

    -- Init parent class
    local self = CBuildRules(InName)
    if self == nil then
        printf('Module Error: Failed to create BuildRule')
        return nil
    end

    -- Ensure that module does not already exist
    if IsModule(NewModuleName) then
        printf('Module is already created')
        return GetModule(NewModuleName)
    end
       
    -- Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()
    
    -- Should the module be dynamic or static, this is overridden by monolithic build, which forces all modules to be linked statically
    self.bIsDynamic = true
    -- Should perform linking at compile time (Ignored if bIsDynamic is false). Set to true if hot-reloading
    self.bRuntimeLinking = true
    
    -- Generate the module
    local BaseGenerate = self.Generate 
    function self.Generate()
        printf('    Generating Module %s\n', self.Name)
  
        -- Is the build monolithic
        local bIsMonolithic = IsMonolithic()
        if bIsMonolithic then
            printf('    Build is monolithic\n')
            
            self.bIsDynamic      = false
            self.bRuntimeLinking = true
        else
            printf('    Build is NOT monolithic\n')
        end
        
        -- Dynamic or static
        if self.bIsDynamic then
            local UpperCaseName  = self.Name:upper()
            local ModuleImplName = UpperCaseName .. '_IMPL=(1)'
            self.AddDefines(
            {
                ModuleImplName
            })

            self.Kind = 'SharedLib'
        else
            self.Kind = 'StaticLib'
        end
        
        -- Always add module name as a define
        self.AddDefines(
        {
            ('MODULE_NAME=' .. '\"' .. self.Name .. '\"')
        })

        -- Generate the project
        BaseGenerate()
    end

    -- Add module among global list
    AddModule(self.Name, self)
    return self
end