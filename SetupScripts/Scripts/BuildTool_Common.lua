include "BuildTool_Log.lua"

-- Custom options
newoption
{
    trigger = "monolithic",
    description = "Links all modules as static libraries instead of DLLs"
}

newoption
{
    trigger = "platform",
    value = "CurrentPlatform",
    description = "Specify the platform to use",
    allowed = { 
        { "Windows" },
        { "macOS" }
    },
}

newoption
{
    trigger = "fatalwarnings",
    description = "Treat compiler warnings as errors (thirdparty modules are exempt)"
}

newoption
{
    trigger = "buildsuffix",
    value = "Name",
    description = "Isolate generated projects and build artifacts under a named suffix"
}

newoption
{
    trigger = "architecture",
    value = "TargetArchitecture",
    description = "Specify the CPU architecture to build for",
    allowed = {
        { "x86_64" },
        { "arm64" },
        { "universal" }
    },
}

local function NormalizePlatform(PlatformName)
    if not PlatformName then
        return nil
    end

    PlatformName = tostring(PlatformName):lower()
    if PlatformName == "windows" then
        return "Windows"
    end
    if PlatformName == "macos" or PlatformName == "macosx" or PlatformName == "osx" then
        return "macOS"
    end

    return PlatformName
end

local function GuessPlatformFromAction()
    if _ACTION == "xcode4" then 
        return "macOS"
    end
    if _ACTION and _ACTION:match("^vs") then
        return "Windows"
    end

    return nil
end

local function GuessPlatformFromHost()
    
    -- "windows", "macosx", "linux", ...
    local Host = os.host()
    if Host == "windows" then
        return "Windows"
    end
    
    if Host == "macosx" then
        return "macOS"
    end

    return nil
end

-- Initialize default if missing or unrecognized
do
    local Explicit = NormalizePlatform(_OPTIONS["platform"])
    if not Explicit then
        _OPTIONS["platform"] = GuessPlatformFromAction() or GuessPlatformFromHost() or "Windows"
        LogHighlight("No --platform specified. Defaulting to '%s'.", _OPTIONS["platform"])
    else
        _OPTIONS["platform"] = Explicit
    end

    -- sanity check against your allowed set
    local Allowed = {
        Windows = true,
        ["macOS"] = true
    }

    if not Allowed[_OPTIONS["platform"]] then
        LogWarning("Unsupported --platform '%s'. Falling back to 'Windows'.", tostring(_OPTIONS["platform"]))
        _OPTIONS["platform"] = "Windows"
    end
end

-- Use this everywhere else:
function IsPlatformWindows()
    return _OPTIONS["platform"] == "Windows"
end

function IsPlatformMac()
    return _OPTIONS["platform"] == "macOS"
end

-- Architecture Management
local function NormalizeArchitecture(ArchitectureName)
    if not ArchitectureName then
        return nil
    end

    ArchitectureName = tostring(ArchitectureName):lower()
    if ArchitectureName == "x86_64" or ArchitectureName == "x64" or ArchitectureName == "amd64" then
        return "x86_64"
    end
    if ArchitectureName == "arm64" or ArchitectureName == "aarch64" then
        return "arm64"
    end
    if ArchitectureName == "universal" then
        return "universal"
    end

    return ArchitectureName
end

-- The bundled premake exposes no os.hostarch(), so uname is the only source here.
local function GuessArchitectureFromHost()
    if os.host() ~= "macosx" then
        return "x86_64"
    end

    local Machine = os.outputof("uname -m")
    return NormalizeArchitecture(Machine and Machine:gsub("%s+", "")) or "x86_64"
end

