include "build_log.lua"

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

-- Check if the module should be built monolithicly
function IsMonolithic()
    if GbIsMonolithic == nil then
        GbIsMonolithic = (_OPTIONS["monolithic"] ~= nil)
    end
    
    return GbIsMonolithic
end

-- Check if the current platform to build for is Win32
function IsPlatformWindows()
    return _OPTIONS["platform"] == "Win32" 
end

-- Check if the current platform to build for is MacOS
function IsPlatformMac()
    return _OPTIONS["platform"] == "macOS" 
end

-- Check the action being used
function BuildWithXcode()
    return _ACTION == "xcode4"
end

function BuildWithVisualStudio()
    return 
        _ACTION == "vs2022" or 
        _ACTION == "vs2019" or 
        _ACTION == "vs2017" or 
        _ACTION == "vs2015" or 
        _ACTION == "vs2013" or
        _ACTION == "vs2012" or
        _ACTION == "vs2010" or
        _ACTION == "vs2008" or
        _ACTION == "vs2005"
end

-- Verify language version
function VerifyLanguageVersion(LanguageVersion)
    return
        LanguageVersion == "c++98" or
        LanguageVersion == "c++11" or
        LanguageVersion == "c++14" or
        LanguageVersion == "c++17" or 
        LanguageVersion == "c++20" or
        LanguageVersion == "latest"
end

-- Helper for printing all strings in a table and ending with endline
function PrintTable(Format, Table)
    if Table == nil then
        return
    end

    if #Table >= 1 then
        for Index = 1, #Table do
            LogInfo(Format, Table[Index])
        end
    end
end

-- Helper to appending multiple elements to a table
function AddUniqueElements(Elements, Table)
    if Table == nil then
        return
    end

    if Elements ~= nil then
        for i = 1, #Elements do
            local bIsUnique = true
            for j = 1, #Table do
                if Table[j] == Elements[i] then
                    bIsUnique = false
                    break
                end
            end
            
            if bIsUnique then
                Table[#Table + 1] = Elements[i]
            end
        end
    end
end


-- Global variable that stores all created modules
GModules = { }

function GetModule(ModuleName)
    if GModules ~= nil then
        return GModules[ModuleName]
    else
        LogError("Global Module-List has not been initialized")
        return nil
    end
end

function IsModule(ModuleName)
    return GetModule(ModuleName) ~= nil  
end

function AddModule(ModuleName, Module)
    if GModules ~= nil then
        GModules[ModuleName] = Module
    else
        LogError("Global Module-List has not been initialized")
    end
end

_GSeperatorForOS = ''
if IsPlatformWindows() then
    _GSeperatorForOS = '\\'
else
    _GSeperatorForOS = '/'
end

function CreateOSPath(InPath)
    return path.translate(InPath, _GSeperatorForOS)
end

-- Mainpath ../BuildScripts/Premake
GEnginePath = CreateOSPath(path.getabsolute( "../../", _PREMAKE_DIR))

function GetEnginePath()
    return GEnginePath
end

-- Join two paths
function JoinPath(Path0, Path1)
    return CreateOSPath(path.join(Path0, Path1))
end

-- Retrieve the path to the Runtime folder containing all the engine modules
_GRuntimeFolderPath = JoinPath(GEnginePath, "Runtime")
function GetRuntimeFolderPath()
    return _GRuntimeFolderPath
end

-- Retrieve the path to the solutions folder containing solution and project files
_GSolutionsFolderPath = JoinPath(GEnginePath, "Solutions")
function GetSolutionsFolderPath()
    return _GSolutionsFolderPath
end

-- Retrieve the path to the dependencies folder containing external dependecy projects
_GExternalDependenciesFolderPath = JoinPath(GEnginePath, "Dependencies")
function GetExternalDependenciesFolderPath()
    return _GExternalDependenciesFolderPath
end

-- Make path relative to dependency folder
function CreateExternalDependencyPath(Path)
    return JoinPath(GetExternalDependenciesFolderPath(), Path)
end


-- Deep copy a table
function Copy(Source)
    return table.deepcopy(Source)
end