include "BuildTool_Log.lua"

-- Custom options
newoption
{
    trigger     = "monolithic",
    description = "Links all modules as static libraries instead of DLLs"
}

newoption
{
    trigger     = "platform",
    value       = "CurrentPlatform",
    description = "Specify the platform to use",
    allowed     = { { "Win32" }, { "macOS" } },
    default     = "Win32"
}

-- Global settings
if type(gSettings) ~= "table" then
    gSettings = {}
end

if gSettings.bEnableDebugLogging == nil then
    gSettings.bEnableDebugLogging = false
end

-- Global variables
local gIsMonolithic
local gModules = {}

-- Check if the module should be built monolithically
function IsBuildMonolithic()
    if gIsMonolithic == nil then
        gIsMonolithic = (_OPTIONS["monolithic"] ~= nil)
    end

    return gIsMonolithic
end

-- Check if the current platform is Windows
function IsPlatformWindows()
    return _OPTIONS["platform"] == "Win32"
end

-- Check if the current platform is macOS
function IsPlatformMac()
    return _OPTIONS["platform"] == "macOS"
end

-- Check the action being used
function BuildWithXcode()
    return _ACTION == "xcode4"
end

-- Visual Studio actions (global-style table)
local gVsActions =
{
    vs2022 = true, vs2019 = true, vs2017 = true, vs2015 = true,
    vs2013 = true, vs2012 = true, vs2010 = true, vs2008 = true, vs2005 = true
}

function BuildWithVisualStudio()
    return gVsActions[_ACTION] == true
end

-- Verify language version
local gValidLanguageVersions =
{
    ["c++98"] = true, ["c++11"] = true, ["c++14"] = true,
    ["c++17"] = true, ["c++20"] = true, ["c++latest"] = true
}

function VerifyLanguageVersion(LanguageVersion)
    return gValidLanguageVersions[LanguageVersion] == true
end

-- Helper for printing all strings in a table and ending with endline
function PrintTable(FormatStr, Tbl)
    if Tbl == nil then 
        return 
    end

    for _, value in ipairs(Tbl) do
        LogInfo(FormatStr, value)
    end
end

-- Helper to append multiple unique elements to a table
function AddUniqueElements(Elements, Tbl)
    if Tbl == nil or Elements == nil then 
        return 
    end

    local element_set = {}
    for _, v in ipairs(Tbl) do
        element_set[v] = true
    end
    for _, v in ipairs(Elements) do
        if not element_set[v] then
            table.insert(Tbl, v)
            element_set[v] = true
        end
    end
end

-- Module management functions
function GetModule(ModuleName)
    return gModules[ModuleName]
end

function IsModule(ModuleName)
    return gModules[ModuleName] ~= nil
end

function AddModule(ModuleName, Module)
    gModules[ModuleName] = Module
end

-- Path handling
local gPathSeparator = IsPlatformWindows() and '\\' or '/'

function CreateOsPath(InPath)
    return path.translate(InPath, gPathSeparator)
end

-- Main paths
local gEnginePath = CreateOsPath(path.getabsolute("../../", _PREMAKE_DIR))

function GetEnginePath()
    return gEnginePath
end

-- Join two paths
function JoinPath(PathA, PathB)
    return CreateOsPath(path.join(PathA, PathB))
end

-- Retrieve the path to the Runtime folder containing all the engine modules
local gRuntimeFolderPath = JoinPath(gEnginePath, "Runtime")

function GetRuntimeFolderPath()
    return gRuntimeFolderPath
end

-- Retrieve the path of the engine 'Build' folder
local gBuildFolderPath = JoinPath(gEnginePath, "Build")
function GetBuildFolderPath()
    return gBuildFolderPath
end

-- Retrieve the path to the Solutions folder containing solution and project files
local gSolutionsFolderPath = JoinPath(gEnginePath, "Solutions")

function GetSolutionsFolderPath()
    return gSolutionsFolderPath
end

-- Retrieve the path to the ThirdParty folder containing external thirdparty projects
local gExternalThirdpartyFolderPath = JoinPath(gEnginePath, "ThirdParty")

function GetExternalThirdpartyFolderPath()
    return gExternalThirdpartyFolderPath
end

-- Make path relative to the thirdparty folder
function CreateExternalThirdpartyPath(ThirdpartyPath)
    return JoinPath(GetExternalThirdpartyFolderPath(), ThirdpartyPath)
end

-- Deep copy a table
function Copy(Source)
    return table.deepcopy(Source)
end
