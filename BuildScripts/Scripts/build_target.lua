include 'build_module.lua'

-- Target types
ETargetType = 
{
    ["Client"]      = 1,
    ["WindowedApp"] = 2,
    ["ConsoleApp"]  = 3,
}

-- Target build rules
function CTargetBuildRules(InName)
    printf('Creating Target \'%s\'', InName)
    
    -- Init parent class
    local self = CBuildRules(InName)
    if self == nil then
        printf('Target Error: Failed to create BuildRule')
        return nil
    end

    -- Ensure that target does not already exist
    if IsTarget(InName) then
        printf('Target is already created')
        return GetTarget(InName)
    end
       
    -- Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- The type of target, decides if there should be a Standalone and DLL or if the application should be a ConsoleApp
    self.TargetType = ETargetType.Client
    -- Weather or not the build should be forced monolithic
    self.bIsMonolithic = IsMonolithic()

    -- Helper function for retrieving path
    self.GetPath = nil
    function self.GetPath()
        return GetEnginePath() .. '/' ..  self.Name
    end

    -- Generate target
    local BaseGenerate = self.Generate
    function self.Generate()
        printf('    Generating Target \'%s\'\n', self.Name)
  
        -- Is the build monolithic
        if self.bIsMonolithic then
            printf('    Build is monolithic\n')
            SetIsMonlithic(true)
        else
            printf('    Build is NOT monolithic\n')
            SetIsMonlithic(false)
        end
        
        -- Generate the project based on type
        if self.TargetType == ETargetType.Client then
            printf('    TargetType=Client\n')

            -- Always add module name as a define
            self.AddDefines(
            {
                ('MODULE_NAME=' .. '\"' .. self.Name .. '\"')
            })

            -- TODO: Check for OS
            -- Include EntryPoint
            local MainFile = '';
            if BuildWithXcode() then
                MainFile = RuntimeFolderPath .. '/Main/Mac/MacMain.cpp'
            else
                MainFile = RuntimeFolderPath .. '/Main/Windows/WindowsMain.cpp'
            end

            -- TODO: Should this be created as a module instead? 
            -- In a monolithic build then the client should be linked statically 
            if self.bIsMonolithic then                
                self.Kind = 'WindowedApp'

                -- Defines
                self.AddDefines(
                { 
                    ('PROJECT_NAME=' .. '\"' .. self.Name .. '\"'),
                    ('PROJECT_LOCATION=' .. '\"' .. FindWorkspaceDir() .. '/' .. self.Name .. '\"')
                })

                self.AddFiles( 
                {
                    (RuntimeFolderPath .. '/Main/EngineLoop.cpp'),
                    (RuntimeFolderPath .. '/Main/EngineMain.inl'),
                    MainFile    
                })

                -- Generate the project
                printf('    ---Generating target project')
                BaseGenerate()
                printf('    ---Finished generating target project')
            else
                self.Kind = 'SharedLib'

                local UpperCaseName  = self.Name:upper()
                local ModuleImplName = UpperCaseName .. '_IMPL=(1)'
                self.AddDefines(
                {
                    ModuleImplName
                })
    
                -- Generate the project
                printf('    ---Generating target project')
                BaseGenerate()
                printf('    ---Finished generating target project')
                
                -- Standalone-executable
                printf('    ---Generating Standablone client executable\n')
                
                local Executeble = CBuildRules(self.Name .. 'Standalone')
                Executeble.Kind = 'WindowedApp'

                Executeble.AddLinkLibraries(
                {
                    self.Name
                })
                
                Executeble.AddModuleDependencies(self.ModuleDependencies)

                -- Files
                Executeble.Files = 
                {
                    (RuntimeFolderPath .. '/Main/EngineLoop.cpp'),
                    (RuntimeFolderPath .. '/Main/EngineMain.inl'),
                    MainFile    
                }

                -- Defines
                Executeble.AddDefines(
                { 
                    ('PROJECT_NAME=' .. '\"' .. self.Name .. '\"'),
                    ('PROJECT_LOCATION=' .. '\"' .. FindWorkspaceDir() .. '/' .. self.Name .. '\"')
                })
    
                -- Overwrite all
                Executeble.ExcludeFiles = {}
        
                -- System includes can be included in a dependency header and therefore necessary in this module aswell
                Executeble.AddSystemIncludes(self.SystemIncludes)

                -- Generate Standalone executable
                Executeble.Generate()

                printf('    ---Finished generating client executable\n')
            end
        elseif self.TargetType == ETargetType.WindowedApp then
            printf('    TargetType=WindowedApp\n')
        elseif self.TargetType == ETargetType.ConsoleApp then
            printf('    TargetType=ConsoleApp\n')
        end
    end

    return self
end