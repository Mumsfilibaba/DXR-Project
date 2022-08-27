include 'build_common.lua'

-- Build rules for a project
function FBuildRules(InName)

    -- Needs to have a valid modulename
    if InName == nil then
        printf('BuildRule failed to due invalid name')
        return nil
    end

    printf('Creating BuildRule \'%s\'\n', InName)

    -- Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- Init Public Members
    local self = 
    {
        -- Name. Must be the name of the folder as well or specify the location
        Name = InName,

        -- Location for IDE project files
        Location = GetSolutionsFolderPath(),
        -- Location for generated files from the build
        BuildFolderPath = GetEnginePath() .. '/Build',
        -- Location for the build (Inside the build folder specified inside the build-folder )
        OutputPath = '/%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}',

        -- Should use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
        bUsePrecompiledHeaders = false,
        -- Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), this makes compilation for all files native to the IOS and Mac platform
        bCompileCppAsObjectiveCpp = true,
        -- Enable runtime type information
        bEnableRunTimeTypeInfo = false,
        -- Enable Edit and Continue in Visual Studio
        bEnableEditAndContinue = false,
        -- Enable C++ intrinsics
        bEnableIntrinsics = true,

        -- Architecture to compile for
        Architecture = 'x64',
        -- Warning level to compile with
        Warnings = 'extra',
        -- How to handle C++ exceptions
        Exceptionhandling = 'Off',
        -- Floating point settings 
        Floatingpoint = 'Fast',
        -- Enable vector extensions
        VectorExtensions = 'SSE2',
        -- Language to compile
        Language   = 'C++',
        CppVersion = 'C++17',
        -- Version of system SDK
        SystemVersion = 'latest',
        -- Ascii or Unicode
        Characterset = 'Ascii',
        -- Premake Flags
        Flags = 
        { 
            'MultiProcessorCompile',
            'NoIncrementalLink',
        },

        --The kind of project to generate (SharedLib, StaticLib, WindowedApp, ConsoleApp etc.)
        Kind = 'SharedLib',
        -- System includes example: #include <vector>
        SystemIncludes = {},
        -- Forceinclude these files
        ForceIncludes = {},

        -- Paths to search library files in
        LibraryPaths = {},

        -- Files to build compile into the module
        Files =
        { 
            '**.h',
            '**.hpp',
            '**.inl',
            '**.c',
            '**.cpp',
            '**.hlsl',
            '**.hlsli'
        },
        -- Files to exclude
        ExcludeFiles =
        {
            '**.hlsl',
            '**.hlsli'
        },

        -- Defines
        Defines = {},

        -- FrameWorks, only on macOS for now, should only list the names not .framework
        FrameWorks = {},

        -- Engine Modules that this module depends on. There are 3 types, dynamic, which are loaded as DLL without automatic importing
        -- Modules using DLLs but are linked at link time (__declspec(dllimport)), and static modules
        -- Modules should be specified by the name their folder has in the Runtime folder
        ModuleDependencies = {},
        -- Extra libraries to link
        LinkLibraries = {}
    }

    -- A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
    local LinkModules = {}
    -- A list of linkoptions (Ignored on other platforms than Windows)
    local LinkOptions = {}

    -- Helper function for retrieving path
    function self.GetPath()
        return RuntimeFolderPath .. '/' ..  self.Name
    end

    -- Helper function for adding flags
    function self.AddFlags(InFlags)
        TableAppendUniqueElementMultiple(InFlags, self.Flags)
    end

    -- Helper function for adding system include directories
    function self.AddSystemIncludes(InSystemInclude)
        TableAppendUniqueElementMultiple(InSystemInclude, self.SystemIncludes)
    end

    -- Helper function for adding includes
    function self.AddIncludes(InIncludes)
        TableAppendUniqueElementMultiple(InIncludes, self.Includes)
    end

    -- Helper function for adding files
    function self.AddFiles(InFiles)
        TableAppendUniqueElementMultiple(InFiles, self.Files)
    end

    -- Helper function for adding exclude files
    function self.AddExcludeFiles(InExcludeFiles)
        TableAppendUniqueElementMultiple(InExcludeFiles, self.ExcludeFiles)
    end

    -- Helper function for adding defines
    function self.AddDefines(InDefines)
        TableAppendUniqueElementMultiple(InDefines, self.Defines)
    end

    -- Helper function for adding a module dependency
    function self.AddModuleDependencies(InModuleDependencies)
        TableAppendUniqueElementMultiple(InModuleDependencies, self.ModuleDependencies)
    end

    -- Helper function for adding libraries
    function self.AddLinkLibraries(InLibraries)
        TableAppendUniqueElementMultiple(InLibraries, self.LinkLibraries)
    end

    -- Helper function for adding frameworks
    function self.AddFrameWorks(InFrameWorks)
        TableAppendUniqueElementMultiple(InFrameWorks, self.FrameWorks)
    end

    -- Helper function for adding forceincludes
    function self.AddForceIncludes(InForceIncludes)
        TableAppendUniqueElementMultiple(InForceIncludes, self.ForceIncludes)
    end

    -- Helper function for adding LibraryPaths
    function self.AddLibraryPaths(InLibraryPaths)
        TableAppendUniqueElementMultiple(InLibraryPaths, self.LibraryPaths)
    end

    -- Helper for adding the .framework extention to a table
    function self.AddFrameWorkExtension()
        for Index = 1, #self.FrameWorks do
            self.FrameWorks[Index] = self.FrameWorks[Index] .. '.framework'
        end
    end

    -- Makes all files relative to runtime folder
    function self.MakeFileNamesRelativeToPath(FileArray)
        for Index = 1, #FileArray do
            if not path.isabsolute(FileArray[Index]) then
                FileArray[Index] = self.GetPath() .. '/' .. FileArray[Index]
            end
        end
    end

    -- Project generation
    function self.GenerateProject()
        project(self.Name)
            printf('    Generating Project \'%s\'', self.Name) 

            architecture(self.Architecture)
            warnings(self.Warnings)
            exceptionhandling(self.Exceptionhandling)

            -- Build type
            kind(self.Kind)

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

            printf('    Generated project location \'%s\'\n', self.Location)
            location(self.Location)

            -- All targets except the dependencies
            targetdir(self.BuildFolderPath .. '/bin/' .. self.OutputPath)
            objdir(self.BuildFolderPath .. '/bin-int/' .. self.OutputPath)

            -- Pre-Compiled Headers
            if self.bUsePrecompiledHeaders then
                filter "action:vs*"
                    pchheader 'PreCompiled.h'
                    pchsource 'PreCompiled.cpp'
                filter "action:not vs*"
                    pchheader(self.GetPath() .. '/PreCompiled.h')
                filter{}

                printf('    Project is using PreCompiled Headers\n')

                self.AddForceIncludes(
                {
                    'PreCompiled.h'
                })
            else
                printf('    Project does NOT use PreCompiled Headers\n')
            end

            -- Debug print
            printf('    Num ForceIncludes=%d', #self.ForceIncludes)
            if #self.ForceIncludes > 0 then
                PrintTableWithEndLine('    Using ForceInclude \'%s\'', self.ForceIncludes)
            else
                printf('')
            end

            -- Add ForceIncludes
            forceincludes(self.ForceIncludes)

            -- Add Module Defines
            printf('    Num Defines=%d', #self.Defines)
            if #self.Defines > 0 then
                PrintTableWithEndLine('    Using define \'%s\'', self.Defines)
            else
                printf('')
            end
            
            defines(self.Defines)

            -- Add System Includes
            printf('    Num SystemIncludes=%d', #self.SystemIncludes)
            if #self.SystemIncludes > 0 then
                PrintTableWithEndLine('    Using SystemInclude \'%s\'', self.SystemIncludes)
            else
                printf('')
            end

            sysincludedirs(self.SystemIncludes)

            -- Add Library Paths
            printf('    Num LibraryPaths=%d', #self.LibraryPaths)
            if #self.LibraryPaths > 0 then
                PrintTableWithEndLine('    Using LibraryPaths \'%s\'', self.LibraryPaths)
            else
                printf('')
            end

            libdirs(self.LibraryPaths)

            -- Add Module Files
            files(self.Files)

            -- In visual studio show natvis files
            filter 'action:vs*'
                vpaths { ['Natvis'] = '**.natvis' }

                files 
                {
                    (self.GetPath() .. '/**.natvis')
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

            -- TODO Enable removefiles to user
            -- TODO: Check for OS
            -- Remove OS-files
            local FilesToRemove = {}
            if BuildWithXcode() then
                FilesToRemove = 
                {
                    (RuntimeFolderPath .. '/**/Windows/**')
                }
            else
                FilesToRemove = 
                {
                    (RuntimeFolderPath .. '/**/Mac/**')
                }
            end

            removefiles(FilesToRemove)

            -- Debug print
            printf('    Num FilesToRemove=%d', #FilesToRemove)
            if #FilesToRemove > 0 then
                PrintTableWithEndLine('    Removing file  \'%s\'', FilesToRemove)
            else
                printf('')
            end

            -- Linking
            filter "system:macosx"
                links(self.FrameWorks)
            filter{}

            links(self.LinkLibraries)
            links(LinkModules)

            linkoptions(LinkOptions)

            -- Dependencies
            dependson(self.ModuleDependencies)
        project '*'
    end

    -- Base generate (generates project files)
    function self.Generate()

        -- Ensure dependencies are included
        for Index = 1, #self.ModuleDependencies do
            printf('\n----Including dependency for project \'%s\'---', self.Name)            
            
            if IsModule(self.ModuleDependencies[Index]) then
                printf('-Dependency \'%s\' is already included', self.ModuleDependencies[Index])
            else
                local DependecyPath = RuntimeFolderPath .. '/' .. self.ModuleDependencies[Index] .. '/Module.lua'
                printf('-Including Dependency \'%s\' Path=\'%s\'\n', self.ModuleDependencies[Index], DependecyPath)
                include(DependecyPath)
            end

            printf('----Dependency for project \'%s\'---\n', self.Name)  
        end

        -- Ensure that the runtime folder is added to the include folders
        self.AddSystemIncludes(
        {
            RuntimeFolderPath
        })

        -- Add framework extension
        self.AddFrameWorkExtension()

        -- Solve dependencies
        for Index = 1, #self.ModuleDependencies do
            local TempModule = GetModule(self.ModuleDependencies[Index])
            if TempModule then
                if TempModule.bRuntimeLinking then
                    LinkModules[#LinkModules + 1] = self.ModuleDependencies[Index];
                end

                if TempModule.bIsDynamic then
                    -- Empty for now
                else
                    -- If we are not building a DLL linking libraries need to be pushed up
                    self.AddLinkLibraries(TempModule.LinkLibraries)
                    self.AddModuleDependencies(TempModule.ModuleDependencies)
                end

                -- System includes can be included in a dependency header and therefore necessary in this module aswell
                self.AddSystemIncludes(TempModule.SystemIncludes)
            else
                printf('BuildRule Error: Module \'%s\' has not been included', self.ModuleDependencies[Index])
            end
        end

        -- Add link options
        if not BuildWithXcode() then
            for Index = 1, #LinkModules do
                LinkOptions[#LinkOptions + 1] = '/INCLUDE:LinkModule_' .. LinkModules[Index]
            end
        end

        -- Make files relative before printing
        self.MakeFileNamesRelativeToPath(self.Files)
        self.MakeFileNamesRelativeToPath(self.ExcludeFiles)

        -- Debug print
        printf('    Num FrameWorks=%d', #self.FrameWorks)
        if #self.FrameWorks > 0 then
            PrintTableWithEndLine('    Using framework dependency \'%s\'', self.FrameWorks)
        else
            printf('')
        end

        printf('    Num ModuleDependencies=%d', #self.ModuleDependencies)
        if #self.ModuleDependencies > 0 then
            PrintTableWithEndLine('    Using module dependency \'%s\'', self.ModuleDependencies)
        else
            printf('')
        end

        printf('    Num LinkLibraries=%d', #self.LinkLibraries)
        if #self.LinkLibraries > 0 then
            PrintTableWithEndLine('    Linking library \'%s\'', self.LinkLibraries)
        else
            printf('')
        end

        printf('    Num Files=%d', #self.Files)
        if #self.Files > 0 then
            PrintTableWithEndLine('    Including file  \'%s\'', self.Files)
        else
            printf('')
        end

        printf('    Num ExcludeFiles=%d', #self.ExcludeFiles)
        if #self.ExcludeFiles > 0 then
            PrintTableWithEndLine('    Excluding file  \'%s\'', self.ExcludeFiles)
        else
            printf('')
        end

        printf('    Num LinkModules=%d', #LinkModules)
        if #LinkModules > 0 then
            PrintTableWithEndLine('    Linking module \'%s\'', LinkModules)
        else
            printf('')
        end

        printf('    Num LinkOptions=%d', #LinkOptions)
        if #LinkOptions > 0 then
            PrintTableWithEndLine('    Link options \'%s\'', LinkOptions)
        else
            printf('')
        end

        -- Project
        self.GenerateProject()
    end
    
    return self
end