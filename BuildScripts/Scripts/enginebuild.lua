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

-- Retrieve the workspace directory
function FindWorkspaceDir()
	return os.getcwd()
end

-- Helper for printing all strings in a table and ending with endline
local function PrintTableWithEndLine( Format, Table )
    if #Table >= 1 then
        for Index = 1, #Table do
            printf( Format, Table[Index])
        end

        -- Empty line
        printf( '' )
    end
end

-- Helper appending an element to a table
local function TableAppend( Element, Table )
    if Table == nil then
        return
    end

    if Element ~= nil then
        for Index = 1, #Table do
            if Table[Index] == Element then
                return
            end
        end

        Table[#Table + 1] = Element
    end
end

local function AddFrameWorkExtension( Table )
    for Index = 1, #Table do
        Table[Index] = Table[Index] .. '.framework'
    end
end  

-- Global variable that stores all created modules
GModules = {}

-- Create a new Engine Module
function CreateModule( NewModuleName )

    -- Needs to have a valid modulename
    if NewModuleName == nil then
        return nil
    end

    -- New module
    local NewModule = {}
    
    -- Ensure that module does not already exist
    if GModules then
        if GModules[NewModuleName] then
            printf( 'Module is already created' )
            return nil
        end

        GModules[NewModuleName] = NewModule;
        printf( 'NumModules created %d\n', #GModules)
    else
        printf( 'Could not find GModules\n' )
    end

    printf( 'Creating Module %s', NewModuleName )

    -- Name. Must be the name of the folder as well or specify the location
    NewModule.Name     = NewModuleName;
    NewModule.Location = ''

    -- Should the module be dynamic or static, this is overridden by monolithic build, which forces all modules to be linked statically
    NewModule.bIsDynamic = true

    -- Should the module use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
    NewModule.bUsePrecompiledHeaders = false

    -- Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), this makes compilation for all files native to the IOS and Mac platform
    NewModule.bCompileCppAsObjectiveCpp = true

    -- Location for the build
    NewModule.OutputPath = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"

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
    NewModule.Files =
    { 
        "%{wks.location}/Runtime/%{prj.name}/**.h",
        "%{wks.location}/Runtime/%{prj.name}/**.hpp",
        "%{wks.location}/Runtime/%{prj.name}/**.inl",
        "%{wks.location}/Runtime/%{prj.name}/**.c",
        "%{wks.location}/Runtime/%{prj.name}/**.cpp",
        "%{wks.location}/Runtime/%{prj.name}/**.hlsl",
        "%{wks.location}/Runtime/%{prj.name}/**.hlsli",	
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
    NewProject.DynamicModuleDependencies = {}
    NewProject.ModuleDependencies        = {}
    NewProject.StaticModuleDependencies  = {}

    -- Extra libraries to link
    NewModule.LinkLibraries = {}

    -- Helper function for adding a define
    function NewModule:AddDefine( Define )
        TableAppend( Define, self.Defines )
    end

    -- Helper function for adding a forceinclude
    function NewModule:AddForceInclude( Include )
        TableAppend( Include, self.ForceIncludes )
    end

    -- Helper function for adding a system include directory
    function NewModule:AddSysInclude( IncludeDir )
        TableAppend( IncludeDir, self.SysIncludes )
    end

    -- Function that create premake project
    function NewModule:Generate()
        printf('Generating Module %s', self.Name)

        self:AddSysInclude( "%{wks.location}/Runtime" )

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
                self:AddDefine( ModuleImplName );
            end
        end

        -- Add framework extension
        AddFrameWorkExtension( self.FrameWorks )

        -- A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
        local AllDependencies   = {}
        local ModulesToLink     = {}
        local StaticLinkOptions = {}

        -- Dynamic modules
        for Index = 1, #self.DynamicModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.DynamicModuleDependencies[Index];

            if bIsMonolithic then
                ModulesToLink[#ModulesToLink + 1]         = self.DynamicModuleDependencies[Index];
                StaticLinkOptions[#StaticLinkOptions + 1] = self.DynamicModuleDependencies[Index];
            end
        end

        -- Modules
        for Index = 1, #self.ModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.ModuleDependencies[Index];
            ModulesToLink[#ModulesToLink + 1]     = self.ModuleDependencies[Index];
            
            if bIsMonolithic then
                StaticLinkOptions[#StaticLinkOptions + 1] = self.ModuleDependencies[Index];
            end
        end

        -- Static Modules
        for Index = 1, #self.StaticModuleDependencies do
            ModulesToLink[#ModulesToLink + 1]         = self.StaticModuleDependencies[Index];
            AllDependencies[#AllDependencies + 1]     = self.StaticModuleDependencies[Index];
            StaticLinkOptions[#StaticLinkOptions + 1] = self.StaticModuleDependencies[Index];
        end

        -- Debug print
        PrintTableWithEndLine( '    Using framework %s'          , self.FrameWorks )
        PrintTableWithEndLine( '    Using dependency %s'         , AllDependencies )
        PrintTableWithEndLine( '    Using static module %s'      , StaticLinkOptions )
        PrintTableWithEndLine( '    Linking module %s'           , ModulesToLink )
        PrintTableWithEndLine( '    Linking External Library %s' , self.LinkLibraries )
        PrintTableWithEndLine( '    Including File  %s'          , self.Files )

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

            local FinalLocation;
            if self.Location == '' then
                -- TODO: The wks.location should be something else so that the solution files can be in a seperate folder
                FinalLocation = '%{wks.location}/Runtime/' .. self.Name
            else
                FinalLocation = self.Location
            end

            printf('    Project Location %s\n', FinalLocation)
            location(FinalLocation)

            -- All targets except the dependencies
            targetdir('%{wks.location}/Build/bin/'     .. NewModule.OutputPath)
            objdir   ('%{wks.location}/Build/bin-int/' .. NewModule.OutputPath)

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

                printf( '    Project is using PreCompiled Headers\n' )

                self:AddForceInclude( 'PreCompiled.h' )
            else
                printf( '    Project does NOT use PreCompiled Headers\n' )
            end

            -- Add ForceIncludes
            PrintTableWithEndLine( '    Using ForceInclude %s', self.ForceIncludes )

            forceincludes(self.ForceIncludes)

            -- Add System Includes
            PrintTableWithEndLine( '    Using System Include Dir %s', self.SysIncludes )

            sysincludedirs(self.SysIncludes)

            -- Always add module name as a define
            self:AddDefine('MODULE_NAME=' .. '\"' .. self.Name .. '\"')

            -- Add Module Defines
            PrintTableWithEndLine( '    Using define %s', self.Defines )

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
function CreateProject( NewProjectName )

    -- Project name must be valid
    if NewProjectName == nil then
        return nil
    end

    -- Create project
    local NewProject = {}
    NewProject.Name     = NewProjectName
    NewProject.Location = ''

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
    NewProject.OutputPath = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"

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
    NewProject.Files =
    { 
        "%{wks.location}/Runtime/%{prj.name}/**.h",
        "%{wks.location}/Runtime/%{prj.name}/**.hpp",
        "%{wks.location}/Runtime/%{prj.name}/**.inl",
        "%{wks.location}/Runtime/%{prj.name}/**.c",
        "%{wks.location}/Runtime/%{prj.name}/**.cpp",
        "%{wks.location}/Runtime/%{prj.name}/**.hlsl",
        "%{wks.location}/Runtime/%{prj.name}/**.hlsli",	
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

    -- Helper function for adding a define
    function NewProject:AddDefine( Define )
        TableAppend( Define, self.Defines )
    end

    -- Helper function for adding a forceinclude
    function NewProject:AddForceInclude( Include )
        TableAppend( Include, self.ForceIncludes )
    end

    -- Helper function for adding a system include directory
    function NewProject:AddSysInclude( IncludeDir )
        TableAppend( IncludeDir, self.SysIncludes )
    end
    
    -- Check if the project has a module project
    function NewProject:HasApplicationModule()
        return self.Module ~= nil
    end

    -- Generate project
    function NewProject:Generate()
        
        printf( 'Creating Project %s', self.Name )
                
        -- Add framework extension
        AddFrameWorkExtension( self.FrameWorks )

        -- A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
        local AllDependencies   = {}
        local ModulesToLink     = {}
        local StaticLinkOptions = {}

        -- Dynamic modules
        for Index = 1, #self.DynamicModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.DynamicModuleDependencies[Index];

            if bIsMonolithic then
                ModulesToLink[#ModulesToLink + 1]         = self.DynamicModuleDependencies[Index];
                StaticLinkOptions[#StaticLinkOptions + 1] = self.DynamicModuleDependencies[Index];
            end
        end

        -- Modules
        for Index = 1, #self.ModuleDependencies do
            AllDependencies[#AllDependencies + 1] = self.ModuleDependencies[Index];
            ModulesToLink[#ModulesToLink + 1]     = self.ModuleDependencies[Index];
            
            if bIsMonolithic then
                StaticLinkOptions[#StaticLinkOptions + 1] = self.ModuleDependencies[Index];
            end
        end

        -- Static Modules
        for Index = 1, #self.StaticModuleDependencies do
            AllDependencies[#AllDependencies + 1]     = self.StaticModuleDependencies[Index];
            ModulesToLink[#ModulesToLink + 1]         = self.StaticModuleDependencies[Index];
            StaticLinkOptions[#StaticLinkOptions + 1] = self.StaticModuleDependencies[Index];
        end

        -- Debug print
        PrintTableWithEndLine( '    Using framework %s'          , self.FrameWorks )
        PrintTableWithEndLine( '    Using dependency %s'         , AllDependencies )
        PrintTableWithEndLine( '    Using static module %s'      , StaticLinkOptions )
        PrintTableWithEndLine( '    Linking module %s'           , ModulesToLink )
        PrintTableWithEndLine( '    Linking External Library %s' , self.LinkLibraries )
        PrintTableWithEndLine( '    Including File  %s'          , self.Files )

        -- Generate the project module
        if self.bEnableApplicationModule then
            local ProjectModule = CreateModule( NewProjectName )
            ProjectModule.Name = self.Name

            ProjectModule.Location = self.Location

            ProjectModule.bIsDynamic                = self.bIsDynamic
            ProjectModule.bUsePrecompiledHeaders    = self.bUsePrecompiledHeaders
            ProjectModule.bCompileCppAsObjectiveCpp = self.bCompileCppAsObjectiveCpp

            ProjectModule.OutputPath = self.OutputPath

            ProjectModule.Architecture = self.Architecture

            ProjectModule.Warnings = self.Warnings

            ProjectModule.Exceptionhandling = self.Exceptionhandling

            ProjectModule.bEnableRunTimeTypeInfo = self.bEnableRunTimeTypeInfo

            ProjectModule.Floatingpoint = self.Floatingpoint

            ProjectModule.VectorExtensions = self.VectorExtensions

            ProjectModule.bEnableEditAndContinue = self.bEnableEditAndContinue
            ProjectModule.bEnableIntrinsics      = self.bEnableIntrinsics
            
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
        end

        -- Generate workspace
        local WorkspaceName = 'DXR Engine ' .. self.Name
        workspace( WorkspaceName )

            printf( 'Generating Workspace %s', WorkspaceName )

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

            defines
            {
                'WORKSPACE_LOCATION=' .. '\"' .. FindWorkspaceDir().. '\"',
            }

            -- Includes
            includedirs
            {
                '%{wks.location}/Runtime',
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

            -- Include all modules and generate the projects
            for Index = 1, #AllDependencies do
                local DependecyPath = 'Runtime/' .. AllDependencies[Index] .. 'Module.lua' 
                include( DependecyPath )
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

                -- Definea
                self:AddDefine( 'PROJECT_NAME=' .. '\"' .. self.Name .. '\"' )
                self:AddDefine( "PROJECT_LOCATION=" .. "\"" .. findWorkspaceDir() .. "/" .. projectname .. "\"" )

                PrintTableWithEndLine( '    Using define %s', self.Defines )

                defines( self.Defines )

                -- Include EngineLoop
                files
                {
                    "%{wks.location}/Runtime/Main/EngineLoop.cpp",
                    "%{wks.location}/Runtime/Main/EngineMain.inl",	
                }

                -- Include EntryPoint
                filter "system:windows"
                    files
                    {
                        "%{wks.location}/Runtime/Main/Windows/WindowsMain.cpp",	
                    }
                filter {}
                
                filter "system:macosx"
                    files
                    {
                        "%{wks.location}/Runtime/Main/Mac/MacMain.cpp",	
                    }
                filter {}

                -- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
                if self.bCompileCppAsObjectiveCpp then
                    filter { "system:macosx", "files:**.cpp" }
                        compileas "Objective-C++"
                    filter {}
                end
                
                -- In visual studio show natvis files
                filter "action:vs*"
                    vpaths { ["Natvis"] = "**.natvis" }
                    
                    files 
                    {
                        "%{wks.location}/%{prj.name}/**.natvis",
                    }
                filter {}
                
                -- Remove files
                filter "system:windows"
                    removefiles
                    {
                        "%{wks.location}/**/Mac/**"
                    }
                filter {}

                filter "system:macosx"
                    removefiles
                    {
                        "%{wks.location}/**/Windows/**"
                    }
                filter {}
                
                -- Linking
                filter "system:macosx"
                    links(self.FrameWorks)
                filter{}
                
                links(self.LinkLibraries)

            project '*'
    end

    return NewProject
end