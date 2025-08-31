include "BuildTool.lua"
include "BuildTool_Target.lua"

-- Name of the workspace being generated
local gWorkspaceName = ""

function GetWorkspaceName()
    return gWorkspaceName
end

function SetWorkspaceName(ModuleName)
    gWorkspaceName = ModuleName
end

-- All the targets int the workspace
local gTargetRules = {}

-- Retrieve a target added to the workspace
function GetTarget(InTargetName)
    if not InTargetName then 
        return nil 
    end

    for _, Target in ipairs(gTargetRules) do
        if Target and Target.Name == InTargetName then
            return Target
        end
    end

    return nil
end

-- Returns true if a target already exists
function IsTarget(InTargetName)
    return GetTarget(InTargetName) ~= nil
end

-- Add a target to the workspace (prevents duplicates by Name)
function AddTarget(Target)
    if not Target or not Target.Name then
        LogError("AddTarget: invalid target")
        return
    end

    if IsTarget(Target.Name) then
        LogWarning("Target '%s' already added; skipping", Target.Name)
        return
    end

    table.insert(gTargetRules, Target)
end

-- Name of the current target being processed
local gTargetName = ""

-- Retrieve the current target name
function GetCurrentTargetName()
    return gTargetName
end

-- Global defines for the whole workspace
local gGlobalDefines = {}

-- Helper function for adding defines
function AddGlobalDefines(InDefine)
    AddUniqueElements(InDefine, gGlobalDefines)
end

-- Projects that should have projects generated
local gProjectRules = {}

-- Helper function for adding a rule
function AddProjectRule(InRule)
    table.insert(gProjectRules, InRule)
end

-- Name of the project that should be set as startup project
local gStartProjectName = ""

-- Generate the actual solution files
function GenerateSolutionFiles()

    -- Log the start of the generation
    LogInfo("--- Generating Solution Files for Workspace '%s' ---", GetWorkspaceName())

    -- Set the name of the workspace
    workspace(GetWorkspaceName())

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
    LogInfo("--- Workspace Defines (Num Defines=%d) ---", #gGlobalDefines)
    if #gGlobalDefines > 0 then
        PrintTable("  Using Define '%s'", gGlobalDefines)
    else
        LogInfo("")
    end

    defines(gGlobalDefines)

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
    LogInfo("StartProject = '%s'", gStartProjectName)
    startproject(gStartProjectName)

    -- Generate project files for all the rules that have been added
    LogInfo("--- Generating module and target project files ---")
    for _, CurrentRule in ipairs(gProjectRules) do
        CurrentRule.GenerateProject()
    end
end

-- Generate workspace
function GenerateWorkspace()

    -- Logging
    LogInfo("--- Generating Workspace '%s' ---", gWorkspaceName)
    LogInfo("ConfigurationPath = '%s'", GetOutputConfigPath())

    if gTargetRules == nil then
        LogError("TargetRules cannot be nil")
        return
    end

    if #gTargetRules < 1 then
        LogError("Workspace must contain at least one build rule (Current=%d)", #gTargetRules)
        return
    end

    -- Define the workspace location. We do this with a Unix path since the engine (C++ side) expects this currently.
    local UnixEnginePath = path.translate(GetEnginePath(), "/")
    local EngineLocation = 'ENGINE_LOCATION="' .. UnixEnginePath .. '"'
    AddGlobalDefines({
        EngineLocation
    })
    
    LogInfo("Engine Path ='%s'", CreateOsPath(GetEnginePath()))
    LogInfo("RuntimeFolderPath = '%s'", CreateOsPath(GetRuntimeFolderPath()))
    
    -- Check if the command line overrides monolithic builds
    if IsBuildMonolithic() then
        AddGlobalDefines({
            "MONOLITHIC_BUILD=(1)"
        })
    end

    -- IDE Defines
    if BuildWithVisualStudio() then 
        AddGlobalDefines({
            "IDE_VISUAL_STUDIO",
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        })
    end

    -- OS Defines
    if IsPlatformWindows() then
        AddGlobalDefines({
            "PLATFORM_WINDOWS=(1)"
        })
    end
    if IsPlatformMac() then
        AddGlobalDefines({
            "PLATFORM_MACOS=(1)"
        })
    end

    -- Setup startup project
    local StartProjectTarget = gTargetRules[1]
    if (StartProjectTarget.TargetType == ETargetType.Client) and (not IsBuildMonolithic()) then
        gStartProjectName = StartProjectTarget.Name .. "Standalone"
    else
        gStartProjectName = StartProjectTarget.Name
    end
    
    -- Generate projects from targets
    LogInfo("--- Generating Targets (NumTargets=%d) ---", #gTargetRules)
    for _, CurrentTarget in ipairs(gTargetRules) do
        gTargetName = CurrentTarget.Name
        CurrentTarget.Generate()
    end

    -- Generate the actual solution files
    GenerateSolutionFiles()

    LogInfo("--- Finished generating workspace ---")
end
