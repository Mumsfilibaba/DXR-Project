include "build_module.lua"

-- Target types
ETargetType = 
{
    ["Client"]      = 1,
    ["WindowedApp"] = 2,
    ["ConsoleApp"]  = 3,
}

-- Target build rules
function FTargetBuildRules(InName, InWorkspace)

    -- Needs to have a valid modulename
    if InName == nil then
        LogError("BuildRule failed due to invalid name")
        return nil
    end
    
    -- Needs to have a valid workspace
    if InWorkspace == nil then
        LogError("Workspace cannot be nil")
        return nil
    end

    LogHighlight("Creating Target \'%s\'", InName)

    -- Init parent class
    local self = FBuildRules(InName)
    if self == nil then
        LogError("Failed to create BuildRule")
        return nil
    end

    self.Workspace = InWorkspace

    -- Ensure that target does not already exist
    if self.Workspace.IsTarget(InName) then
        LogError("Target is already created")
        return nil
    end

    -- Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- @brief - The type of target, decides if there should be a Standalone and DLL or if the application should be a ConsoleApp
    self.TargetType = ETargetType.Client
    
    -- @brief - Weather or not the build should be forced monolithic
    self.bIsMonolithic = IsMonolithic()

    -- @brief - Helper function for retrieving path
    self.GetPath = nil
    function self.GetPath()
        return self.Workspace.GetEnginePath() .. "/" ..  self.Name
    end

    -- @brief - Inject module into the current module (I.e put the files into the executable)
    function InjectLaunchModule(Rule)
        for Index = 1, #Rule.ModuleDependencies do
            local CurrentModuleName = Rule.ModuleDependencies[Index]
            if CurrentModuleName == "Launch" then
                if IsModule("Launch") then
                    local LaunchModule = GetModule("Launch");
                    LaunchModule.Kind = "None"

                    Rule.AddFiles(LaunchModule.Files)
                    Rule.AddExcludeFiles(LaunchModule.ExcludeFiles)
                else
                    LogError("Found the Launch Module among dependencies, however, the Launch module has not been initialized")
                end

                break
            end
        end
    end

    -- @brief - Generate target
    local BuildRulesGenerate = self.Generate
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating Target")
            return
        end

        LogInfo("\n--- Generating Target \'%s\' ---", self.Name)
  
        if self.bIsMonolithic then
            LogInfo("    Target \'%s\' is monolithic", self.Name)
        else
            LogInfo("    Target \'%s\' is NOT monolithic", self.Name)
        end
        
        -- Generate the project based on type
        if self.TargetType == ETargetType.Client then
            LogInfo("    TargetType=Client")

            -- Always add module name as a define
            self.AddDefines({ "MODULE_NAME=" .. "\"" .. self.Name .. "\"" })

            local UpperCaseName = self.Name:upper()
            local ModuleApiName = UpperCaseName .. "_API"

            -- TODO: Should this be created as a module instead? 
            -- In a monolithic build then the client should be linked statically 
            if self.bIsMonolithic then                
                self.Kind               = "WindowedApp"
                self.bRuntimeLinking    = false
                self.bIsDynamic         = false
                self.bEmbedDependencies = true

                -- TODO: These should be handled via file and loaded into the Project-Module
                -- Defines
                self.AddDefines({ "PROJECT_NAME=" .. "\"" .. self.Name .. "\"" })
                self.AddDefines({ "PROJECT_LOCATION=" .. "\"" .. self.GetPath() .. "\"" })
                self.AddDefines({ ModuleApiName })

                -- Generate the project
                LogInfo("\n--- Generating project for target \'%s\' ---", self.Name)
                BuildRulesGenerate()
                InjectLaunchModule(self)
                LogInfo("\n--- Finished generating project for target \'%s\' ---", self.Name)
            else
                self.Kind            = "SharedLib"
                self.bRuntimeLinking = true
                self.bIsDynamic      = true
                
                self.AddDefines({ ModuleApiName .. "=MODULE_EXPORT" })
                
                -- Generate the project
                LogInfo("\n--- Generating project for target \'%s\' ---", self.Name)
                BuildRulesGenerate()
                LogInfo("\n--- Finished generating project for target \'%s\' ---", self.Name)
                
                -- Standalone-executable
                LogInfo("\n--- Generating Standablone client executable project for target \'%s\' ---", self.Name)
                
                local Executeble = FBuildRules(self.Name .. "Standalone")
                Executeble.Kind               = "WindowedApp"
                Executeble.bEmbedDependencies = true

                -- Setup the workspace
                Executeble.Workspace = self.Workspace

                -- Link the module
                Executeble.AddLinkLibraries({ self.Name })
                Executeble.AddExtraEmbedNames({ self.Name })
                Executeble.AddModuleDependencies(self.ModuleDependencies)
                
                if IsPlatformMac() then
                    Executeble.AddFrameWorks({ "AppKit" })
                end

                -- Setup Defines
                Executeble.AddDefines({ ModuleApiName })

                -- Overwrite all exclude-files
                Executeble.ExcludeFiles = { }
        
                -- System includes can be included in a dependency header and therefore necessary in this module aswell
                Executeble.AddSystemIncludes(self.SystemIncludes)

                -- Generate Standalone executable
                Executeble.Generate()
                InjectLaunchModule(Executeble)

                LogInfo("\n--- Finished generating standablone client executable project for target \'%s\' ---", self.Name)
            end
        elseif self.TargetType == ETargetType.WindowedApp then
            LogError("    TargetType=WindowedApp is not implemented yet")
            -- TODO: Handle this case properly
        elseif self.TargetType == ETargetType.ConsoleApp then
            LogError("    TargetType=ConsoleApp is not implemented yet")
            -- TODO: Handle this case properly
        end
    end

    return self
end