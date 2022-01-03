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
    if Element ~= nil then
        for Index = 1, #Table do
            if Table[Index] == Element then
                return
            end
        end

        Table[#Table + 1] = Element
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
    NewModule.DynamicModuleDependencies = {}
    NewModule.ModuleDependencies        = {}
    NewModule.StaticModuleDependencies  = {}

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

        -- A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
        local Dependencies      = {}
        local ModulesToLink     = {}
        local StaticLinkOptions = {}

        -- Add framework extension
        for Index = 1, #self.FrameWorks do
            self.FrameWorks[Index] = self.FrameWorks[Index] .. '.framework'
        end

        -- Dynamic modules
        for Index = 1, #self.DynamicModuleDependencies do
            Dependencies[#Dependencies + 1] = self.DynamicModuleDependencies[Index];

            if bIsMonolithic then
                ModulesToLink[#ModulesToLink + 1]         = self.DynamicModuleDependencies[Index];
                StaticLinkOptions[#StaticLinkOptions + 1] = self.DynamicModuleDependencies[Index];
            end
        end

        -- Modules
        for Index = 1, #self.ModuleDependencies do
            Dependencies[#Dependencies + 1]   = self.ModuleDependencies[Index];
            ModulesToLink[#ModulesToLink + 1] = self.ModuleDependencies[Index];
            
            if bIsMonolithic then
                StaticLinkOptions[#StaticLinkOptions + 1] = self.ModuleDependencies[Index];
            end
        end

        -- Static Modules
        for Index = 1, #self.StaticModuleDependencies do
            ModulesToLink[#ModulesToLink + 1]         = self.StaticModuleDependencies[Index];
            Dependencies[#Dependencies + 1]           = self.StaticModuleDependencies[Index];
            StaticLinkOptions[#StaticLinkOptions + 1] = self.StaticModuleDependencies[Index];
        end

        -- Debug print
        PrintTableWithEndLine( '    Using framework %s'          , self.FrameWorks )
        PrintTableWithEndLine( '    Using dependency %s'         , Dependencies )
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
                    '%{wks.location}/%{prj.name}/**.natvis',
                }
            filter {}

            -- Remove files
            excludes(self.ExcludeFiles)

            -- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
            filter { 'system:macosx', 'files:**.cpp' }
                if self.bCompileCppAsObjectiveCpp then
                    compileas 'Objective-C++'
                end
            filter {}

            -- OS
            filter 'system:windows'
                removefiles
                {
                    '**/Mac/**',
                }
            filter {}

            filter 'system:macosx'
                removefiles
                {
                    '**/Windows/**',
                }
            filter {}

            -- Linking
            links(self.FrameWorks)
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
end