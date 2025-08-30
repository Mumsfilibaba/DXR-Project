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
    allowed     = { { "Windows" }, { "macOS" } },
    default     = "Windows"
}

-- Global settings
if type(gSettings) ~= "table" then
    gSettings = {}
end

if gSettings.bEnableDebugLogging == nil then
    gSettings.bEnableDebugLogging = false
end

-- Monolithic Build Management
local gIsMonolithic = false

-- Check if the module should be built monolithically
function IsBuildMonolithic()
    if gIsMonolithic == nil then
        gIsMonolithic = (_OPTIONS["monolithic"] ~= nil)
    end

    return gIsMonolithic
end

-- Check if the current platform is Windows
function IsPlatformWindows()
    return _OPTIONS["platform"] == "Windows"
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
local gModules = {}

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

-- Output path for the binaries inside the buildfolder
local gOutputConfigPath = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"

function GetOutputConfigPath()
    return gOutputConfigPath
end

-- Make path relative to the thirdparty folder
function CreateExternalThirdpartyPath(ThirdpartyPath)
    return JoinPath(GetExternalThirdpartyFolderPath(), ThirdpartyPath)
end

-- Deep copy a table
function Copy(Source)
    return table.deepcopy(Source)
end

-- Module indexing (scan without executing Module.lua)
local gModuleIndex = {}
local gModuleIndexScanned = false
local gModuleSearchRoots = {}

function AddModuleSearchRoot(RootPath)
    if type(RootPath) ~= "string" or RootPath == "" then
        LogError("AddModuleSearchRoot: invalid root")
        return
    end

    local NormalizedRootPath = CreateOsPath(RootPath)
    for ExistingIndex, ExistingRoot in ipairs(gModuleSearchRoots) do
        if string.lower(ExistingRoot) == string.lower(NormalizedRootPath) then
            return -- avoid duplicates
        end
    end

    table.insert(gModuleSearchRoots, NormalizedRootPath)
end

local function IsPathPrefix(ChildPath, ParentPath)
    local ChildPathLower  = string.lower(CreateOsPath(ChildPath))
    local ParentPathLower = string.lower(CreateOsPath(ParentPath))

    if #ChildPathLower < #ParentPathLower then
        return false
    end
    if ChildPathLower:sub(1, #ParentPathLower) ~= ParentPathLower then
        return false
    end
    -- exact match or next char is a separator
    if #ChildPathLower == #ParentPathLower then
        return true
    end
    local NextCharAfterPrefix = ChildPathLower:sub(#ParentPathLower + 1, #ParentPathLower + 1)
    return (NextCharAfterPrefix == '\\' or NextCharAfterPrefix == '/')
end

local function IndexModuleFile(ScriptFilePath)
    local FileHandle = io.open(ScriptFilePath, "r")
    if not FileHandle then
        return
    end

    local SourceCode = FileHandle:read("*a")
    FileHandle:close()

    -- Find all ModuleBuildRules("Name") occurrences.
    -- Name allows letters, digits, '_', '-', '+', '.'
    for ModuleName in SourceCode:gmatch("ModuleBuildRules%s*%(%s*[%\"']([%w_%-%+%.]+)[%\"']%s*%)") do
        local ScriptDirectory = path.getdirectory(ScriptFilePath)
        local RootLabel = "ThirdParty"
        if IsPathPrefix(ScriptDirectory, GetRuntimeFolderPath()) then
            RootLabel = "Runtime"
        end

        gModuleIndex[ModuleName] = {
            ScriptPath = ScriptFilePath,
            ScriptDir  = ScriptDirectory,
            Root       = RootLabel
        }

        if _G.gSettings and _G.gSettings.bEnableDebugLogging then
            LogInfo("Indexed module '%s' at '%s' (Root=%s)", ModuleName, ScriptFilePath, RootLabel)
        end
    end
end

local function ScanRoot(RootDirectory)
    local SearchPattern = CreateOsPath(path.join(RootDirectory, "**/Module.lua"))
    local MatchedFiles  = os.matchfiles(SearchPattern)

    for FileIndex, ScriptFilePath in ipairs(MatchedFiles) do
        LogHighlight("Found module-file '%s'", ScriptFilePath)
        IndexModuleFile(ScriptFilePath)
    end
end

function SearchForModuleFiles()
    if gModuleIndexScanned then
        return
    end

    for RootIndex, RootDirectory in ipairs(gModuleSearchRoots) do
        if os.isdir(RootDirectory) then
            LogHighlight("Scanning directory '%s'", RootDirectory)
            ScanRoot(RootDirectory)
        end
    end

    gModuleIndexScanned = true
end

function GetIndexedModuleInfo(ModuleName)
    return gModuleIndex[ModuleName]
end

function InvalidateModuleIndex()
    gModuleIndex = {}
    gModuleIndexScanned = false
end
