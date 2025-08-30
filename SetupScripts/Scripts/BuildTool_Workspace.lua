include "BuildTool_Module.lua"
include "BuildTool_Target.lua"

-- Generate a workspace from an array of target rules
function WorkspaceRules(WorkspaceName)
    
    -- Must have a valid workspace name
    if WorkspaceName == nil then
        return nil
    end

    LogHighlight("Creating Workspace '%s'", WorkspaceName)

    -- Initialize this object
    local self = {
        
        -- Name of the workspace being generated
        Name = WorkspaceName,
        
        -- List of targets for this workspace
        TargetRules = {},

        -- Name of the target of the workspace
        TargetName = "",
        
        -- Engine folder path
        EnginePath = GetEnginePath(),

        -- Defines
        Defines = {},

        -- Projects that should have projects generated
        ProjectRules = {},

        -- Name of the project that should be set as startup project
        StartProjectName = "",
    }

    -- Retrieve the current target name
    function self.GetCurrentTargetName()
        return self.TargetName
    end

    -- Retrieve a target added to the workspace
    function self.GetTarget(TargetName)
        if not TargetName then 
            return nil 
        end

        for _, t in ipairs(self.TargetRules) do
            if t and t.Name == TargetName then
                return t
            end
        end

        return nil
    end

    -- Check if a target already exists
    function self.IsTarget(TargetName)
        return self.GetTarget(TargetName) ~= nil
    end

    -- Helper function for adding a target (prevents duplicates by Name)
    function self.AddTarget(Target)
        if not Target or not Target.Name then
            LogError("AddTarget: invalid target")
            return
        end

        if self.IsTarget(Target.Name) then
            LogWarning("Target '%s' already added; skipping", Target.Name)
            return
        end

        table.insert(self.TargetRules, Target)
    end

    -- Helper function for adding defines
    function self.AddDefines(Define)
        AddUniqueElements(Define, self.Defines)
    end

    -- Helper function for adding a rule
    function self.AddRule(Rule)
        table.insert(self.ProjectRules, Rule)
    end

    -- Generate the actual solution files
    function self.GenerateSolutionFiles()
        
        -- Log the start of the generation
        LogInfo("\n--- Generating Solution Files for Workspace '%s' ---", self.Name)

        -- Set the name of the workspace
        workspace(self.Name)

        -- Set location of the generated solution file
        local SolutionLocation = GetSolutionsFolderPath()
        location(SolutionLocation)

        LogInfo("Generated solution location '%s'", SolutionLocation)

        -- Platforms
        platforms({
            "x64"
        })

        -- Configurations
        configurations({
            "Debug",
            "Release",
            "Production",
        })

        -- Includes
        local RuntimeFolderPathLocal = GetRuntimeFolderPath()
        includedirs({
            RuntimeFolderPathLocal
        })

        -- Workspace defines
        LogInfo("\n--- Workspace Defines (Num Defines=%d) ---", #self.Defines)
        if #self.Defines > 0 then
            PrintTable("  Using Define '%s'", self.Defines)
        else
            LogInfo("")
        end

        defines(self.Defines)

        -- Per-config CRT (Debug vs Release)
        filter "configurations:Debug"
            symbols "On"
            runtime "Debug"
            defines {
                "_DEBUG",
                "DEBUG",
                "DEBUG_BUILD=(1)"
            }
        filter {}

        filter "configurations:Release"
            symbols "On"
            runtime "Release"
            defines {
                "NDEBUG",
                "RELEASE_BUILD=(1)"
            }
        filter {}

        filter "configurations:Production"
            symbols "Off"
            runtime "Release"
            defines {
                "NDEBUG",
                "PRODUCTION_BUILD=(1)"
            }
        filter {}

        -- Architecture for all projects
        architecture "x86_64"

        -- Static vs dynamic CRT (MSVC only)
        filter "action:vs*"
            if IsBuildMonolithic() then
                staticruntime "On"   -- /MT(d)
            else
                staticruntime "Off"  -- /MD(d)
            end
        filter {}

        -- Architecture defines
        filter "architecture:x86"
            defines({
                "ARCHITECTURE_X86=(1)"
            })
        filter {}

        filter "architecture:x86_64"
            defines({
                "PLATFORM_ARCHITECTURE_X86_64=(1)"
            })
        filter {}

        filter "architecture:ARM"
            defines({
                "PLATFORM_ARCHITECTURE_ARM=(1)"
            })
        filter {}

        -- Startup project name
        LogInfo("StartProject = '%s'", self.StartProjectName)
        startproject(self.StartProjectName)

        -- Generate project files for all the rules that have been added
        LogInfo("\n--- Generating module and target project files ---")
        for _, CurrentRule in ipairs(self.ProjectRules) do
            CurrentRule.GenerateProject()
        end
    end

    -- Generate workspace
    function self.Generate()

        -- Default roots for Module.lua files: Runtime and ThirdParty
        AddModuleSearchRoot(GetRuntimeFolderPath())
        AddModuleSearchRoot(GetExternalThirdpartyFolderPath())

        -- Search through all of the folders for Module.lua and index them
        SearchForModuleFiles();

        LogInfo("\n--- Generating Workspace '%s' ---", self.Name)
        LogInfo("ConfigurationPath = '%s'", GetOutputConfigPath())

        if self.TargetRules == nil then
            LogError("TargetRules cannot be nil")
            return
        end

        if #self.TargetRules < 1 then
            LogError("Workspace must contain at least one build rule (Current=%d)", #self.TargetRules)
            return
        end

        -- Define the workspace location; we do this with a Unix path since the engine (C++ side) expects this currently
        local UnixEnginePath = path.translate(GetEnginePath(), "/")
        local EngineLocation = 'ENGINE_LOCATION="' .. UnixEnginePath .. '"'
        self.AddDefines({
            EngineLocation
        })
        
        LogInfo("Engine Path ='%s'", GetEnginePath())
        LogInfo("RuntimeFolderPath = '%s'", GetRuntimeFolderPath())
        
        -- Check if the command line overrides monolithic builds
        if IsBuildMonolithic() then
            self.AddDefines({
                "MONOLITHIC_BUILD=(1)"
            })
        end

        -- IDE Defines
        if BuildWithVisualStudio() then 
            self.AddDefines({
                "IDE_VISUAL_STUDIO",
                "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
                "_CRT_SECURE_NO_WARNINGS",
            })
        end

        -- OS Defines
        if IsPlatformWindows() then
            self.AddDefines({
                "PLATFORM_WINDOWS=(1)"
            })
        end
        if IsPlatformMac() then
            self.AddDefines({
                "PLATFORM_MACOS=(1)"
            })
        end

        -- Setup startup project
        local StartProjectTarget = self.TargetRules[1]
        if (StartProjectTarget.TargetType == ETargetType.Client) and (not IsBuildMonolithic()) then
            self.StartProjectName = StartProjectTarget.Name .. "Standalone"
        else
            self.StartProjectName = StartProjectTarget.Name
        end
        
        -- Generate projects from targets
        LogInfo("\n--- Generating Targets (NumTargets=%d) ---", #self.TargetRules)
        for _, CurrentTarget in ipairs(self.TargetRules) do
            self.TargetName = CurrentTarget.Name

            CurrentTarget.Workspace = self
            CurrentTarget.Generate()
        end

        -- Generate the actual solution files
        self.GenerateSolutionFiles()

        LogInfo("\n--- Finished generating workspace ---")
    end

    return self
end
