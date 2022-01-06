-- Custom options 
newoption 
{
	trigger     = "monolithic",
	description = "Links all modules as static libraries instead of DLLs"
}

-- Check if the module should be built monolithicly 
function IsMonolithic()
    return _OPTIONS['monolithic'] ~= nil
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
local function PrintTableWithEndLine(Format, Table)
    if #Table >= 1 then
        for Index = 1, #Table do
            printf(Format, Table[Index])
        end

        -- Empty line
        printf('')
    end
end

-- Helper appending an element to a table
local function TableAppend(Elements, Table)
    if Table == nil then
        return
    end

    if Elements ~= nil then
        for i = 1, #Table do
            if Table[i] == Elements then
                return
            end
        end
        
        Table[#Table + 1] = Elements
    end
end

-- Helper to appending multiple elements to a table
local function TableAppendMultiple(Elements, Table)
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

local function AddFrameWorkExtension(Table)
    for Index = 1, #Table do
        Table[Index] = Table[Index] .. '.framework'
    end
end  

-- Global variable that stores all created modules
GModules = {}

-- Output path for dependencies (ImGui etc.)
GOutputPath = '%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}'
printf('\nINFO:\nBuildPath=%s', GOutputPath)

-- Mainpath ../BuildScripts
GBasePath = path.getabsolute( '../', _PREMAKE_DIR)  
printf('BasePath =%s\n', GBasePath)

-- Retrieve the workspace directory
function FindWorkspaceDir()
	return GBasePath
end

-- Retrieve the path to the Runtime folder containing all the engine modules
function GetRuntimeFolderPath()
    return GBasePath .. '/Runtime'
end

-- Retrieve the path to the solutions folder containing solution and project files
function GetSolutionsFolderPath()
    return GBasePath .. '/Solutions'
end

-- Retrieve the path to the dependencies folder containing external dependecy projects
function GetExternalDependenciesFolderPath()
    return GBasePath .. '/Dependencies'
end

-- Create a new Engine Module
function CreateModule(NewModuleName)
    -- Needs to have a valid modulename
    if NewModuleName == nil then
        return nil
    end

    -- New module
    local NewModule = {}
    
    -- Ensure that module does not already exist
    if GModules then
        if GModules[NewModuleName] then
            printf('Module is already created')
            return nil
        end

        GModules[NewModuleName] = NewModule;
    else
        printf('Could not find GModules\n')
    end

    printf('Creating Module %s', NewModuleName)

    -- Name. Must be the name of the folder as well or specify the location
    NewModule.Name = NewModuleName;

    -- Default path for solutions and project paths
    NewModule.Location = GetSolutionsFolderPath()

    -- Should the module be dynamic or static, this is overridden by monolithic build, which forces all modules to be linked statically
    NewModule.bIsDynamic = true

    -- Should the module use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
    NewModule.bUsePrecompiledHeaders = false

    -- Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), this makes compilation for all files native to the IOS and Mac platform
    NewModule.bCompileCppAsObjectiveCpp = true

    -- Location for the build
    NewModule.OutputPath = GBasePath .. '/%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}'

    -- Compile this module for the selected architecture
    NewModule.Architecture = 'x64'
    
    -- Compile with the warning level
    NewModule.Warnings = 'extra'

    -- How to handle c++ exceptions
    NewModule.Exceptionhandling = 'Off'

    -- Should the module enable runtime type information
    NewModule.bEnableRunTimeTypeInfo = false

    -- Floating point settings 
    NewModule.Floatingpoint = 'Fast'

    -- Enable vector extensions
    NewModule.VectorExtensions = 'SSE2'

    -- Enable Edit and Continue in Visual Studio
    NewModule.bEnableEditAndContinue = false

    -- Enable C++ intrinsics
    NewModule.bEnableIntrinsics = true
    
    -- Language of the module
    NewModule.Language   = 'C++'
    NewModule.CppVersion = 'C++17'

    -- Version of system SDK
    NewModule.SystemVersion = 'latest'

    -- Ascii or Unicode
    NewModule.Characterset = 'Ascii'

    -- Premake Flags
    NewModule.Flags = 
    { 
        'MultiProcessorCompile',
        'NoIncrementalLink',
    }

    -- System includes example: #include <vector>
    NewModule.SysIncludes = {}

    -- Forceinclude these files
    NewModule.ForceIncludes = {}

    -- Files to build compile into the module
    local RuntimeFolderPath = GetRuntimeFolderPath()
    NewModule.Files =
    { 
        (RuntimeFolderPath .. '/%{prj.name}/**.h'),
        (RuntimeFolderPath .. '/%{prj.name}/**.hpp'),
        (RuntimeFolderPath .. '/%{prj.name}/**.inl'),
        (RuntimeFolderPath .. '/%{prj.name}/**.c'),
        (RuntimeFolderPath .. '/%{prj.name}/**.cpp'),
        (RuntimeFolderPath .. '/%{prj.name}/**.hlsl'),
        (RuntimeFolderPath .. '/%{prj.name}/**.hlsli')	
    }

    -- Defines
    NewModule.Defines = {}

    -- We do not want to compile HLSL files so exclude them from project
    NewModule.ExcludeFiles =
    {
        "**.hlsl",
        "**.hlsli",
    }

    -- FrameWorks, only on macOS for now, should only list the names not .framework
    NewModule.FrameWorks = {}

    -- Engine Modules that this module depends on. There are 3 types, dynamic, which are loaded as DLL without automatic importing
    -- Modules using DLLs but are linked at link time (__declspec(dllimport)), and static modules
    -- Modules should be specified by the name their folder has in the Runtime folder
    NewModule.DynamicModuleDependencies = {}
    NewModule.ModuleDependencies        = {}
    NewModule.StaticModuleDependencies  = {}

    -- Extra libraries to link
    NewModule.LinkLibraries = {}

    -- Helper function for adding flags
    function NewModule:AddFlags(InFlags)
        TableAppendMultiple(InFlags, self.Flags)
    end

    -- Helper function for adding sys includes
    function NewModule:AddSysIncludes(InSysIncludes)
        TableAppendMultiple(InSysIncludes, self.SysIncludes)
    end

    -- Helper function for adding sys includes
    function NewModule:AddIncludes(InIncludes)
        TableAppendMultiple(InIncludes, self.Includes)
    end

    -- Helper function for adding files
    function NewModule:AddFiles(InFiles)
        TableAppendMultiple(InFiles, self.Files)
    end

    -- Helper function for adding exclude files
    function NewModule:AddExcludeFiles(InExcludeFiles)
        TableAppendMultiple(InExcludeFiles, self.ExcludeFiles)
    end

    -- Helper function for adding defines
    function NewModule:AddDefines(InDefines)
        TableAppendMultiple(InDefines, self.Defines)
    end

    -- Helper function for adding dynamic module dependencies
    function NewModule:AddDynamicModuleDependencies(InDynamicModules)
        TableAppendMultiple(InDynamicModules, self.DynamicModuleDependencies)
    end

    -- Helper function for adding a Libraries
    function NewModule:AddModuleDependencies(InModuleDependencies)
        TableAppendMultiple(InModuleDependencies, self.ModuleDependencies)
    end

    -- Helper function for adding a Library
    function NewModule:AddStaticModuleDependencies(InStaticDependencies)
        TableAppendMultiple(InStaticDependencies, self.StaticModuleDependencies)
    end

    -- Helper function for adding libraries
    function NewModule:AddLinkLibraries(InLibraries)
        TableAppendMultiple(InLibraries, self.LinkLibraries)
    end

    -- Helper function for adding forceincludes
    function NewModule:AddForceIncludes(InForceIncludes)
        TableAppendMultiple(InForceIncludes, self.ForceIncludes)
    end

    -- Helper function for adding system include directories
    function NewModule:AddSystemIncludeDirs(InSysIncludeDirs)
        TableAppendMultiple(InSysIncludeDirs, self.SysIncludes)
    end
    
    -- Check if the project has a module project
    function NewModule:HasApplicationModule()
        return self.Module ~= nil
    end

    -- Function that create premake project
    function NewModule:Generate()
        printf('Generating Module %s', self.Name)

        local RuntimeFolderPath = GetRuntimeFolderPath()
        self:AddSystemIncludeDirs(RuntimeFolderPath)

        -- Is the build monolithic
        local bIsMonolithic = IsMonolithic()

        if bIsMonolithic then
            printf('    Build is monolithic\n')
        else
            printf('    Build is NOT monolithic\n')

            -- Add IMPL define to dynamic modules
            if self.bIsDynamic then
                local UpperCaseName  = self.Name:upper()
                local ModuleImplName = UpperCaseName .. '_IMPL=(1)'
                self:AddDefines( ModuleImplName );
            end
        end

        -- Add framework extension
        AddFrameWorkExtension(self.FrameWorks)

        -- A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
        local AllDependencies = {}
        local LinkModules     = {}

        -- Dynamic modules
        for Index = 1, #self.DynamicModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.DynamicModuleDependencies[Index];

            if bIsMonolithic then
                LinkModules[#LinkModules + 1] = self.DynamicModuleDependencies[Index];
            end
        end

        -- Modules
        for Index = 1, #self.ModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.ModuleDependencies[Index];
            LinkModules[#LinkModules + 1]         = self.ModuleDependencies[Index];
        end

        -- Static Modules
        for Index = 1, #self.StaticModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.StaticModuleDependencies[Index];
            LinkModules[#LinkModules + 1]         = self.StaticModuleDependencies[Index];
        end

        -- Debug print   
        printf('    Num of FrameWorks=%d', #self.FrameWorks)
        PrintTableWithEndLine('    Using framework dependency %s', self.FrameWorks)
        
        printf('    Num of AllDependencies=%d', #AllDependencies)
        PrintTableWithEndLine('    Using module dependency %s', AllDependencies)
        
        printf('    Num of LinkModules=%d', #LinkModules)
        PrintTableWithEndLine('    Linking module %s', LinkModules)

        printf('    Num of LinkLibraries=%d', #self.LinkLibraries)
        PrintTableWithEndLine('    Linking External Library %s', self.LinkLibraries)
        
        printf('    Num of Files=%d', #self.Files)
        PrintTableWithEndLine('    Including File  %s', self.Files)

        -- Project
        project(self.Name)
            architecture(self.Architecture)
            warnings(self.Warnings)
            exceptionhandling(self.Exceptionhandling)

            if self.bEnableRunTimeTypeInfo then
                rtti 'On'
            else
                rtti 'Off'
            end

            floatingpoint(self.Floatingpoint)
            vectorextensions(self.VectorExtensions)

            if self.bEnableEditAndContinue then
                editandcontinue 'On'
            else
                editandcontinue 'Off'
            end

            if self.bEnableIntrinsics then
                intrinsics 'On'
            else
                intrinsics 'Off'
            end

            language(self.Language)
            cppdialect(self.CppVersion)
            systemversion(self.SystemVersion)
            characterset(self.Characterset)

            printf('    Generated Project-file Location %s\n', self.Location)
            location(self.Location)

            -- All targets except the dependencies
            targetdir(GBasePath .. '/Build/bin/' .. NewModule.OutputPath)
            objdir(GBasePath .. '/Build/bin-int/' .. NewModule.OutputPath)

            -- Build type. If the build type is dynamic we need to check for the monolithic type
            if self.bIsDynamic then
                filter 'not options:monolithic'
                    kind 'SharedLib'
                filter {}
            
                filter 'options:monolithic'
                    kind 'StaticLib'
                filter {}
            else
                kind 'StaticLib'
            end

            -- Pre-Compiled Headers
            if self.bUsePrecompiledHeaders then
                pchheader 'PreCompiled.h'
                pchsource 'PreCompiled.cpp'

                printf('    Project is using PreCompiled Headers\n')

                self:AddForceIncludes('PreCompiled.h')
            else
                printf('    Project does NOT use PreCompiled Headers\n')
            end

            -- Add ForceIncludes
            PrintTableWithEndLine('    Using ForceInclude %s', self.ForceIncludes)

            forceincludes(self.ForceIncludes)

            -- Add System Includes
            PrintTableWithEndLine('    Using System Include Dir %s', self.SysIncludes)

            sysincludedirs(self.SysIncludes)

            -- Always add module name as a define
            self:AddDefines('MODULE_NAME=' .. '\"' .. self.Name .. '\"')

            -- Add Module Defines
            PrintTableWithEndLine('    Using define %s', self.Defines)

            defines(self.Defines)

            -- Add Module Files
            files(self.Files)

            -- In visual studio show natvis files
            filter 'action:vs*'
                vpaths { ['Natvis'] = '**.natvis' }
                
                files 
                {
                    '%{wks.location}/Runtime/%{prj.name}/**.natvis',
                }
            filter {}

            -- Remove files
            excludes(self.ExcludeFiles)

            -- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
            if self.bCompileCppAsObjectiveCpp then
                filter { 'system:macosx', 'files:**.cpp' }
                    compileas 'Objective-C++'
                filter {}
            end

            -- OS
            filter 'system:windows'
                removefiles
                {
                    '%{wks.location}/**/Mac/**',
                }
            filter {}

            filter 'system:macosx'
                removefiles
                {
                    '%{wks.location}/**/Windows/**',
                }
            filter {}

            -- Linking
            filter "system:macosx"
                links(self.FrameWorks)
            filter{}

            links(self.LinkLibraries)
            links(ModulesToLink)

            -- Dependencies
            dependson(Dependencies)
        project '*'
    end

    return NewModule
end

-- Create a new project
function CreateProject(NewProjectName)

    -- Project name must be valid
    if NewProjectName == nil then
        return nil
    end

    printf('CreateProject \'%s\'', NewProjectName)

    -- Create project
    local NewProject = {}
    NewProject.Name = NewProjectName

    -- Default path for solutions and project paths
    NewProject.Location = GetSolutionsFolderPath()

    -- Override the option of monolithic build
    NewProject.bIsMonolithic = false

    -- Console or windowed app
    NewProject.bIsConsoleApp = false
    
    -- Generate a module for the project, can be false if application should not be loadable on demand
    NewProject.bEnableApplicationModule = true
    NewProject.Module                   = nil

    -- Should the project use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
    NewProject.bUsePrecompiledHeaders = false

    -- Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), this makes compilation for all files native to the IOS and Mac platform
    NewProject.bCompileCppAsObjectiveCpp = true

    -- Location for the build
    NewProject.OutputPath = '%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}'

    -- Compile this module for the selected architecture
    NewProject.Architecture = 'x64'
    
    -- Compile with the warning level
    NewProject.Warnings = 'extra'

    -- How to handle c++ exceptions
    NewProject.Exceptionhandling = 'Off'

    -- Should the module enable runtime type information
    NewProject.bEnableRunTimeTypeInfo = false

    -- Floating point settings 
    NewProject.Floatingpoint = 'Fast'

    -- Enable vector extensions
    NewProject.VectorExtensions = 'SSE2'

    -- Enable Edit and Continue in Visual Studio
    NewProject.bEnableEditAndContinue = false

    -- Enable C++ intrinsics
    NewProject.bEnableIntrinsics = true
    
    -- Language of the module
    NewProject.Language   = 'C++'
    NewProject.CppVersion = 'C++17'

    -- Version of system SDK
    NewProject.SystemVersion = 'latest'

    -- Ascii or Unicode
    NewProject.Characterset = 'Ascii'

    -- Premake Flags
    NewProject.Flags = 
    { 
        'MultiProcessorCompile',
        'NoIncrementalLink',
    }

    -- System includes example: #include <vector>
    NewProject.SysIncludes = {}

    -- Forceinclude these files
    NewProject.ForceIncludes = {}

    -- Files to build compile into the module
    local RuntimeFolderPath = GetRuntimeFolderPath()
    NewProject.Files =
    { 
        (RuntimeFolderPath .. '/%{prj.name}/**.h'),
        (RuntimeFolderPath .. '/%{prj.name}/**.hpp'),
        (RuntimeFolderPath .. '/%{prj.name}/**.inl'),
        (RuntimeFolderPath .. '/%{prj.name}/**.c'),
        (RuntimeFolderPath .. '/%{prj.name}/**.cpp'),
        (RuntimeFolderPath .. '/%{prj.name}/**.hlsl'),
        (RuntimeFolderPath .. '/%{prj.name}/**.hlsli"')	
    }

    -- Defines
    NewProject.Defines = {}

    -- We do not want to compile HLSL files so exclude them from project
    NewProject.ExcludeFiles =
    {
        "**.hlsl",
        "**.hlsli",
    }

    -- FrameWorks, only on macOS for now, should only list the names not .framework
    NewProject.FrameWorks = {}

    -- Engine Modules that this module depends on. There are 3 types, dynamic, which are loaded as DLL without automatic importing
    -- Modules using DLLs but are linked at link time (__declspec(dllimport)), and static modules
    -- Modules should be specified by the name their folder has in the Runtime folder
    NewProject.DynamicModuleDependencies = {}
    NewProject.ModuleDependencies        = {}
    NewProject.StaticModuleDependencies  = {}

    -- Extra libraries to link
    NewProject.LinkLibraries = {}

    -- Helper function for adding flags
    function NewProject:AddFlags(InFlags)
        TableAppendMultiple(InFlags, self.Flags)
    end

    -- Helper function for adding sys includes
    function NewProject:AddSysIncludes(InSysIncludes)
        TableAppendMultiple(InSysIncludes, self.SysIncludes)
    end

    -- Helper function for adding sys includes
    function NewProject:AddIncludes(InIncludes)
        TableAppendMultiple(InIncludes, self.Includes)
    end

    -- Helper function for adding files
    function NewProject:AddFiles(InFiles)
        TableAppendMultiple(InFiles, self.Files)
    end

    -- Helper function for adding exclude files
    function NewProject:AddExcludeFiles(InExcludeFiles)
        TableAppendMultiple(InExcludeFiles, self.ExcludeFiles)
    end

    -- Helper function for adding defines
    function NewProject:AddDefines(InDefines)
        TableAppendMultiple(InDefines, self.Defines)
    end

    -- Helper function for adding dynamic module dependencies
    function NewProject:AddDynamicModuleDependencies(InDynamicModules)
        TableAppendMultiple(InDynamicModules, self.DynamicModuleDependencies)
    end

    -- Helper function for adding a Libraries
    function NewProject:AddModuleDependencies(InModuleDependencies)
        TableAppendMultiple(InModuleDependencies, self.ModuleDependencies)
    end

    -- Helper function for adding a Library
    function NewProject:AddStaticModuleDependencies(InStaticDependencies)
        TableAppendMultiple(InStaticDependencies, self.StaticModuleDependencies)
    end

    -- Helper function for adding libraries
    function NewProject:AddLinkLibraries(InLibraries)
        TableAppendMultiple(InLibraries, self.LinkLibraries)
    end

    -- Helper function for adding forceincludes
    function NewProject:AddForceIncludes(InForceIncludes)
        TableAppendMultiple(InForceIncludes, self.ForceIncludes)
    end

    -- Helper function for adding system include directories
    function NewProject:AddSystemIncludeDirs(InSysIncludeDirs)
        TableAppendMultiple(InSysIncludeDirs, self.SysIncludes)
    end
    
    -- Check if the project has a module project
    function NewProject:HasApplicationModule()
        return self.Module ~= nil
    end

    -- Generate project
    function NewProject:Generate()
        printf('Generate Project %s', self.Name)
                
        -- Add framework extension
        AddFrameWorkExtension(self.FrameWorks)

        -- A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
        local AllDependencies = {}
        local LinkModules     = {}

        -- Dynamic modules
        for Index = 1, #self.DynamicModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.DynamicModuleDependencies[Index];

            if bIsMonolithic then
                LinkModules[#LinkModules + 1] = self.DynamicModuleDependencies[Index];
            end
        end

        -- Modules
        for Index = 1, #self.ModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.ModuleDependencies[Index];
            LinkModules[#LinkModules + 1]       = self.ModuleDependencies[Index];
        end

        -- Static Modules
        for Index = 1, #self.StaticModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.StaticModuleDependencies[Index];
            LinkModules[#LinkModules + 1]       = self.StaticModuleDependencies[Index];
        end

        -- Debug print      
        printf('    Num of FrameWorks=%d', #self.FrameWorks)
        PrintTableWithEndLine('    Using framework dependency %s', self.FrameWorks)
        
        printf('    Num of AllDependencies=%d', #AllDependencies)
        PrintTableWithEndLine('    Using module dependency %s', AllDependencies)
        
        printf('    Num of LinkModules=%d', #LinkModules)
        PrintTableWithEndLine('    Linking module %s', LinkModules)

        printf('    Num of LinkLibraries=%d', #self.LinkLibraries)
        PrintTableWithEndLine('    Linking External Library %s', self.LinkLibraries)
        
        printf('    Num of Files=%d', #self.Files)
        PrintTableWithEndLine('    Including File  %s', self.Files)

        -- Generate the project module
        if self.bEnableApplicationModule then
            printf('    bEnableApplicationModule=true\n')

            local ProjectModule = CreateModule( NewProjectName )
            ProjectModule.Name = self.Name

            ProjectModule.Location = self.Location

            ProjectModule.bIsDynamic                = self.bIsDynamic
            ProjectModule.bUsePrecompiledHeaders    = self.bUsePrecompiledHeaders
            ProjectModule.bCompileCppAsObjectiveCpp = self.bCompileCppAsObjectiveCpp
            ProjectModule.bEnableEditAndContinue    = self.bEnableEditAndContinue
            ProjectModule.bEnableIntrinsics         = self.bEnableIntrinsics

            ProjectModule.OutputPath = self.OutputPath

            ProjectModule.Architecture = self.Architecture

            ProjectModule.Warnings = self.Warnings

            ProjectModule.Exceptionhandling = self.Exceptionhandling

            ProjectModule.bEnableRunTimeTypeInfo = self.bEnableRunTimeTypeInfo

            ProjectModule.Floatingpoint = self.Floatingpoint

            ProjectModule.VectorExtensions = self.VectorExtensions
            
            ProjectModule.Language   = self.Language
            ProjectModule.CppVersion = self.CppVersion

            ProjectModule.SystemVersion = self.SystemVersion

            ProjectModule.Characterset = self.Characterset

            ProjectModule.Flags = self.Flags

            ProjectModule.SysIncludes = self.SysIncludes

            ProjectModule.ForceInclude = self.ForceInclude

            ProjectModule.Files = self.Files

            ProjectModule.Defines = self.Defines

            ProjectModule.ExcludeFiles = self.ExcludeFiles

            ProjectModule.FrameWorks = self.FrameWorks

            ProjectModule.DynamicModuleDependencies = self.DynamicModuleDependencies
            ProjectModule.ModuleDependencies        = self.ModuleDependencies
            ProjectModule.StaticModuleDependencies  = self.StaticModuleDependencies
            
            ProjectModule.LinkLibraries = self.LinkLibraries

            NewProject.Module = ProjectModule
        else
            printf('    bEnableApplicationModule=false\n')
        end

        -- Generate workspace
        local WorkspaceName = 'DXR Engine ' .. self.Name
        workspace( WorkspaceName )
            printf('Generating Workspace %s', WorkspaceName)

            -- Set location of the generated solution file
            printf('    Generated Solution-file location %s\n', self.Location)
            location(self.Location)

            -- Platforms
            platforms
            {
                'x64',
            }

            -- Configurations
            configurations
            {
                'Debug',
                'Release',
                'Production',
            }

            -- Define the workspace location
            local WorkspaceLocation = 'WORKSPACE_LOCATION=' .. '\"' .. FindWorkspaceDir() .. '\"'
            printf('    WORKSPACE_LOCATION=%s', WorkspaceLocation)
            defines
            {
                WorkspaceLocation
            }

            -- Includes
            local RuntimeFolderPath = GetRuntimeFolderPath()
            printf('    RuntimeFolderPath=%s', RuntimeFolderPath)
            includedirs
            {
                RuntimeFolderPath,
            }

            filter 'options:monolithic'
                defines
                {
                    'MONOLITHIC_BUILD=(1)'
                }
            filter {}
        
            filter 'configurations:Debug'
                symbols 'on'
                runtime 'Debug'
                defines
                {
                    '_DEBUG',
                    'DEBUG_BUILD=(1)',
                }
            filter {}
        
            filter 'configurations:Release'
                symbols  'on'
                runtime  'Release'
                optimize 'Full'
                defines
                {
                    'NDEBUG',
                    'RELEASE_BUILD=(1)',
                }
            filter {}
        
            filter 'configurations:Production'
                symbols  'off'
                runtime  'Release'
                optimize 'Full'
                defines
                {
                    'NDEBUG',
                    'PRODUCTION_BUILD=(1)',
                }
            filter {}
            
            -- Architecture defines
            filter 'architecture:x86'
                defines
                {
                    'ARCHITECTURE_X86=(1)',
                }
            filter {}

            filter 'architecture:x86_x64'
                defines
                {
                    'ARCHITECTURE_X86_X64=(1)',
                }
            filter {}

            filter 'architecture:ARM'
                defines
                {
                    'ARCHITECTURE_ARM=(1)',
                }
            filter {}

            -- IDE options
            filter 'action:vs*'
                defines
                {
                    'IDE_VISUAL_STUDIO',
                    '_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING',
                    '_CRT_SECURE_NO_WARNINGS',
                }
            filter {}

            -- OS
            filter 'system:windows'
                defines
                {
                    'PLATFORM_WINDOWS=(1)',
                }
            filter {}

            filter 'system:macosx'
                defines
                {
                    'PLATFORM_MACOS=(1)',
                }
            filter {}

            -- Dependencies -- TODO: Better way of handling these dependencies
            local ExternalDependecyPath = GetExternalDependenciesFolderPath()
            local SolutionsFolderPath   = GetSolutionsFolderPath()
            group 'Dependencies'
                printf('\n    --- External Dependencies ---')
                
                -- Imgui
                project 'ImGui'
                    printf('    Generating Dependecy ImGui')

                    kind 	'StaticLib'
                    location(SolutionsFolderPath .. '/Dependencies/ImGui')

                    -- Locations
                    targetdir(ExternalDependecyPath .. '/Build/bin/ImGui/' .. GOutputPath)
                    objdir(ExternalDependecyPath .. '/Build/bin-int/ImGui/' .. GOutputPath)

                    -- Files
                    files
                    {
                        (ExternalDependecyPath .. '/imgui/imconfig.h'),
                        (ExternalDependecyPath .. '/imgui/imgui.h'),
                        (ExternalDependecyPath .. '/imgui/imgui.cpp'),
                        (ExternalDependecyPath .. '/imgui/imgui_draw.cpp'),
                        (ExternalDependecyPath .. '/imgui/imgui_demo.cpp'),
                        (ExternalDependecyPath .. '/imgui/imgui_internal.h'),
                        (ExternalDependecyPath .. '/imgui/imgui_tables.cpp'),
                        (ExternalDependecyPath .. '/imgui/imgui_widgets.cpp'),
                        (ExternalDependecyPath .. '/imgui/imstb_rectpack.h'),
                        (ExternalDependecyPath .. '/imgui/imstb_textedit.h'),
                        (ExternalDependecyPath .. '/imgui/imstb_truetype.h'),
                    }
                    
                    -- Configurations
                    filter 'configurations:Debug or Release'
                        symbols  'on'
                        runtime  'Release'
                        optimize 'Full'
                    filter {}
                    
                    filter 'configurations:Production'
                        symbols  'off'
                        runtime  'Release'
                        optimize 'Full'
                    filter {}
                
                -- tinyobjloader Project
                project 'tinyobjloader'
                    printf('    Generating Dependecy tinyobjloader')

                    kind 	 'StaticLib'
                    location (SolutionsFolderPath .. '/Dependencies/tinyobjloader')

                    -- Locations
                    targetdir(ExternalDependecyPath .. '/Build/bin/tinyobjloader/' .. GOutputPath)
                    objdir(ExternalDependecyPath .. '/Build/bin-int/tinyobjloader/' .. GOutputPath)

                    -- Files
                    files 
                    {
                        (ExternalDependecyPath .. '/tinyobjloader/tiny_obj_loader.h'),
                        (ExternalDependecyPath .. '/tinyobjloader/tiny_obj_loader.cc'),
                    }

                    -- Configurations
                    filter 'configurations:Debug or Release'
                        symbols  'on'
                        runtime  'Release'
                        optimize 'Full'
                    filter {}

                    filter 'configurations:Production'
                        symbols  'off'
                        runtime  'Release'
                        optimize 'Full'	
                    filter {}
                
                -- OpenFBX Project
                project 'OpenFBX'
                    printf('    Generating Dependecy OpenFBX')

                    kind 	 'StaticLib'
                    location (SolutionsFolderPath .. '/Dependencies/OpenFBX')
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. '/Build/bin/OpenFBX/' .. GOutputPath)
                    objdir(ExternalDependecyPath .. '/Build/bin-int/OpenFBX/' .. GOutputPath)

                    -- Files
                    files 
                    {
                        (ExternalDependecyPath .. '/OpenFBX/src/ofbx.h'),
                        (ExternalDependecyPath .. '/OpenFBX/src/ofbx.cpp'),
                        (ExternalDependecyPath .. '/OpenFBX/src/miniz.h'),
                        (ExternalDependecyPath .. '/OpenFBX/src/miniz.c'),
                    }

                    -- Configurations 
                    filter 'configurations:Debug or Release'
                        symbols  'on'
                        runtime  'Release'
                        optimize 'Full'
                    filter {}
                    
                    filter 'configurations:Production'
                        symbols  'off'
                        runtime  'Release'
                        optimize 'Full'
                    filter {}
            group ""

            -- Include all modules and generate the projects
            printf('\nCreating Module Dependencies')            
            for Index = 1, #AllDependencies do
                local DependecyPath = RuntimeFolderPath .. '/' .. AllDependencies[Index] .. '/Module.lua'
                printf('--Including Dependency %s', DependecyPath)

                include(DependecyPath)
            end

            -- Executeble
            project( self.Name )               
                printf( 'Generating Project %s', self.Name ) 

                -- Kind of executeable
                if self.bIsConsoleApp then
                    kind 'ConsoleApp'
                else
                    kind 'WindowedApp'
                end

                -- Defines
                self:AddDefines
                { 
                    ('PROJECT_NAME=' .. '\"' .. self.Name .. '\"'),
                    ('PROJECT_LOCATION=' .. '\"' .. FindWorkspaceDir() .. '/' .. self.Name .. '\"')
                }

                PrintTableWithEndLine('    Using define %s', self.Defines)

                defines(self.Defines)

                -- Include EngineLoop
                local RuntimeFolderPath = GetRuntimeFolderPath()
                files
                {
                    (RuntimeFolderPath .. '/Main/EngineLoop.cpp'),
                    (RuntimeFolderPath .. '/Main/EngineMain.inl'),	
                }

                -- Include EntryPoint
                filter 'system:windows'
                    files
                    {
                        (RuntimeFolderPath .. '/Main/Windows/WindowsMain.cpp'),	
                    }
                filter {}
                
                filter 'system:macosx'
                    files
                    {
                        (RuntimeFolderPath .. '/Main/Mac/MacMain.cpp'),	
                    }
                filter {}

                -- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
                if self.bCompileCppAsObjectiveCpp then
                    filter { 'system:macosx', 'files:**.cpp' }
                        compileas 'Objective-C++'
                    filter {}
                end
                
                -- In visual studio show natvis files
                filter 'action:vs*'
                    vpaths { ['Natvis'] = '**.natvis' }
                    
                    files 
                    {
                        GBasePath .. '/%{prj.name}/**.natvis',
                    }
                filter {}
                
                -- Remove files
                filter 'system:windows'
                    removefiles
                    {
                        GBasePath .. '/**/Mac/**'
                    }
                filter {}

                filter 'system:macosx'
                    removefiles
                    {
                        GBasePath .. '/**/Windows/**'
                    }
                filter {}
                
                -- LinkOptions
                if bIsMonolithic then
                    local LinkOptions_Windows = {}
                    local LinkOptions_Mac     = {}
                    for Index = 1, #LinkModules do
                        LinkOptions_Windows[#LinkOptions_Windows + 1] = '/INCLUDE:LinkModule_' .. LinkModules[Index]
                    end

                    PrintTableWithEndLine( 'Link Options %s', LinkOptions )

                    linkoptions( LinkOptions )
                end

                -- Linking
                filter 'system:macosx'
                    links(self.FrameWorks)
                filter{}
                
                links(self.LinkLibraries)
                links(LinkModules)

            project '*'
    end

    return NewProject
end