-- Initialize default if missing or unrecognized
do
    local Explicit = NormalizeArchitecture(_OPTIONS["architecture"])
    if not Explicit then
        _OPTIONS["architecture"] = GuessArchitectureFromHost()
        LogHighlight("No --architecture specified. Defaulting to '%s'.", _OPTIONS["architecture"])
    else
        _OPTIONS["architecture"] = Explicit
    end

    -- A fat binary is a Mach-O concept; nothing equivalent exists on Windows.
    if _OPTIONS["architecture"] == "universal" and not IsPlatformMac() then
        LogWarning("--architecture=universal is macOS only. Falling back to 'x86_64'.")
        _OPTIONS["architecture"] = "x86_64"
    end
end

function GetTargetArchitecture()
    return _OPTIONS["architecture"]
end

function IsArchitectureUniversal()
    return GetTargetArchitecture() == "universal"
end

-- True when at least one slice is an x86 target, so x86-only compiler settings still apply
function TargetsX86()
    local Architecture = GetTargetArchitecture()
    return Architecture == "x86_64" or Architecture == "universal"
end

local gArchitecturePlatformNames =
{
    ["x86_64"]    = "x64",
    ["arm64"]     = "ARM64",
    ["universal"] = "Universal",
}

function GetArchitecturePlatformName()
    return gArchitecturePlatformNames[GetTargetArchitecture()]
end

local gPremakeArchitectures =
{
    ["x86_64"]    = "x86_64",
    ["arm64"]     = "ARM64",
    ["universal"] = "universal",
}

function GetPremakeArchitecture()
    return gPremakeArchitectures[GetTargetArchitecture()]
end

-- The xcode4 exporter emits no ARCHS of its own, so this list alone decides the slices built
local gXcodeArchitectures =
{
    ["x86_64"]    = { "x86_64" },
    ["arm64"]     = { "arm64" },
    ["universal"] = { "x86_64", "arm64" },
}

function GetXcodeArchs()
    return gXcodeArchitectures[GetTargetArchitecture()]
end

-- Monolithic Build Management
local gIsMonolithic = nil

function IsBuildMonolithic()
    if gIsMonolithic == nil then
        gIsMonolithic = (_OPTIONS["monolithic"] ~= nil)
    end

    return gIsMonolithic
end

ELayout = 
{ 
    Modular    = 1, 
    Monolithic = 2 
}

function GetGeneratedLayouts()
    if IsBuildMonolithic() then
        return { ELayout.Monolithic }
    end

    if BuildWithVisualStudio() then
        return { ELayout.Modular, ELayout.Monolithic }
    end

    return { ELayout.Modular }
end

-- True when the configuration names decide the layout rather than the generation
function HasPerConfigurationLayouts()
    return #GetGeneratedLayouts() > 1
end

function GetLayoutConfigFilter(Layout)
    if not HasPerConfigurationLayouts() then
        return nil
    end

    return Layout == ELayout.Monolithic and "configurations:*Monolithic*"
                                         or "configurations:not *Monolithic*"
end

-- Warning Management
function IsFatalWarnings()
    return _OPTIONS["fatalwarnings"] ~= nil
end

-- Suffix that keeps a generation from colliding with the solution and binaries of a normal one
local function GetBuildSuffix()
    local Suffix = _OPTIONS["buildsuffix"]
    if type(Suffix) ~= "string" or Suffix == "" then
        return nil
    end

    return Suffix
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
function PrintTable(FormatStr, Table)
    if Table == nil then 
        return 
    end

    for _, Value in ipairs(Table) do
        LogInfo(FormatStr, Value)
    end
end

-- Helper to append multiple unique elements to a table
function AddUniqueElements(Elements, Table)
    if Table == nil or Elements == nil then 
        return 
    end

    local ElementSet = {}
    for _, Value in ipairs(Table) do
        ElementSet[Value] = true
    end
    for _, Value in ipairs(Elements) do
        if not ElementSet[Value] then
            table.insert(Table, Value)
            ElementSet[Value] = true
        end
    end
end

