include "BuildTool_Common.lua"

-- Build rules for a project
function BuildRules(Name)

    -- Needs to have a valid module name
    if Name == nil then
        LogError("BuildRule failed due to invalid name")
        return nil
    end

    LogHighlight("Creating BuildRule '%s'", Name)

    -- Folder path for engine modules
    local RuntimeFolderPath = GetRuntimeFolderPath()

    -- Initialize public members
    local self =
    {
        -- @brief - Name. Must be the name of the folder as well or specify the location
        Name = Name,

        -- @brief - Location for IDE project files
        ProjectFilePath = "",

        -- @brief - Location for generated files from the build
        BuildFolderPath = "",

        -- @brief - Location for the build (inside the build folder specified inside the build-folder)
        OutputPath = "",

        -- @brief - The workspace that this rule is currently a part of
        Workspace = {},

        -- @brief - Should use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
        bUsePrecompiledHeaders = false,

        -- @brief - Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), making compilation for all files native to the iOS and Mac platform
        bCompileCppAsObjectiveCpp = true,

        -- @brief - Enable runtime type information
        bEnableRuntimeTypeInfo = false,

        -- @brief - Enable Edit and Continue in Visual Studio
        bEnableEditAndContinue = false,

        -- @brief - Enable C++ intrinsics
        bEnableIntrinsics = true,

        -- @brief - Architecture to compile for
        Architecture = "x86_64",

        -- @brief - Warning level to compile with
        Warnings = "extra",

        -- @brief - How to handle C++ exceptions
        ExceptionHandling = "Off",

        -- @brief - Floating point settings
        FloatingPoint = "Fast",

        -- @brief - Enable vector extensions
        VectorExtensions = "AVX2",

        -- @brief - Language to compile
        Language = "C++",

        -- @brief - Language version to compile
        CppVersion = "C++20",

        -- @brief - Version of system SDK
        SystemVersion = "latest",

        -- @brief - ASCII or Unicode
        CharacterSet = "Ascii",

        -- @brief - Premake flags
        Flags =
        {
            "MultiProcessorCompile",
            "NoIncrementalLink",
        },

        -- @brief - The kind of project to generate (SharedLib, StaticLib, WindowedApp, ConsoleApp, etc.)
        Kind = "SharedLib",

        -- @brief - Include directories, e.g., #include <ThirdParty.h> or #include "ThirdParty.h"
        IncludeDirs = {},

        -- @brief - External includes, e.g., #include <ThirdParty.h>
        ExternalIncludeDirs = {},

        -- @brief - Force include these files
        ForceIncludes = {},

        -- @brief - Paths to search library files in
        LibraryPaths = {},

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
        Defines = {},

        -- @brief - Frameworks, only on macOS for now; should only list the names, not .framework
        Frameworks = {},

        -- @brief - Should the libraries be embedded into the executable (this only applies to macOS at the moment)
        bEmbedThirdparties = false,

        -- @brief - Extra names to embed (this only applies to macOS at the moment)
        ExtraEmbedNames = {},

        -- @brief - Engine modules that this module depends on
        Modules = {},

        -- @brief - Extra libraries to link
        LinkLibraries = {},

        -- @brief - A list of thirdparties that a module depends on; ensures that the IDE builds all the projects
        LinkModules = {},

        -- @brief - A list of link options (ignored on platforms other than Windows)
        LinkOptions = {}
    }

    -- Helper function for retrieving path
    local BuildRulePath = JoinPath(RuntimeFolderPath, self.Name)
    function self.GetPath()
        return BuildRulePath
    end

    -- Helper functions for adding elements
    function self.AddFlags(InFlags)
        AddUniqueElements(InFlags, self.Flags)
    end

    function self.AddIncludeDirs(InIncludeDirs)
        AddUniqueElements(InIncludeDirs, self.IncludeDirs)
    end

    function self.AddExternalIncludeDirs(InExternalIncludeDirs)
        AddUniqueElements(InExternalIncludeDirs, self.ExternalIncludeDirs)
    end

    function self.AddFiles(InFiles)
        AddUniqueElements(InFiles, self.Files)
    end

    function self.AddExcludeFiles(InExcludeFiles)
        AddUniqueElements(InExcludeFiles, self.ExcludeFiles)
    end

    function self.AddDefines(InDefines)
        AddUniqueElements(InDefines, self.Defines)
    end

    function self.AddModules(InModules)
        AddUniqueElements(InModules, self.Modules)
    end

    function self.AddExtraEmbedNames(InExtraEmbedNames)
        AddUniqueElements(InExtraEmbedNames, self.ExtraEmbedNames)
    end

    function self.AddLinkLibraries(InLibraries)
        AddUniqueElements(InLibraries, self.LinkLibraries)
    end

    function self.AddFrameworks(InFrameworks)
        AddUniqueElements(InFrameworks, self.Frameworks)
    end

    function self.AddForceIncludes(InForceIncludes)
        AddUniqueElements(InForceIncludes, self.ForceIncludes)
    end

    function self.AddLibraryPaths(InLibraryPaths)
        AddUniqueElements(InLibraryPaths, self.LibraryPaths)
    end

    function self.AddLinkOptions(InOptions)
        AddUniqueElements(InOptions, self.LinkOptions)
    end

    -- Helper for adding the .framework extension to frameworks
    function self.AddFrameworkExtension()
        for Index = 1, #self.Frameworks do
            self.Frameworks[Index] = self.Frameworks[Index] .. ".framework"
        end
    end

    -- Makes all files relative to runtime folder
    function self.MakeFileNamesRelativeToPath(FileArray)
        for Index = 1, #FileArray do
            local CurrentFile = FileArray[Index]
            if not path.isabsolute(FileArray[Index]) then
                FileArray[Index] = JoinPath(self.GetPath(), FileArray[Index])
            end
        end
    end

    -- Project generation
    function self.GenerateProject()
        project(self.Name)
            LogHighlight("\n--- Generating project files for Project '%s' ---", self.Name)

            architecture(self.Architecture)
            warnings(self.Warnings)
            exceptionhandling(self.ExceptionHandling)

            -- Build type
            kind(self.Kind)

            -- Setup runtime type information
            if self.bEnableRuntimeTypeInfo then
                rtti("On")
            else
                rtti("Off")
            end

            floatingpoint(self.FloatingPoint)
            vectorextensions(self.VectorExtensions)

            -- Setup Edit and Continue
            if self.bEnableEditAndContinue then
                editandcontinue("On")
            else
                editandcontinue("Off")
            end

            -- Setup intrinsics
            if self.bEnableIntrinsics then
                intrinsics("On")
            else
                intrinsics("Off")
            end

            -- Setup language
            local CurrentLanguage = self.Language:upper()
            if CurrentLanguage ~= "C++" then
                LogError("Invalid language '%s'", self.Language)
                return nil
            end

            language(self.Language)

            -- Setup version
            local CurrentLanguageVersion = self.CppVersion:lower()
            if not VerifyLanguageVersion(CurrentLanguageVersion) then
                LogError("Invalid language version '%s'", self.CppVersion)
                return nil
            end

            cppdialect(self.CppVersion)

            -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
            filter "action:vs*"
                buildoptions { "/Zc:__cplusplus" }
            filter {}

            -- Setup system version
            systemversion(self.SystemVersion)

            -- Setup character set
            local CurrentCharacterSet = self.CharacterSet:lower()
            if CurrentCharacterSet ~= "ascii" and CurrentCharacterSet ~= "unicode" then
                LogError("Invalid character set '%s'", self.CharacterSet)
                return nil
            end

            characterset(self.CharacterSet)

            -- Setup location
            self.ProjectFilePath = CreateOsPath(self.ProjectFilePath)
            LogInfo("    Project location '%s'", self.ProjectFilePath)
            location(self.ProjectFilePath)

            -- Setup all targets except the thirdparties
            local FullObjectFolderPath = JoinPath(JoinPath(self.BuildFolderPath, "bin"), self.OutputPath)
            LogInfo("    Target location '%s'", FullObjectFolderPath)
            targetdir(FullObjectFolderPath)

            local FullIntermediateFolderPath = JoinPath(JoinPath(self.BuildFolderPath, "bin-int"), self.OutputPath)
            LogInfo("    Object files location '%s'", FullIntermediateFolderPath)
            objdir(FullIntermediateFolderPath)

            -- Setup precompiled headers
            if self.bUsePrecompiledHeaders then
                if BuildWithVisualStudio() then
                    -- Specify the full path for everything to work properly on Windows
                    local PchSourcePath = JoinPath(self.GetPath(), "PreCompiled.cpp")
                    LogHighlight("    PreCompiled source path '%s'", PchSourcePath)

                    -- Use the Unix path (this is probably an internal Premake thing)
                    local UnixPchSourcePath = path.translate(PchSourcePath, '/')
                    pchheader("PreCompiled.h")
                    pchsource(UnixPchSourcePath)
                else
                    -- Specify the full path for everything to work properly on non-Windows
                    local PchPath = JoinPath(self.GetPath(), "PreCompiled.h")
                    pchheader(PchPath)
                end

                LogInfo("    Project is using PreCompiled Headers")
            else
                LogInfo("    Project does NOT use PreCompiled Headers")
            end

            -- Debug logging
            LogInfo("\n--- ForceIncludes for module '%s' (Num ForceIncludes=%d) ---", self.Name, #self.ForceIncludes)
            if #self.ForceIncludes > 0 then
                PrintTable("    Using ForceInclude '%s'", self.ForceIncludes)
            end

            LogInfo("\n--- Defines for module '%s' (Num Defines=%d) ---", self.Name, #self.Defines)
            if #self.Defines > 0 then
                PrintTable("    Using define '%s'", self.Defines)
            end

            LogInfo("\n--- Includes for module '%s' (Num Includes=%d) ---", self.Name, #self.IncludeDirs)
            if #self.IncludeDirs > 0 then
                PrintTable("    Using Includes '%s'", self.IncludeDirs)
            end

            LogInfo("\n--- ExternalIncludes for module '%s' (Num ExternalIncludes=%d) ---", self.Name, #self.ExternalIncludeDirs)
            if #self.ExternalIncludeDirs > 0 then
                PrintTable("    Using ExternalInclude '%s'", self.ExternalIncludeDirs)
            end

            LogInfo("\n--- LibraryPaths for module '%s' (Num LibraryPaths=%d) ---", self.Name, #self.LibraryPaths)
            if #self.LibraryPaths > 0 then
                PrintTable("    Using LibraryPath '%s'", self.LibraryPaths)
            end

            LogInfo("\n--- Files for module '%s' (Num Files=%d) ---", self.Name, #self.Files)
            if #self.Files > 0 then
                PrintTable("    Including file '%s'", self.Files)
            end

            LogInfo("\n--- Exclude files for module '%s' (Num ExcludeFiles=%d) ---", self.Name, #self.ExcludeFiles)
            if #self.ExcludeFiles > 0 then
                PrintTable("    Excluding file '%s'", self.ExcludeFiles)
            end

            LogInfo("\n--- Frameworks for module '%s' (Num Frameworks=%d) ---", self.Name, #self.Frameworks)
            if #self.Frameworks > 0 then
                PrintTable("    Using framework thirdparty '%s'", self.Frameworks)
            end

            LogInfo("\n--- LinkLibraries for module '%s' (Num LinkLibraries=%d) ---", self.Name, #self.LinkLibraries)
            if #self.LinkLibraries > 0 then
                PrintTable("    Linking library '%s'", self.LinkLibraries)
            end

            LogInfo("\n--- Link modules for module '%s' (Num LinkModules=%d) ---", self.Name, #self.LinkModules)
            if #self.LinkModules > 0 then
                PrintTable("    Linking module '%s'", self.LinkModules)
            end

            LogInfo("\n--- Link options for module '%s' (Num LinkOptions=%d) ---", self.Name, #self.LinkOptions)
            if #self.LinkOptions > 0 then
                PrintTable("    Link options '%s'", self.LinkOptions)
            end

            LogInfo("\n--- Module thirdparties for module '%s' (Num ModuleThirdParties=%d) ---", self.Name, #self.Modules)
            if #self.Modules > 0 then
                PrintTable("    Using module thirdparty '%s'", self.Modules)
            end

            LogInfo("\n--- Embedded modules for module '%s' (Num Embedded Modules=%d) ---", self.Name, #self.Modules)
            if #self.Modules > 0 then
                PrintTable("    Embed Module '%s'", self.Modules)
            end

            -- Setup force includes
            forceincludes(self.ForceIncludes)

            defines(self.Defines)

            includedirs(self.IncludeDirs)
            externalincludedirs(self.ExternalIncludeDirs)

            libdirs(self.LibraryPaths)

            files(self.Files)

            -- Setup exclude OS-specific files
            if IsPlatformWindows() then
                filter { "files:**/Mac/**.cpp" }
                    flags { "ExcludeFromBuild" }
                filter {}
            elseif IsPlatformMac() then
                filter { "files:**/Windows/**.cpp" }
                    flags { "ExcludeFromBuild" }
                filter {}

                -- On macOS, compile all .cpp files as Objective-C++ to avoid pre-processor checks
                if self.bCompileCppAsObjectiveCpp then
                    filter { "files:**.cpp" }
                        compileas("Objective-C++")
                    filter {}
                end
            end

            -- In Visual Studio, show .natvis files
            if BuildWithVisualStudio() then
                vpaths { ["Natvis"] = "**.natvis" }

                local NatvisPath = JoinPath(self.GetPath(), "**.natvis")
                LogHighlight("NatvisPath='%s'", NatvisPath)

                files {
                    NatvisPath
                }
            end

            -- Remove files
            removefiles(self.ExcludeFiles)

            -- Setup linking
            if IsPlatformMac() then
                -- Ignore linking when Kind is set to 'None'
                if self.Kind == "None" then
                    LogWarning("Ignoring Frameworks due to the kind being set to 'None'")
                else
                    links(self.Frameworks)
                end
            end

            -- Ignore linking and thirdparties when Kind is set to 'None'
            if self.Kind == "None" then
                LogWarning("Ignoring LinkLibraries due to the kind being set to 'None'")
                LogWarning("Ignoring LinkModules due to the kind being set to 'None'")
                LogWarning("Ignoring LinkOptions due to the kind being set to 'None'")
                LogWarning("Ignoring ThirdParty due to the kind being set to 'None'")
            else
                -- Link libraries (external libraries, etc.)
                links(self.LinkLibraries)
                links(self.LinkModules)
                linkoptions(self.LinkOptions)

                -- Setup thirdparties
                dependson(self.Modules)
            end

            -- Setup embedded frameworks, etc.
            filter { "action:xcode4" }
                if self.bEmbedThirdparties then
                    -- Embed modules and extra embed names
                    embed(self.Modules)
                    embed(self.ExtraEmbedNames)
                end
            filter {}

            -- Xcode build settings
            filter { "action:xcode4" }
                xcodebuildsettings
                {
                    ["PRODUCT_BUNDLE_IDENTIFIER"]  = "com.DXREngine." .. self.Name,
                    ["CODE_SIGN_STYLE"]            = "Automatic",
                    ["ARCHS"]                      = "x86_64",
                    ["ONLY_ACTIVE_ARCH"]           = "YES",
                    ["ENABLE_HARDENED_RUNTIME"]    = "NO",
                    ["GENERATE_INFOPLIST_FILE"]    = "YES",
                    ["LD_RUNPATH_SEARCH_PATHS"]    = "/usr/local/lib/ $(INSTALL_PATH) @executable_path/../Frameworks",
                    ["GCC_ENABLE_AVX2_EXTENSIONS"] = "YES",
                }
            filter {}

            -- Copy dynamic libraries from thirdparties folder
            if IsPlatformWindows() then
                local DxilDllCmd = "copy " .. CreateExternalThirdpartyPath("DXC/bin/dxil.dll") .. " " .. FullObjectFolderPath
                LogHighlight("dxil.dll Cmd %s", DxilDllCmd)

                local DxcompilerDllCmd = "copy " .. CreateExternalThirdpartyPath("DXC/bin/dxcompiler.dll") .. " " .. FullObjectFolderPath
                LogHighlight("dxcompiler.dll Cmd %s", DxcompilerDllCmd)

                local AgilitySdkFolder = JoinPath(FullObjectFolderPath, "D3D12")

                -- Ensure the folder exists before copying files
                local CreateAgilityFolderCmd = "if not exist \"" .. AgilitySdkFolder .. "\" mkdir \"" .. AgilitySdkFolder .. "\""

                local D3d12coreDllCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/D3D12Core.dll") .. " " .. AgilitySdkFolder
                LogHighlight("d3d12core.dll Cmd %s", D3d12coreDllCmd)

                local D3d12corePdbCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/D3D12Core.pdb") .. " " .. AgilitySdkFolder
                LogHighlight("d3d12core.pdb Cmd %s", D3d12corePdbCmd)

                local D3d12SDKLayersDllCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3d12SDKLayers.dll") .. " " .. AgilitySdkFolder
                LogHighlight("d3d12SDKLayers.dll Cmd %s", D3d12SDKLayersDllCmd)

                local D3d12SDKLayersPdbCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3d12SDKLayers.pdb") .. " " .. AgilitySdkFolder
                LogHighlight("d3d12SDKLayers.pdb Cmd %s", D3d12SDKLayersPdbCmd)

                local D3dconfigExeCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3dconfig.exe") .. " " .. AgilitySdkFolder
                LogHighlight("d3dconfig.exe Cmd %s", D3dconfigExeCmd)

                local D3dconfigPdbCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/d3dconfig.pdb") .. " " .. AgilitySdkFolder
                LogHighlight("d3dconfig.pdb Cmd %s", D3dconfigPdbCmd)

                local DirectSrExeCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/DirectSR.dll") .. " " .. AgilitySdkFolder
                LogHighlight("DirectSR.dll Cmd %s", DirectSrExeCmd)

                local DirectSrPdbCmd = "copy " .. CreateExternalThirdpartyPath("D3D12AgilitySDK/microsoft.direct3d.d3d12.1.716.0-preview/build/native/bin/x64/DirectSR.pdb") .. " " .. AgilitySdkFolder
                LogHighlight("DirectSR.pdb Cmd %s", DirectSrPdbCmd)

                postbuildcommands
                {
                    CreateAgilityFolderCmd, -- Ensure folder exists before copying
                    DxilDllCmd,
                    DxcompilerDllCmd,
                    D3d12coreDllCmd,
                    D3d12corePdbCmd,
                    D3d12SDKLayersDllCmd,
                    D3d12SDKLayersPdbCmd,
                    D3dconfigExeCmd,
                    D3dconfigPdbCmd,
                    DirectSrExeCmd,
                    DirectSrPdbCmd,
                }
            elseif IsPlatformMac() then
                local LibDxcompilerDllCmd = "cp " .. CreateExternalThirdpartyPath("DXC/bin/libdxcompiler.dylib") .. " " .. FullObjectFolderPath
                LogHighlight("libdxcompiler.dylib Cmd %s", LibDxcompilerDllCmd)

                postbuildcommands
                {
                    LibDxcompilerDllCmd
                }
            end
        project "*"

        LogHighlight("\n--- Finished generating project files for Project '%s' ---", self.Name)
    end

    -- Base generate (generates project files)
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating Rule")
            return
        end

        -- Ensure thirdparties are included
        for Index = 1, #self.Modules do
            LogHighlight("\n--- Including thirdparty for project '%s' ---", self.Name)

            local CurrentModuleName = self.Modules[Index]
            if IsModule(CurrentModuleName) then
                LogHighlightWarning("-Dependency '%s' is already included", CurrentModuleName)
            else
                local ThirdpartyPath = JoinPath(JoinPath(RuntimeFolderPath, CurrentModuleName), "Module.lua")
                LogInfo("-Including Dependency '%s' Path='%s'", CurrentModuleName, ThirdpartyPath)
                include(ThirdpartyPath)

                -- Generate module, but check so that it exists since some platforms do not create certain modules (D3D12RHI, MetalRHI, etc.)
                if IsModule(CurrentModuleName) then
                    local CurrentModule = GetModule(CurrentModuleName)
                    CurrentModule.Workspace = self.Workspace
                    CurrentModule.Generate()
                else
                    LogWarning("Could not find '%s', perhaps it does not exist, or it may not be supported on the current setup or platform. Check the logs for more information.", CurrentModuleName)
                end
            end
        end

        -- Setup folder paths
        self.BuildFolderPath = self.Workspace.GetBuildFolderPath()
        self.OutputPath      = self.Workspace.GetOutputPath()
        self.ProjectFilePath = self.Workspace.GetSolutionsFolderPath()

        -- Ensure that the runtime folder is added to the include folders
        self.AddExternalIncludeDirs { RuntimeFolderPath }

        -- Add framework extension
        self.AddFrameworkExtension()

        -- Solve thirdparties
        for Index = 1, #self.Modules do
            local CurrentModuleName = self.Modules[Index]
            local CurrentModule     = GetModule(CurrentModuleName)

            if CurrentModule then
                if not CurrentModule.bRuntimeLinking then
                    table.insert(self.LinkModules, CurrentModuleName)
                end

                -- Add define for importing a dynamic module's exported functions and classes
                if CurrentModule.bIsDynamic then
                    local ModuleApiName = CurrentModule.Name:upper() .. "_API"

                    -- This should be linked at compile time
                    if not CurrentModule.bRuntimeLinking then
                        ModuleApiName = ModuleApiName .. "=MODULE_IMPORT"
                    end

                    self.AddDefines { ModuleApiName }
                end

                -- Propagate third-party and include/link info
                self.AddLinkLibraries(CurrentModule.LinkLibraries)
                self.AddFrameworks(CurrentModule.Frameworks)
                self.AddModules(CurrentModule.Modules)

                self.AddIncludeDirs(CurrentModule.IncludeDirs)
                self.AddExternalIncludeDirs(CurrentModule.ExternalIncludeDirs)
                self.AddLibraryPaths(CurrentModule.LibraryPaths)
            else
                LogError("Module '%s' has not been included", CurrentModuleName)
            end
        end

        -- Add link options
        if BuildWithVisualStudio() then
            -- TODO: We only want this for monolithic builds
            for Index = 1, #self.LinkModules do
                local CurrentModuleName = self.LinkModules[Index]
                if CurrentModuleName ~= "Launch" then
                    self.AddLinkOptions { "/INCLUDE:LinkModule_" .. CurrentModuleName }
                end
            end
        end

        -- Setup precompiled headers
        if self.bUsePrecompiledHeaders then
            self.AddForceIncludes { "PreCompiled.h" }
        end

        -- Make files relative before printing
        self.MakeFileNamesRelativeToPath(self.Files)
        self.MakeFileNamesRelativeToPath(self.ExcludeFiles)

        -- Add this rule to the workspace
        self.Workspace.AddRule(self)
    end

    return self
end
