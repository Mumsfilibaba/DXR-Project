include "BuildTool.lua"

-- Target types
ETargetType =
{
    Game    = 1, -- Standalone game (Uses main inside launch)
    Editor  = 2, -- Editor with the game (Uses main inside launch)
    Program = 3, -- Standalone application (Uses main inside of the application itself)
}

-- Target build rules
function TargetBuildRules(Name)

    -- Needs to have a valid module name
    if type(Name) ~= "string" or Name == "" then
        LogError("TargetBuildRules failed due to invalid name")
        return nil
    end

    -- Ensure that target does not already exist
    if IsTargetRule(Name) then
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

    -- The type of target. Decides if there should be a Standalone and DLL or if the app should be a ConsoleApp.
    self.TargetType = ETargetType.Game
    
    -- Helper functions for the target's source path. Defaults to <Engine>/<Name>; the
    -- workspace overrides it with the folder the Target.lua was found in.
    local PathToTarget = JoinPath(GetEnginePath(), self.Name)
    function self.GetPath()
        return PathToTarget
    end
    function self.SetPath(NewPath)
        PathToTarget = CreateOsPath(NewPath)
    end

    -- Inject module into the current module (i.e., put the files into the executable)
    local function InjectLaunchModule(Rule)
        for i = 1, #Rule.Modules do
            local DepName = Rule.Modules[i]
            if DepName == "Launch" then
                if IsModuleRule("Launch") then
                    local Launch = GetModuleRule("Launch")
                    Launch.Kind = "None"  -- prevent a separate build target

                    Rule.AddFiles(Launch.Files)
                    Rule.AddExcludeFiles(Launch.ExcludeFiles)
                    Rule.AddDefines(Launch.Defines)
                    Rule.AddIncludeDirs(Launch.IncludeDirs)
                    Rule.AddExternalIncludeDirs(Launch.ExternalIncludeDirs)
                    Rule.AddForceIncludes(Launch.ForceIncludes)
                else
                    LogError("Found the Launch Module among dependencies, but it has not been initialized")
                end
                break
            end
        end
    end

    -- Generate target
    local BaseGenerate = self.Generate
    function self.Generate()
        if self.IsGenerated and self.IsGenerated() then
            LogHighlightWarning("Target '%s' already generated. Skipping ..", self.Name)
            return
        end

        LogInfo("--- Generating Target '%s' ---", self.Name)
  
        if IsBuildMonolithic() then
            LogInfo("Target '%s' is monolithic", self.Name)
        else
            LogInfo("Target '%s' is NOT monolithic", self.Name)
        end
        
        -- Generate the project based on type
        if self.TargetType == ETargetType.Game then
            LogInfo("TargetType=Game")

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

                -- Defines. Empty value, since a monolithic target exports nothing
                self.AddDefines({
                    ModuleApiName .. "="
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
        elseif self.TargetType == ETargetType.Editor then
            LogInfo("TargetType=Editor contributes configurations only, no project")
        elseif self.TargetType == ETargetType.Program then
            LogInfo("TargetType=Program")

            -- A Program owns its own main(), so Launch is never injected and there is no
            -- separate Standalone executable to generate.
            if self.Kind == "SharedLib" or self.Kind == "WindowedApp" then
                self.Kind = "ConsoleApp"
            end

            self.bIsDynamic         = false
            self.bRuntimeLinking    = false
            self.bEmbedThirdparties = true

            -- Hyphens are legal in a target name but not in a macro, and a Program exports
            -- nothing, so its API macro expands to nothing.
            local SafeName = self.Name:gsub("[^%w_]", "_"):upper()
            self.AddDefines({
                'MODULE_NAME="' .. self.Name .. '"',
                SafeName .. "_API="
            })

            LogInfo("--- Generating project for target '%s' ---", self.Name)
            BaseGenerate()
            LogInfo("--- Finished generating project for target '%s' ---", self.Name)
        end
    end

    -- Add target to global list
    AddTargetRule(self.Name, self)
    return self
end