-- Returns the elements of Elements that do not appear in Excluded
function ExcludeElements(Elements, Excluded)
    if Elements == nil then
        return {}
    end

    if Excluded == nil then
        return Elements
    end

    local ExcludedSet = {}
    for _, Value in ipairs(Excluded) do
        ExcludedSet[Value] = true
    end

    local Result = {}
    for _, Value in ipairs(Elements) do
        if not ExcludedSet[Value] then
            table.insert(Result, Value)
        end
    end

    return Result
end

-- Module management functions
local gModuleRules = {}

function GetModuleRule(ModuleName)
    return gModuleRules[ModuleName]
end

function IsModuleRule(ModuleName)
    return gModuleRules[ModuleName] ~= nil
end

function AddModuleRule(ModuleName, ModuleRule)
    gModuleRules[ModuleName] = ModuleRule
end

-- Target management functions
local gTargetRules = {}

function GetTargetRule(TargetName)
    return gTargetRules[TargetName]
end

function IsTargetRule(TargetName)
    return gTargetRules[TargetName] ~= nil
end

function AddTargetRule(TargetName, TargetRule)
    gTargetRules[TargetName] = TargetRule
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

-- Join two or more paths
function JoinPath(...)
    return CreateOsPath(path.join(...))
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
if GetBuildSuffix() then
    gSolutionsFolderPath = JoinPath(gSolutionsFolderPath, GetBuildSuffix())
end

function GetSolutionsFolderPath()
    return gSolutionsFolderPath
end

-- Retrieve the path to the ThirdParty folder containing external thirdparty projects
local gExternalThirdPartyFolderPath = JoinPath(gEnginePath, "ThirdParty")

function GetExternalThirdPartyFolderPath()
    return gExternalThirdPartyFolderPath
end

-- Output path for the binaries inside the buildfolder
local gOutputConfigPath = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"

-- When the configuration name carries the layout there is nothing to disambiguate. When it
-- does not, a monolithic generation would otherwise overwrite the modular binaries.
if IsBuildMonolithic() and not HasPerConfigurationLayouts() then
    gOutputConfigPath = gOutputConfigPath .. "-Monolithic"
end

if GetBuildSuffix() then
    gOutputConfigPath = gOutputConfigPath .. "-" .. GetBuildSuffix()
end

function GetOutputConfigPath()
    return gOutputConfigPath
end

-- Make path relative to the thirdparty folder
function CreateExternalThirdpartyPath(ThirdpartyPath)
    return JoinPath(GetExternalThirdPartyFolderPath(), ThirdpartyPath)
end

-- Shared local helpers
local function NormalizePath(Path)
    Path = CreateOsPath(Path or "")
    Path = Path:gsub("[/\\]+$", "") -- remove trailing slash(es)
    return Path:lower()
end

