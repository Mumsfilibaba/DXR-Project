include "build_common.lua"

-- Build rules for a project
function FBuildRules(InName)

    -- Needs to have a valid modulename
    if InName == nil then
        LogError("BuildRule failed due to invalid name")
        return nil
    end

    LogHighlight("Creating BuildRule \'%s\'", InName)

    -- Folder path for engine-modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- Init Public Members
    local self = 
    {
        -- @brief - Name. Must be the name of the folder as well or specify the location
        Name = InName,

        -- @brief - Location for IDE project files
        ProjectFilePath = "",
        
        -- @brief - Location for generated files from the build
        BuildFolderPath = "",
        
        -- @brief - Location for the build (Inside the build folder specified inside the build-folder )
        OutputPath = "",

        -- @brief - The workspace that this rule is currently a part of
        Workspace = { },

        -- @brief - Should use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
        bUsePrecompiledHeaders = false,
        
        -- @brief - Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), this makes compilation for all files native to the IOS and Mac platform
        bCompileCppAsObjectiveCpp = true,
        
        -- @brief - Enable runtime type information
        bEnableRunTimeTypeInfo = false,
        
        -- @brief - Enable Edit and Continue in Visual Studio
        bEnableEditAndContinue = false,
        
        -- @brief - Enable C++ intrinsics
        bEnableIntrinsics = true,

        -- @brief - Architecture to compile for
        Architecture = "x86_64",
        
        -- @brief - Warning level to compile with
        Warnings = "extra",
        
        -- @brief - How to handle C++ exceptions
        Exceptionhandling = "Off",
        
        -- @brief - Floating point settings 
        Floatingpoint = "Fast",
        
        -- @brief - Enable vector extensions
        VectorExtensions = "SSE2",
        
        -- @brief - Language to compile
        Language   = "C++",

        -- @brief - Language Version to compile
        CppVersion = "C++20",
        
        -- @brief - Version of system SDK
        SystemVersion = "latest",
        
        -- @brief - Ascii or Unicode
        Characterset = "Ascii",
        
        -- @brief - Premake Flags
        Flags = 
        { 
            "MultiProcessorCompile",
            "NoIncrementalLink",
        },

        -- @brief - The kind of project to generate (SharedLib, StaticLib, WindowedApp, ConsoleApp etc.)
        Kind = "SharedLib",
        
        -- @brief - System includes example: #include <vector>
        SystemIncludes = { },
        
        -- @brief - Forceinclude these files
        ForceIncludes = { },

        -- @brief - Paths to search library files in
        LibraryPaths = { },

        -- @brief - Files to compile into the module
        Files =
        { 
            "**.h",
            "**.hpp",
            "**.inl",
            "**.c",
            "**.cpp",
            "**.hlsl",
            "**.hlsli"
        },

        -- @brief - Files to exclude
        ExcludeFiles =
        {
            "**.hlsl",
            "**.hlsli"
        },

        -- @brief - Defines
        Defines = { },

        -- @brief - FrameWorks, only on macOS for now, should only list the names not .framework
        FrameWorks = { },

        -- @brief - Should the libraries be embedded into the executable (This only applies to macOS at the moment)
        bEmbedDependencies = false,

        -- @brief - Extra names to embed (This only applies to macOS at the moment)
        ExtraEmbedNames = { },

        -- @brief - Engine Modules that this module depends on. There are 3 types, dynamic, which are loaded as DLL without automatic importing Modules using DLLs 
        -- but are linked at link time (__declspec(dllimport)), and static modules. Modules should be specified by the name their folder has in the Runtime folder
        ModuleDependencies = { },
        
        -- @brief - Extra libraries to link
        LinkLibraries = { }
    }

    -- @brief - A list of dependencies that a module depends on. Ensures that the IDE builds all the projects
    local LinkModules = { }
    
    -- @brief - A list of linkoptions (Ignored on other platforms than Windows)
    local LinkOptions = { }

    -- @brief - Helper function for retrieving path
    function self.GetPath()
        return RuntimeFolderPath .. "/" ..  self.Name
    end

    -- @brief - Helper function for adding flags
    function self.AddFlags(InFlags)
        AddUniqueElements(InFlags, self.Flags)
    end

    -- @brief - Helper function for adding system include directories
    function self.AddSystemIncludes(InSystemIncludes)
        AddUniqueElements(InSystemIncludes, self.SystemIncludes)
    end

    -- @brief - Helper function for adding includes
    function self.AddIncludes(InIncludes)
        AddUniqueElements(InIncludes, self.Includes)
    end

    -- @brief - Helper function for adding files
    function self.AddFiles(InFiles)
        AddUniqueElements(InFiles, self.Files)
    end

    -- @brief - Helper function for adding exclude files
    function self.AddExcludeFiles(InExcludeFiles)
        AddUniqueElements(InExcludeFiles, self.ExcludeFiles)
    end

    -- @brief - Helper function for adding defines
    function self.AddDefines(InDefines)
        AddUniqueElements(InDefines, self.Defines)
    end

    -- @brief - Helper function for adding a module dependency
    function self.AddModuleDependencies(InModuleDependencies)
        AddUniqueElements(InModuleDependencies, self.ModuleDependencies)
    end

    -- @brief - Helper function for adding a extra embed names (This only applies to macOS at the moment)
    function self.AddExtraEmbedNames(InExtraEmbedNames)
        AddUniqueElements(InExtraEmbedNames, self.ExtraEmbedNames)
    end

    -- @brief - Helper function for adding libraries
    function self.AddLinkLibraries(InLibraries)
        AddUniqueElements(InLibraries, self.LinkLibraries)
    end

    -- @brief - Helper function for adding frameworks
    function self.AddFrameWorks(InFrameWorks)
        AddUniqueElements(InFrameWorks, self.FrameWorks)
    end

    -- @brief - Helper function for adding forceincludes
    function self.AddForceIncludes(InForceIncludes)
        AddUniqueElements(InForceIncludes, self.ForceIncludes)
    end

    -- @brief - Helper function for adding LibraryPaths
    function self.AddLibraryPaths(InLibraryPaths)
        AddUniqueElements(InLibraryPaths, self.LibraryPaths)
    end

    -- @brief - Helper for adding the .framework extention to a table
    function self.AddFrameWorkExtension()
        for Index = 1, #self.FrameWorks do
            self.FrameWorks[Index] = self.FrameWorks[Index] .. ".framework"
        end
    end

    -- @brief - Makes all files relative to runtime folder
    function self.MakeFileNamesRelativeToPath(FileArray)
        for Index = 1, #FileArray do
            if not path.isabsolute(FileArray[Index]) then
                FileArray[Index] = self.GetPath() .. "/" .. FileArray[Index]
            end
        end
    end

    -- @brief - Project generation
    function self.GenerateProject()
        project(self.Name)
            LogHighlight("\n--- Generating project files for Project \'%s\' ---", self.Name) 

            architecture(self.Architecture)
            warnings(self.Warnings)
            exceptionhandling(self.Exceptionhandling)

            -- Build type
            kind(self.Kind)

            -- Setup RunTimeTypeInfo
            if self.bEnableRunTimeTypeInfo then
                rtti("On")
            else
                rtti("Off")
            end

            floatingpoint(self.Floatingpoint)
            vectorextensions(self.VectorExtensions)

            -- Setup EditAndContinue
            if self.bEnableEditAndContinue then
                editandcontinue("On")
            else
                editandcontinue("Off")
            end

            -- Setup Intrinsics
            if self.bEnableIntrinsics then
                intrinsics("On")
            else
                intrinsics("Off")
            end

            -- Setup Language
            local CurrentLanguage = self.Language:upper()
            if CurrentLanguage ~= "C++" then
                LogError("Invalid language \'%s\'", self.Language) 
                return nil
            end

            language(self.Language)

            -- Setup Version
            local CurrentLanguageVersion = self.CppVersion:lower()
            if VerifyLanguageVersion(CurrentLanguageVersion) == false then
                LogError("Invalid language version \'%s\'", self.Language) 
                return nil
            end

            cppdialect(self.CppVersion)

            -- Setup System
            systemversion(self.SystemVersion)

            -- Setup CharacterSet
            local CurrentCharacterset = self.Characterset:lower()
            if (CurrentCharacterset == "ascii" or CurrentCharacterset == "unicode") == false then
                LogError("Invalid Characterset \'%s\'", self.Characterset) 
                return nil
            end

            characterset(self.Characterset)

            -- Setup Location
            LogInfo("    Project location \'%s\'", self.ProjectFilePath)
            location(self.ProjectFilePath)

            -- Setup all targets except the dependencies
            local FullObjectFolderPath = self.BuildFolderPath .. "/bin/" .. self.OutputPath
            LogInfo("    Target location \'%s\'", FullObjectFolderPath)
            targetdir(FullObjectFolderPath)

            local FullIntermediateFolderPath = self.BuildFolderPath .. "/bin-int/" .. self.OutputPath
            LogInfo("    Object files location \'%s\'", FullIntermediateFolderPath)
            objdir(FullIntermediateFolderPath)

            -- Setup Pre-Compiled Headers
            if self.bUsePrecompiledHeaders then
                filter "action:vs*"
                    pchheader("PreCompiled.h")
                    pchsource("PreCompiled.cpp")
                filter "action:not vs*"
                    pchheader(self.GetPath() .. "/PreCompiled.h")
                filter{}

                LogInfo("    Project is using PreCompiled Headers")
            else
                LogInfo("    Project does NOT use PreCompiled Headers")
            end

            -- Debug Logging
            LogInfo("\n--- ForceIncludes for module \'%s\' (Num ForceIncludes=%d) ---", self.Name, #self.ForceIncludes)
            if #self.ForceIncludes > 0 then
                PrintTable("    Using ForceInclude \'%s\'", self.ForceIncludes)
            end

            LogInfo("\n--- Defines for module \'%s\' (Num Defines=%d) ---", self.Name, #self.Defines)
            if #self.Defines > 0 then
                PrintTable("    Using define \'%s\'", self.Defines)
            end

            LogInfo("\n--- SystemIncludes for module \'%s\' (Num SystemIncludes=%d) ---", self.Name, #self.SystemIncludes)
            if #self.SystemIncludes > 0 then
                PrintTable("    Using SystemInclude \'%s\'", self.SystemIncludes)
            end

            LogInfo("\n--- LibraryPaths for module \'%s\' (Num LibraryPaths=%d) ---", self.Name, #self.LibraryPaths)
            if #self.LibraryPaths > 0 then
                PrintTable("    Using LibraryPath \'%s\'", self.LibraryPaths)
            end

            LogInfo("\n--- Files for module \'%s\' (Num Files=%d) ---", self.Name, #self.Files)
            if #self.Files > 0 then
                PrintTable("    Including file  \'%s\'", self.Files)
            end
            
            LogInfo("\n--- Exclude files for module \'%s\' (Num ExcludeFiles=%d) ---", self.Name, #self.ExcludeFiles)
            if #self.ExcludeFiles > 0 then
                PrintTable("    Excluding file  \'%s\'", self.ExcludeFiles)
            end
            
            LogInfo("\n--- Frameworks for module \'%s\' (Num FrameWorks=%d) ---", self.Name, #self.FrameWorks)
            if #self.FrameWorks > 0 then
                PrintTable("    Using framework dependency \'%s\'", self.FrameWorks)
            end
            
            LogInfo("\n--- LinkLibraries for module \'%s\' (Num LinkLibraries=%d) ---", self.Name, #self.LinkLibraries)
            if #self.LinkLibraries > 0 then
                PrintTable("    Linking library \'%s\'", self.LinkLibraries)
            end
            
            LogInfo("\n--- Link modules for module \'%s\' (Num LinkModules=%d) ---", self.Name, #LinkModules)
            if #LinkModules > 0 then
                PrintTable("    Linking module \'%s\'", LinkModules)
            end
            
            LogInfo("\n--- Link options for module \'%s\' (Num LinkOptions=%d) ---", self.Name, #LinkOptions)
            if #LinkOptions > 0 then
                PrintTable("    Link options \'%s\'", LinkOptions)
            end
            
            LogInfo("\n--- Module dependencies for module \'%s\' (Num ModuleDependencies=%d) ---", self.Name, #self.ModuleDependencies)
            if #self.ModuleDependencies > 0 then
                PrintTable("    Using module dependency \'%s\'", self.ModuleDependencies)
            end
            
            LogInfo("\n--- Embedded modules for module \'%s\' (Num Embedded Modules=%d) ---", self.Name, #self.ModuleDependencies)
            if #self.ModuleDependencies > 0 then
                PrintTable("    Embed Module \'%s\'", self.ModuleDependencies)
            end

            -- Setup ForceIncludes
            forceincludes(self.ForceIncludes)

            defines(self.Defines)
            
            externalincludedirs(self.SystemIncludes)
            
            libdirs(self.LibraryPaths)
            
            files(self.Files)

            -- Setup Exclude OS-files
            if IsPlatformWindows() then
                filter { "files:**/Mac/**.cpp" }
                    flags { "ExcludeFromBuild" }
                filter {}
            elseif IsPlatformMac() then
                filter { "files:**/Windows/**.cpp" }
                    flags { "ExcludeFromBuild" }
                filter {}

                -- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
                if self.bCompileCppAsObjectiveCpp then
                    filter { "files:**.cpp" }
                        compileas("Objective-C++")
                    filter {}
                end
            end

            -- In visual studio show natvis files
            filter "action:vs*"
                vpaths { ["Natvis"] = "**.natvis" }

                files 
                {
                    (self.GetPath() .. "/**.natvis")
                }
            filter {}

            -- Remove files
            removefiles(self.ExcludeFiles)

            -- Setup Linking
            if IsPlatformMac() then
                -- Ignore linking when kind is set to none
                if self.Kind == "None" then
                    LogWarning("Ignoring Frameworks due to the kind being set to \'None\'")
                else
                    links(self.FrameWorks)
                end
            end

            -- Ignore linking and dependencies when kind is set to none
            if self.Kind == "None" then
                LogWarning("Ignoring LinkLibraries due to the kind being set to \'None\'")
                LogWarning("Ignoring LinkModules due to the kind being set to \'None\'")
                LogWarning("Ignoring LinkOptions due to the kind being set to \'None\'")
                LogWarning("Ignoring Dependencies due to the kind being set to \'None\'")
            else
                -- Link libraries (External libraries etc.)
                links(self.LinkLibraries)
                links(LinkModules)
                linkoptions(LinkOptions)
                -- Setup Dependencies
                dependson(self.ModuleDependencies)
            end
            
            -- Setup embeded Frameworks etc.
            filter { "action:xcode4" }
                if self.bEmbedDependencies then
                    -- TODO: Embedding the frameworks seems to still sign them, even if it clearly states "Embed without signing", someone at Apple, probably f-ed up, or something is very, very unclear here
                    -- embed(self.FrameWorks)
                    embed(self.ModuleDependencies)
                    embed(self.ExtraEmbedNames)
                end
            filter {}

            -- TODO: If the app actually needs to get signed, this needs to be revisited
            filter { "action:xcode4" }
                xcodebuildsettings 
                {
                    ["PRODUCT_BUNDLE_IDENTIFIER"] = "dxrproject." .. self.Name,
                    ["CODE_SIGN_STYLE"]           = "Automatic",
                    ["ENABLE_HARDENED_RUNTIME"]   = "NO",                                          -- hardened runtime is required for notarization
                    ["GENERATE_INFOPLIST_FILE"]   = "YES",                                         -- generate the .plist file for now
                    -- ["CODE_SIGN_IDENTITY"]        = "Apple Development",                        -- sets 'Signing Certificate' to 'Development'. Defaults to 'Sign to Run Locally'. not doing this will crash your app if you upgrade the project when prompted by Xcode
                    ["LD_RUNPATH_SEARCH_PATHS"]   = "$(inherited) @executable_path/../Frameworks", -- tell the executable where to find the frameworks. Path is relative to executable location inside .app bundle
                }
            filter {}
        project "*"

        LogHighlight("\n--- Finished generating project files for Project \'%s\' ---", self.Name) 
    end

    -- Base generate (generates project files)
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating Rule")
            return
        end

        -- Ensure dependencies are included
        for Index = 1, #self.ModuleDependencies do
            LogHighlight("\n--- Including dependency for project \'%s\' ---", self.Name)
            
            local CurrentModuleName = self.ModuleDependencies[Index]
            if IsModule(CurrentModuleName) then
                LogHighlightWarning("-Dependency \'%s\' is already included", CurrentModuleName)
            else
                local DependecyPath = RuntimeFolderPath .. "/" .. CurrentModuleName .. "/Module.lua"
                LogInfo("-Including Dependency \'%s\' Path=\'%s\'", CurrentModuleName, DependecyPath)
                include(DependecyPath)

                -- Generate module, but check so that it exists since some platforms does not create certain modules (D3D12RHI, MetalRHI etc.)
                if IsModule(CurrentModuleName) then
                    local CurrentModule = GetModule(CurrentModuleName)
                    CurrentModule.Workspace = self.Workspace
                    CurrentModule.Generate()
                else
                    LogWarning("Failed to properly create module \'%s\'", CurrentModuleName)
                end
            end
        end

        -- Setup folder paths
        self.BuildFolderPath = self.Workspace.GetBuildFolderPath()
        self.OutputPath      = self.Workspace.GetOutputPath()
        self.ProjectFilePath = self.Workspace.GetSolutionsFolderPath()

        -- Ensure that the runtime folder is added to the include folders
        self.AddSystemIncludes({ RuntimeFolderPath })

        -- Add framework extension
        self.AddFrameWorkExtension()

        -- Solve dependencies
        for Index = 1, #self.ModuleDependencies do
            local CurrentModuleName = self.ModuleDependencies[Index]
            local CurrentModule     = GetModule(CurrentModuleName)
            if CurrentModule then
                if CurrentModule.bRuntimeLinking == false then
                    LinkModules[#LinkModules + 1] = CurrentModuleName
                end

                if CurrentModule.bIsDynamic then
                    local ModuleApiName = CurrentModule.Name:upper() .. "_API"
                    
                    -- This should be linked at compile time
                    if CurrentModule.bRuntimeLinking == false then
                        ModuleApiName = ModuleApiName .. "=MODULE_IMPORT"
                    end

                    self.AddDefines({ ModuleApiName })
                end
                
                -- TODO: This should probably bes seperated into public/private dependencies since public should always be pushed up
                -- We always want to add the frameworks and modules as a dependency 
                self.AddLinkLibraries(CurrentModule.LinkLibraries)
                self.AddFrameWorks(CurrentModule.FrameWorks)
                self.AddModuleDependencies(CurrentModule.ModuleDependencies)

                -- System includes can be included in a dependency header and therefore necessary in this module aswell
                self.AddSystemIncludes(CurrentModule.SystemIncludes)
            else
                LogError("Module \'%s\' has not been included", CurrentModuleName)
            end
        end

        -- Add link options
        if BuildWithVisualStudio() then
            for Index = 1, #LinkModules do
                LinkOptions[#LinkOptions + 1] = "/INCLUDE:LinkModule_" .. LinkModules[Index]
            end
        end

        -- Setup Pre-Compiled Headers
        if self.bUsePrecompiledHeaders then
            self.AddForceIncludes({ "PreCompiled.h" })
        end

        -- Make files relative before printing
        self.MakeFileNamesRelativeToPath(self.Files)
        self.MakeFileNamesRelativeToPath(self.ExcludeFiles)

        -- Add this rule to the workspace
        self.Workspace.AddRule(self)
    end
    
    return self
end