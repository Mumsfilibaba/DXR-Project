include "BuildTool.lua"
include "BuildTool_Target.lua"

-- Name of the workspace being generated
local gWorkspaceName = ""

function GetWorkspaceName()
    return gWorkspaceName
end

function SetWorkspaceName(WorkspaceName)
    gWorkspaceName = WorkspaceName
end

-- All the targets in the workspace (names only)
local gTargets = {}

-- Retrieve a target name that has been added to the workspace (returns the name or nil)
function GetTarget(InTargetName)
    if not InTargetName then
        return nil
    end
    
    for _, TargetName in ipairs(gTargets) do
        if TargetName == InTargetName then
            return TargetName
        end
    end

    return nil
end

-- Returns true if a target name already exists
function IsTarget(InTargetName)
    return GetTarget(InTargetName) ~= nil
end

-- Add a target to the workspace (names only)
function AddTarget(TargetName)
    if type(TargetName) ~= "string" or TargetName == "" then
        LogError("AddTarget: invalid target name")
        return
    end

    if IsTarget(TargetName) then
        LogWarning("Target '%s' already added. Skipping ..", TargetName)
        return
    end

    table.insert(gTargets, TargetName)
end

-- Name of the current target being processed
local gCurrentTargetName = ""

-- Retrieve the current target name
function GetCurrentTargetName()
    return gCurrentTargetName
end

-- Global defines for the whole workspace
local gGlobalDefines = {}

-- Helper function for adding defines
function AddGlobalDefines(InDefine)
    AddUniqueElements(InDefine, gGlobalDefines)
end

-- Projects that should have projects generated
local gProjectRules = {}

-- Adding a rule that should be included as a project in the workspace
function AddProjectRule(InRule)
    table.insert(gProjectRules, InRule)
end

-- Check if a project with a given name was added
local function HasProjectRule(ProjectName)
    for _, Rule in ipairs(gProjectRules) do
        if Rule and Rule.Name == ProjectName then
            return true
        end
    end

    return false
end

-- Name of the project that should be set as startup project
local gStartProjectName = ""

-- The different types of targets that exists within the workspace, used to create different configurations
local gUsedTargetTypes    = {}
local gUsedTargetTypesSet = {}

local gValidTargetTypes = {
    [ETargetType.Game]    = true,
    [ETargetType.Editor]  = true,
    [ETargetType.Program] = true,
}

local function AddTargetType(TargetType)
    if not gValidTargetTypes[TargetType] then
        return false
    end

    if gUsedTargetTypesSet[TargetType] then
        return false
    end

    gUsedTargetTypesSet[TargetType] = true
    table.insert(gUsedTargetTypes, TargetType)
    return true
end

local function HasTargetType(TargetType)
    return gUsedTargetTypesSet[TargetType] == true
end

local function GetUsedTargetTypes()
    return gUsedTargetTypes
end

local function ClearUsedTargetTypes()
    gUsedTargetTypes    = {}
    gUsedTargetTypesSet = {}
end

-- Configurations for this workspace
local gConfigurations = { }

-- Solution generation
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
    configurations(gConfigurations)

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

    -- Debug configs
    filter { "configurations:*Debug*" }
        symbols "On"
        runtime "Debug"
        defines {
            "_DEBUG",
            "DEBUG",
            "DEBUG_BUILD=(1)"
        }
    filter {}

    -- Development configs
    filter { "configurations:*Development*" }
        symbols "On"
        runtime "Release"
        defines {
            "NDEBUG",
            "DEVELOPMENT_BUILD=(1)"
        }
    filter {}

    -- Release configs
    filter { "configurations:*Release*" }
        symbols "Off"
        runtime "Release"
        defines { 
            "NDEBUG",
            "RELEASE_BUILD=(1)"
        }
    filter {}

    -- Editor configs
    filter { "configurations:*Editor*" }
        defines { 
            "EDITOR_BUILD=(1)",
        }
    filter {}

    -- Architecture for all projects
    architecture "x86_64"

    -- Static vs dynamic CRT (MSVC only)
    filter "action:vs*"
        if IsBuildMonolithic() then
            staticruntime "On" -- /MT(d)
        else
            staticruntime "Off" -- /MD(d)
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