local function PathIsUnder(ChildPath, ParentPath)
    local NormChild  = NormalizePath(ChildPath)
    local NormParent = NormalizePath(ParentPath)
    if NormChild == NormParent then
        return true
    end

    return NormChild:sub(1, #NormParent + 1) == (NormParent .. "\\")
        or NormChild:sub(1, #NormParent + 1) == (NormParent .. "/")
end

local function StripLuaComments(Source)
    return Source
        :gsub("%-%-%[%[.-%]%]", "")  -- block
        :gsub("%-%-.-\n", "\n")      -- line to EOL
        :gsub("%-%-.*$", "")         -- line at EOF (no trailing \n)
end

local function ResolveAbsolutePath(InputPath)
    -- Resolve relative to the current working directory (Premake)
    return path.isabsolute(InputPath) and InputPath or path.getabsolute(InputPath)
end

-- Module indexing (scans for Module.lua files without executing)
local gModuleIndex = {}
local gModuleIndexScanned = false
local gModuleSearchRoots = {}

function AddModuleSearchRoot(RootPath)
    if type(RootPath) ~= "string" or RootPath == "" then
        LogError("AddModuleSearchRoot: invalid root")
        return
    end

    -- Resolve relative -> absolute, then build a normalized key for comparison
    local AbsolutePath = ResolveAbsolutePath(RootPath)
    local NewKey       = NormalizePath(AbsolutePath)

    local StoredPath = CreateOsPath(AbsolutePath):gsub("[/\\]+$", "")

    -- Dedupe using normalized keys (case/sep-insensitive)
    for _, ExistingRoot in ipairs(gModuleSearchRoots) do
        if NormalizePath(ExistingRoot) == NewKey then
            LogHighlightWarning("AddModuleSearchRoot: '%s' already present. Skipping ..", StoredPath)
            return
        end
    end

    table.insert(gModuleSearchRoots, StoredPath)

    -- new root -> enable (re)scan
    gModuleIndexScanned = false

    LogInfo("AddModuleSearchRoot: added '%s' (from '%s')", StoredPath, RootPath)
end

local function IndexModuleFile(ScriptFilePath, SearchRootDir)
    local File = io.open(ScriptFilePath, "r")
    if not File then
        return
    end

    local Source = StripLuaComments(File:read("*a"))
    File:close()

    local ScriptDir = path.getdirectory(ScriptFilePath)
    local RootLabel = path.getname(CreateOsPath(SearchRootDir))

    if not PathIsUnder(ScriptDir, SearchRootDir) then
        LogHighlightWarning("Module file '%s' not under declared search root '%s' (labeling as '%s').", CreateOsPath(ScriptFilePath), CreateOsPath(SearchRootDir), RootLabel)
    end

    -- Enforce matching quotes via %1, same as target version
    for _, ModuleName in Source:gmatch("ModuleBuildRules%s*%(%s*([\"'])([%w_%-%+%.]+)%1%s*%)") do
        local Previous = gModuleIndex[ModuleName]
        if Previous and Previous.ScriptPath ~= ScriptFilePath then
            LogHighlightWarning("Module '%s' defined in multiple files:\n  %s\n  %s\nUsing first one.", ModuleName, CreateOsPath(Previous.ScriptPath), CreateOsPath(ScriptFilePath))
        else
            gModuleIndex[ModuleName] = {
                ScriptPath = ScriptFilePath,
                ScriptDir  = ScriptDir,
                Root       = RootLabel,
                RootDir    = CreateOsPath(SearchRootDir)
            }

            LogInfo("Indexed module '%s' at '%s' (Root=%s)", ModuleName, CreateOsPath(ScriptFilePath), RootLabel)
        end
    end
end

local function ScanModuleRoot(RootDirectory)
    local Base = path.translate(RootDirectory, '/')

    local Patterns = {
        Base .. "/Module.lua",
        Base .. "/**/Module.lua",
    }

    local Seen = {}
    for _, Pattern in ipairs(Patterns) do
        local Files = os.matchfiles(Pattern)
        for _, ScriptPath in ipairs(Files) do
            local Key = string.lower(path.translate(ScriptPath, '/'))
            if not Seen[Key] then
                Seen[Key] = true
                LogHighlight("Found module-file '%s'", CreateOsPath(ScriptPath))
                IndexModuleFile(ScriptPath, RootDirectory)
            end
        end
    end
end

local function SearchForModuleFiles()
    if gModuleIndexScanned then
        return
    end

    InvalidateModuleIndex()

    table.sort(gModuleSearchRoots, function(ValA, ValB) return ValA:lower() < ValB:lower() end)

    for _, RootDirectory in ipairs(gModuleSearchRoots) do
        if os.isdir(RootDirectory) then
            LogHighlight("Scanning directory '%s'", CreateOsPath(RootDirectory))
            ScanModuleRoot(RootDirectory)
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

-- Target indexing (scans for Target.lua files without executing)
local gTargetIndex = {}
local gTargetIndexScanned = false
local gTargetSearchRoots = {}

function AddTargetSearchRoot(RootPath)
    if type(RootPath) ~= "string" or RootPath == "" then
        LogError("AddTargetSearchRoot: invalid root")
        return
    end

    -- Resolve relative -> absolute, then build a normalized key for comparison
    local AbsolutePath = ResolveAbsolutePath(RootPath)
    local NewKey       = NormalizePath(AbsolutePath)

    local StoredPath = CreateOsPath(AbsolutePath):gsub("[/\\]+$", "")

    -- Dedupe using normalized keys (case/sep-insensitive)
    for _, ExistingRoot in ipairs(gTargetSearchRoots) do
        if NormalizePath(ExistingRoot) == NewKey then
            LogHighlightWarning("AddTargetSearchRoot: '%s' already present. Skipping ..", StoredPath)
            return
        end
    end

    table.insert(gTargetSearchRoots, StoredPath)

    -- new root -> enable (re)scan
    gTargetIndexScanned = false

    LogInfo("AddTargetSearchRoot: added '%s' (from '%s')", StoredPath, RootPath)
end

local function IndexTargetFile(ScriptFilePath, SearchRootDir)
    local File = io.open(ScriptFilePath, "r")
    if not File then
        return
    end

    local Source = StripLuaComments(File:read("*a"))
    File:close()

    local ScriptDir = path.getdirectory(ScriptFilePath)
    local RootLabel = path.getname(CreateOsPath(SearchRootDir))

    if not PathIsUnder(ScriptDir, SearchRootDir) then
        LogHighlightWarning("Target file '%s' not under declared search root '%s' (labeling as '%s').", CreateOsPath(ScriptFilePath), CreateOsPath(SearchRootDir), RootLabel )
    end

    -- Enforce matching quotes via %1
    for _, TargetName in Source:gmatch("TargetBuildRules%s*%(%s*([\"'])([%w_%-%+%.]+)%1%s*%)") do
        local Previous = gTargetIndex[TargetName]
        if Previous and Previous.ScriptPath ~= ScriptFilePath then
            LogHighlightWarning("Target '%s' defined in multiple files:\n  %s\n  %s\nUsing first one.", TargetName, CreateOsPath(Previous.ScriptPath), CreateOsPath(ScriptFilePath))
        else
            gTargetIndex[TargetName] = {
                ScriptPath = ScriptFilePath,
                ScriptDir = ScriptDir,
                Root = RootLabel
            }

            LogInfo("Indexed target '%s' at '%s' (Root=%s)", TargetName, CreateOsPath(ScriptFilePath), RootLabel)
        end
    end
end

local function ScanTargetRoot(RootDirectory)
    local Base = path.translate(RootDirectory, '/')

    local Patterns = {
        Base .. "/Target.lua",
        Base .. "/**/Target.lua",
    }

    local Seen = {}
    for _, Pattern in ipairs(Patterns) do
        local Files = os.matchfiles(Pattern)
        for _, ScriptPath in ipairs(Files) do
            local Key = string.lower(path.translate(ScriptPath, '/'))
            if not Seen[Key] then
                Seen[Key] = true
                LogHighlight("Found target-file '%s'", CreateOsPath(ScriptPath))
                IndexTargetFile(ScriptPath, RootDirectory)
            end
        end
    end
end

local function SearchForTargetFiles()
    if gTargetIndexScanned then
        return
    end

    InvalidateTargetIndex()

    table.sort(gTargetSearchRoots, function(ValA, ValB) return ValA:lower() < ValB:lower() end)

    for _, RootDir in ipairs(gTargetSearchRoots) do
        if os.isdir(RootDir) then
            LogHighlight("Scanning directory '%s' for targets", CreateOsPath(RootDir))
            ScanTargetRoot(RootDir)
        end
    end

    gTargetIndexScanned = true
end

function GetIndexedTargetInfo(TargetName)
    return gTargetIndex[TargetName]
end

function InvalidateTargetIndex()
    gTargetIndex = {}
    gTargetIndexScanned = false
end

-- Function that ensures that we search for both module and target files
function SearchForBuildFiles()

    -- Start by searching for target files ..
    SearchForTargetFiles()

    -- .. then search for module files
    SearchForModuleFiles()
end
