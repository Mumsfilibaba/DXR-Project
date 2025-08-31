include "BuildTool.lua"

-- Target types
ETargetType =
{
    Client      = 1,
    WindowedApp = 2,
    ConsoleApp  = 3,
}

-- Target build rules
function TargetBuildRules(Name, Workspace)

    -- Needs to have a valid module name
    if Name == nil then
        LogError("BuildRule failed due to invalid name")
        return nil
    end
    
    -- Needs to have a valid workspace
    if Workspace == nil then
        LogError("Workspace cannot be nil")
        return nil
    end

    -- Ensure that target does not already exist
    if Workspace.IsTarget(Name) then
        LogError("Target is already created")
        return nil
    end

    LogHighlight("Creating Target '%s'", Name)

    -- Initialize parent class
    local self = BuildRules(Name)
    if self == nil then
        LogError("Failed to create BuildRule")
        return nil
    end

    self.Workspace = Workspace

    -- Folder path for engine modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- The type of target. Decides if there should be a Standalone and DLL or if the app should be a ConsoleApp.
    self.TargetType = ETargetType.Client
    
    -- Helper function for retrieving path
    local PathToTarget = JoinPath(GetEnginePath(), self.Name)
    function self.GetPath()
        return PathToTarget
    end

    -- Inject module into the current module (i.e., put the files into the executable)
    local function InjectLaunchModule(Rule)
        for Index = 1, #Rule.Modules do
            local CurrentModuleName = Rule.Modules[Index]
            if CurrentModuleName == "Launch" then
                if IsModule("Launch") then
                    local LaunchModule = GetModule("Launch")
                    LaunchModule.Kind = "None"

                    Rule.AddFiles(LaunchModule.Files)
                    Rule.AddExcludeFiles(LaunchModule.ExcludeFiles)
                    Rule.AddDefines(LaunchModule.Defines)
                else
                    LogError("Found the Launch Module among thirdparties, but it has not been initialized")
                end

                break
            end
        end
    end

    -- Generate target
    local BaseGenerate = self.Generate
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating Target")
            return
        end

        LogInfo("--- Generating Target '%s' ---", self.Name)
  
        if IsBuildMonolithic() then
            LogInfo("Target '%s' is monolithic", self.Name)
        else
            LogInfo("Target '%s' is NOT monolithic", self.Name)
        end
        
        -- Generate the project based on type
        if self.TargetType == ETargetType.Client then
            LogInfo("TargetType=Client")

            -- Always add module name as a define
            self.AddDefines({
                'MODULE_NAME="' .. self.Name .. '"'
            })

            local UpperCaseName = self.Name:upper()
            local ModuleApiName = UpperCaseName .. "_API"

            -- In a monolithic build, the client is linked statically
            -- TODO: Should this be created as a module instead?
            if IsBuildMonolithic() then
                self.Kind = "WindowedApp"
                self.bRuntimeLinking = false
                self.bIsDynamic = false
                self.bEmbedThirdparties = true

                -- Defines
                self.AddDefines({
                    ModuleApiName
                })

                -- Generate the project
                LogInfo("--- Generating project for target '%s' ---", self.Name)
                BaseGenerate()
                InjectLaunchModule(self)
                LogInfo("--- Finished generating project for target '%s' ---", self.Name)
            else
                self.Kind = "SharedLib"
                self.bRuntimeLinking = true
                self.bIsDynamic = true
                
                self.AddDefines({
                    ModuleApiName .. "=MODULE_EXPORT"
                })
                
                -- Generate the project
                LogInfo("--- Generating project for target '%s' ---", self.Name)
                BaseGenerate()
                LogInfo("--- Finished generating project for target '%s' ---", self.Name)
                
                -- Standalone executable
                LogInfo("--- Generating Standalone client executable project for target '%s' ---", self.Name)
                
                local Executable = BuildRules(self.Name .. "Standalone")
                Executable.Kind = "WindowedApp"
                Executable.bEmbedThirdparties = true

                -- Setup the workspace
                Executable.Workspace = self.Workspace

                -- Link the module
                Executable.AddModules(self.Modules)
                
                Executable.AddLinkLibraries({
                    self.Name
                })
                Executable.AddExtraEmbedNames({
                    self.Name
                })
                
                if IsPlatformMac() then
                    Executable.AddFrameworks({
                        "AppKit"
                    })
                end

                -- Setup Defines
                Executable.AddDefines({
                    ModuleApiName
                })

                -- Overwrite all exclude files
                Executable.ExcludeFiles = {}
        
                -- Includes can be included in a thirdparty header and therefore necessary in this module as well
                Executable.AddIncludeDirs(self.IncludeDirs)
                Executable.AddExternalIncludeDirs(self.ExternalIncludeDirs)

                -- Generate Standalone executable
                Executable.Generate()
                InjectLaunchModule(Executable)

                LogInfo("--- Finished generating standalone client executable project for target '%s' ---", self.Name)
            end
        elseif self.TargetType == ETargetType.WindowedApp then
            LogError("TargetType=WindowedApp is not implemented yet")
            -- TODO: Handle this case properly
        elseif self.TargetType == ETargetType.ConsoleApp then
            LogError("TargetType=ConsoleApp is not implemented yet")
            -- TODO: Handle this case properly
        end
    end

    return self
end