-- Decide startup project based on what was actually generated
local function ComputeStartProjectName()
    if #gTargets == 0 then
        gStartProjectName = ""
        return
    end

    -- If non-monolithic, prefer "<Name>Standalone" if it exists, else fallback to "<Name>"
    local FirstTarget = gTargets[1]
    if not IsBuildMonolithic() then
        local Standalone = FirstTarget .. "Standalone"
        if HasProjectRule(Standalone) then
            gStartProjectName = Standalone
            return
        end
    end

    -- Default
    gStartProjectName = FirstTarget
end

-- Generate workspace
function GenerateWorkspace()

    -- Logging
    LogInfo("--- Generating Workspace '%s' ---", gWorkspaceName)
    LogInfo("ConfigurationPath = '%s'", GetOutputConfigPath())

    if gTargets == nil then
        LogError("TargetNames cannot be nil")
        return
    end

    if #gTargets < 1 then
        LogError("Workspace must contain at least one target (Current=%d)", #gTargets)
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

    -- Include the target scripts and collect the type of targets we will have in the workspace
    for i = 1, #gTargets do
        local CurrentTargetName = gTargets[i]
        
        local TargetInfo = GetIndexedTargetInfo and GetIndexedTargetInfo(CurrentTargetName) or nil
        if TargetInfo and os.isfile(TargetInfo.ScriptPath) then
            local ExistingRule = GetTargetRule and GetTargetRule(CurrentTargetName) or nil
            if ExistingRule then
                AddTargetType(ExistingRule.TargetType)
            else
                LogInfo("Including script '%s' to include target '%s'", CreateOsPath(TargetInfo.ScriptPath), CurrentTargetName)
                include(TargetInfo.ScriptPath)

                local CreatedRule = GetTargetRule and GetTargetRule(CurrentTargetName) or nil
                if CreatedRule then
                    LogInfo("Target '%s' was created in script '%s'", CreatedRule.Name or CurrentTargetName, CreateOsPath(TargetInfo.ScriptPath))
                    AddTargetType(CreatedRule.TargetType)
                else
                    LogHighlightWarning("Found target '%s' at '%s', but it did not register.", CurrentTargetName, CreateOsPath(TargetInfo.ScriptPath))
                end
            end
        else
            LogError("Target '%s' not found in indexed roots. Ensure it lives under a configured search root.", CurrentTargetName)
        end
    end

    -- Create all configurations that we need
    for i = 1, #gUsedTargetTypes do
        local CurrentTargetType = gUsedTargetTypes[i]
        if CurrentTargetType == ETargetType.Game then
            LogHighlight("Need configuration for ETargetType.Game")

            table.insert(gConfigurations, "Debug")
            table.insert(gConfigurations, "Development")
            table.insert(gConfigurations, "Release")
            table.insert(gConfigurations, "Debug Monolithic")
            table.insert(gConfigurations, "Development Monolithic")
            table.insert(gConfigurations, "Release Monolithic") 
        elseif CurrentTargetType == ETargetType.Editor then
            LogHighlight("Need configuration for ETargetType.Editor")

            table.insert(gConfigurations, "Debug Editor")
            table.insert(gConfigurations, "Development Editor")
            table.insert(gConfigurations, "Release Editor")
        elseif CurrentTargetType == ETargetType.Program then
            LogHighlight("Need configuration for ETargetType.Program")
            -- TODO
        end
    end

    for i = 1, #gConfigurations do
        local ConfigName = gConfigurations[i]
        LogHighlight("  Use config '%s'", ConfigName)
    end

    -- Execute targets (include their scripts)
    for i = 1, #gTargets do
        local CurrentTargetName = gTargets[i]

        local CurrentTargetRule = GetTargetRule and GetTargetRule(CurrentTargetName) or nil
        if CurrentTargetRule then
            if not CurrentTargetRule.IsGenerated or not CurrentTargetRule.IsGenerated() then
                LogInfo("Generating Target '%s'.", CurrentTargetName)
                gCurrentTargetName = CurrentTargetName
                CurrentTargetRule.Generate()
            else
                LogHighlightWarning("Target '%s' is already generated in workspace '%s'", CurrentTargetName, GetWorkspaceName())
            end
        else
            LogError("Target '%s' does not exist. Check under a valid search root and that it gets created in the script.", CurrentTargetName)
        end
    end

    gCurrentTargetName = ""

    -- Decide startup project based on what actually got registered
    ComputeStartProjectName()

    -- Generate the actual solution files
    GenerateSolutionFiles()

    LogInfo("--- Finished generating workspace ---")
end
