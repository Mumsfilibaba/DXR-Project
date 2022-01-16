-- Custom options 
newoption 
{
	trigger     = "monolithic",
	description = "Links all modules as static libraries instead of DLLs"
}

-- Check if the module should be built monolithicly
function IsMonolithic()
    if GbIsMonolithic == nil then
        GbIsMonolithic = (_OPTIONS['monolithic'] ~= nil)
    end
    
    return GbIsMonolithic
end

-- Sets the global state IsMonolithic, the project creation overrides this variable
function SetIsMonlithic( bIsMonolithic )
    -- Ensures that global variable is created
    local bCurrent = IsMonolithic()
    if bCurrent ~= bIsMonolithic then
        GbIsMonolithic = bIsMonolithic
    end
end

-- Check the action being used
function BuildWithXcode()
    return _ACTION == 'xcode4'
end

function BuildWithVS()
    return 
        _ACTION == 'vs2022' or 
        _ACTION == 'vs2019' or 
        _ACTION == 'vs2017' or 
        _ACTION == 'vs2015' or 
        _ACTION == 'vs2013' or
        _ACTION == 'vs2012' or
        _ACTION == 'vs2010' or
        _ACTION == 'vs2008' or
        _ACTION == 'vs2005'
end

-- Helper for printing all strings in a table and ending with endline
function PrintTableWithEndLine(Format, Table)
    if Table == nil then
        return
    end

    if #Table >= 1 then
        for Index = 1, #Table do
            printf(Format, Table[Index])
        end

        -- Empty line
        printf('')
    end
end

-- Helper appending an element to a table
function TableAppendUniqueElement(Element, Table)
    if Table == nil then
        return
    end

    if Element ~= nil then
        for i = 1, #Table do
            if Table[i] == Element then
                return
            end
        end
        
        Table[#Table + 1] = Element
    end
end

-- Helper to appending multiple elements to a table
function TableAppendUniqueElementMultiple(Elements, Table)
    if Table == nil then
        return
    end

    if Elements ~= nil then
        for i = 1, #Elements do
            local bIsUnique = true
            for j = 1, #Table do
                if Table[j] == Elements[i] then
                    bIsUnique = false
                end
            end
            
            if bIsUnique then
                Table[#Table + 1] = Elements[i]
            end
        end
    end
end

-- Global variable that stores all created modules
GModules = {}

function GetModule(ModuleName)
    if GModules ~= nil then
        return GModules[ModuleName]
    else
        printf('Global list of modules does not exist')
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
        printf('Global list of modules does not exist')
    end
end

-- Global variable that stores all created targets
GTargets = {}

function GetTarget(TargetName)
    if GTargets ~= nil then
        return GTargets[TargetName]
    else
        printf('Global list of targets does not exist')
        return nil
    end
end

function IsTarget(TargetName)
    return GetTarget(TargetName) ~= nil  
end

function AddTarget(TargetName, Target)
    if GTargets ~= nil then
        GTargets[TargetName] = Target
    else
        printf('Global list of targets does not exist')
    end
end

-- Output path for dependencies (ImGui etc.)
GOutputPath = '%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}'
printf('\nINFO:\nBuildPath = \'%s\'', GOutputPath)

function GetOutputPath()
    return GOutputPath
end

-- Mainpath ../BuildScripts
GEnginePath = path.getabsolute( '../', _PREMAKE_DIR)  
printf('EnginePath = \'%s\'\n', GEnginePath)

function GetEnginePath()
    return GEnginePath
end

-- Retrieve the workspace directory
function FindWorkspaceDir()
	return GEnginePath
end

-- Retrieve the path to the Runtime folder containing all the engine modules
function GetRuntimeFolderPath()
    return GEnginePath .. '/Runtime'
end

-- Retrieve the path to the solutions folder containing solution and project files
function GetSolutionsFolderPath()
    return GEnginePath .. '/Solutions'
end

-- Retrieve the path to the dependencies folder containing external dependecy projects
function GetExternalDependenciesFolderPath()
    return GEnginePath .. '/Dependencies'
end

-- Make path relative to dependency folder
function MakeExternalDependencyPath(Path)
    return GetExternalDependenciesFolderPath() .. '/' .. Path
end

-- Deep copy a table
function Copy(Source)
    return table.deepcopy(Source)
end