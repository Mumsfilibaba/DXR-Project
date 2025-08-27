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
        -- Name - Must be the name of the folder as well or specify the location
        Name = Name,

        -- Group - (Private) solution subfolder; can be set by path-resolution code
        Group = "",

        -- Location for IDE project files
        ProjectFilePath = "",

        -- Location for the build (inside the build folder)
        OutputPath = "",

        -- The workspace this rule is part of
        Workspace = {},

        -- Should use precompiled headers (PreCompiled.h / PreCompiled.cpp)
        bUsePrecompiledHeaders = false,

        -- Compile .cpp as Objective-C++ on macOS
        bCompileCppAsObjectiveCpp = true,

        -- RTTI
        bEnableRuntimeTypeInfo = false,

        -- Edit and Continue (VS)
        bEnableEditAndContinue = false,

        -- C++ intrinsics
        bEnableIntrinsics = true,

        -- Optimize during debug builds
        bOptimizeDebugBuild = false,

        -- Disable warnings (Useful for third-party libraries where we do not control the code)
        bSilenceWarnings = false,

        -- Toolchain / build settings
        ExceptionHandling = "Off",
        FloatingPoint = "Fast",
        VectorExtensions = "AVX2",
        Language = "C++",
        CppVersion = "C++20",
        SystemVersion = "latest",
        CharacterSet = "Ascii",

        Flags = {
            "MultiProcessorCompile",
            "NoIncrementalLink",
        },

        -- Kind (SharedLib, StaticLib, WindowedApp, ConsoleApp, etc.)
        Kind = "SharedLib",

        -- Include / link state
        IncludeDirs = {},
        ExternalIncludeDirs = {},
        ForceIncludes = {},
        LibraryPaths = {},
        Defines = {},
        Frameworks = {},

        Files = {
            "**.h",
            "**.hpp",
            "**.inl",
            "**.c",
            "**.cpp",
            "**.hlsl",
            "**.hlsli"
        },

        ExcludeFiles = {
            "**.hlsl",
            "**.hlsli"
        },

        -- macOS embedding
        bEmbedThirdparties = false,
        ExtraEmbedNames = {},

        -- Dependencies (module names)
        Modules = {},

        -- External libs / modules for linker
        LinkLibraries = {},
        LinkModules = {},

        -- Linker options (mostly Windows)
        LinkOptions = {},

        -- Post-build steps (strings)
        PostBuildCommands = {},
    }

    -- Backing storage for the module's *source* path (defaults to Runtime/<Name>)
    local BuildRulePath = JoinPath(RuntimeFolderPath, self.Name)

    -- Accessors / mutators for path & group (so ThirdParty modules can override)
    function self.GetPath()
        return BuildRulePath
    end
    function self.SetPath(NewPath)
        BuildRulePath = CreateOsPath(NewPath)
    end
    function self.SetGroup(NewGroup)
        self.Group = NewGroup or ""
    end

    -- Adders
    function self.AddFlags(InFlags) AddUniqueElements(InFlags, self.Flags) end
    function self.AddIncludeDirs(x) AddUniqueElements(x, self.IncludeDirs) end
    function self.AddExternalIncludeDirs(x) AddUniqueElements(x, self.ExternalIncludeDirs) end
    function self.AddFiles(x) AddUniqueElements(x, self.Files) end
    function self.AddExcludeFiles(x) AddUniqueElements(x, self.ExcludeFiles) end
    function self.AddDefines(x) AddUniqueElements(x, self.Defines) end
    function self.AddModules(x) AddUniqueElements(x, self.Modules) end
    function self.AddExtraEmbedNames(x) AddUniqueElements(x, self.ExtraEmbedNames) end
    function self.AddLinkLibraries(x) AddUniqueElements(x, self.LinkLibraries) end
    function self.AddFrameworks(x) AddUniqueElements(x, self.Frameworks) end
    function self.AddForceIncludes(x) AddUniqueElements(x, self.ForceIncludes) end
    function self.AddLibraryPaths(x) AddUniqueElements(x, self.LibraryPaths) end
    function self.AddLinkOptions(x) AddUniqueElements(x, self.LinkOptions) end
    function self.AddPostBuildCommands(x) AddUniqueElements(x, self.PostBuildCommands) end

    -- Helper for adding the .framework extension to frameworks (idempotent)
    local function EndsWith(str, suffix)
        return suffix ~= "" and str:sub(-#suffix) == suffix
    end

    function self.AddFrameworkExtension()
        for i = 1, #self.Frameworks do
            local FrameworkName = self.Frameworks[i]
            if not EndsWith(FrameworkName, ".framework") then
                self.Frameworks[i] = FrameworkName .. ".framework"
            end
        end
    end

    -- Makes all files relative to the module's source path
    function self.MakeFileNamesRelativeToPath(FileArray)
        for i = 1, #FileArray do
            local CurrentFile = FileArray[i]
            if not path.isabsolute(CurrentFile) then
                FileArray[i] = JoinPath(self.GetPath(), CurrentFile)
            end
        end
    end

    -- Target / object directories
    function self.GetTargetFolderPath()
        local BasePath = JoinPath(JoinPath(GetBuildFolderPath(), "bin"), GetOutputConfigPath())
        return (type(self.OutputPath) == "string" and self.OutputPath ~= "") and JoinPath(BasePath, self.OutputPath) or BasePath
    end

    function self.GetObjectFilesFolderPath()
        local BasePath   = JoinPath(JoinPath(GetBuildFolderPath(), "bin-int"), GetOutputConfigPath())
        local OutputPath = (type(self.OutputPath) == "string" and self.OutputPath ~= "") and JoinPath(BasePath, self.OutputPath) or BasePath  
        -- Need to add project name to make it unique per project (VS limitation)
        return JoinPath(OutputPath, "%{prj.name}")
    end

    -- Project generation
    function self.GenerateProject()

        -- Clear any inherited filter up front (defensive)
        filter {}

        -- Early-exit helper to clean Premake state
        local function AbortGenerateProject(...)
            if select('#', ...) > 0 then
                LogError(...)
            end

            filter {}
            project "*"
            group ""
            return nil
        end

        -- Always set a group explicitly to avoid state leaking between projects
        local GroupName = (type(self.Group) == "string" and self.Group ~= "") and self.Group or ""
        group(GroupName)

        -- Setting up project
        project(self.Name)
            LogHighlight("\n--- Generating Project '%s' ---", self.Name)

            -- Add settings based on configuration
            if self.bOptimizeDebugBuild then
                filter "configurations:Debug"
                    optimize("Full")
                filter {}
            else
                filter "configurations:Debug"
                    optimize("Off")
                filter {}
            end

            filter "configurations:Release"
                optimize("Full")
            filter {}

            filter "configurations:Production"
                optimize("Full")
            filter {}

            -- Setup warning handles
            if self.bSilenceWarnings then
                warnings("Off")
            else
                warnings("Extra")
            end

            -- Handle exception settings
            exceptionhandling(self.ExceptionHandling)

            -- Build type
            kind(self.Kind)

            -- RTTI
            rtti(self.bEnableRuntimeTypeInfo and "On" or "Off")
            floatingpoint(self.FloatingPoint)
            vectorextensions(self.VectorExtensions)

            -- Edit and Continue
            editandcontinue(self.bEnableEditAndContinue and "On" or "Off")

            -- Intrinsics
            intrinsics(self.bEnableIntrinsics and "On" or "Off")

            -- Language
            local CurrentLanguage = (self.Language or ""):upper()
            if CurrentLanguage ~= "C++" then
                return AbortGenerateProject("Invalid language '%s'", tostring(self.Language))
            else
                language(self.Language)
            end

            -- Version
            local CurrentLanguageVersion = (self.CppVersion or ""):lower()
            if not VerifyLanguageVersion(CurrentLanguageVersion) then
                return AbortGenerateProject("Invalid language version '%s'", tostring(self.CppVersion))
            else
                cppdialect(self.CppVersion)
            end

            -- /Zc:__cplusplus for VS
            filter { "action:vs*" }
                buildoptions({
                    "/Zc:__cplusplus"
                })
            filter {}

            -- System SDK
            systemversion(self.SystemVersion)

            -- Charset
            local CurrentCharacterSet = (self.CharacterSet or ""):lower()
            if CurrentCharacterSet ~= "ascii" and CurrentCharacterSet ~= "unicode" then
                return AbortGenerateProject("Invalid character set '%s'", tostring(self.CharacterSet))
            else
                characterset(self.CharacterSet)
            end

            -- Location
            self.ProjectFilePath = CreateOsPath(self.ProjectFilePath)
            LogInfo("    Project location '%s'", self.ProjectFilePath)
            location(self.ProjectFilePath)

            -- Output dirs
            local FullObjectFolderPath = self.GetTargetFolderPath()
            LogInfo("    Target location '%s'", FullObjectFolderPath)
            targetdir(FullObjectFolderPath)

            local FullIntermediateFolderPath = self.GetObjectFilesFolderPath()
            LogInfo("    Object files location '%s'", FullIntermediateFolderPath)
            objdir(FullIntermediateFolderPath)

            -- PCH
            if self.bUsePrecompiledHeaders then
                if BuildWithVisualStudio() then
                    local PchSourcePath = JoinPath(self.GetPath(), "PreCompiled.cpp")
                    LogHighlight("    PreCompiled source path '%s'", PchSourcePath)
                    
                    local UnixPchSourcePath = path.translate(PchSourcePath, '/')
                    pchheader("PreCompiled.h")
                    pchsource(UnixPchSourcePath)
                else
                    local PchPath = JoinPath(self.GetPath(), "PreCompiled.h")
                    pchheader(PchPath)
                end
                LogInfo("    Project is using PreCompiled Headers")
            else
                LogInfo("    Project does NOT use PreCompiled Headers")
            end

            -- Debug logging
            if _G.gSettings and _G.gSettings.bEnableDebugLogging then
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

                LogInfo("\n--- Modules used by module '%s' (Num Modules=%d) ---", self.Name, #self.Modules)
                if #self.Modules > 0 then
                    PrintTable("    Using module '%s'", self.Modules)
                end

                LogInfo("\n--- Embedded modules for module '%s' (Num Embedded Modules=%d) ---", self.Name, #self.Modules)
                if #self.Modules > 0 then
                    PrintTable("    Embed Module '%s'", self.Modules)
                end

                LogInfo("\n--- Post-Build-Commands '%s' (Num Post-Build-Commands=%d) ---", self.Name, #self.PostBuildCommands)
                if #self.PostBuildCommands > 0 then
                    PrintTable("    Post-Build-Command '%s'", self.PostBuildCommands)
                end
            end

            -- Force includes / include dirs / defines / libs / files / postbuild
            forceincludes(self.ForceIncludes)
            includedirs(self.IncludeDirs)
            externalincludedirs(self.ExternalIncludeDirs)
            defines(self.Defines)
            libdirs(self.LibraryPaths)
            files(self.Files)
            postbuildcommands(self.PostBuildCommands)

            -- Exclude OS-specific files
            if IsPlatformWindows() then
                filter { "files:**/Mac/**.cpp" }
                    flags({
                        "ExcludeFromBuild"
                    })
                filter {}
            elseif IsPlatformMac() then
                filter { "files:**/Windows/**.cpp" }
                    flags({
                        "ExcludeFromBuild"
                    })
                filter {}

                if self.bCompileCppAsObjectiveCpp then
                    filter { "files:**.cpp" }
                        compileas("Objective-C++")
                    filter {}
                end
            end

            -- VS natvis
            if BuildWithVisualStudio() then
                local NatvisPath = JoinPath(self.GetPath(), "**.natvis")
                LogHighlight("NatvisPath='%s'", NatvisPath)

                vpaths({
                    ["Natvis"] = "**.natvis"
                })
                
                files({
                    NatvisPath
                })
            end

            -- Remove files
            removefiles(self.ExcludeFiles)

            -- macOS frameworks
            if IsPlatformMac() then
                if self.Kind == "None" then
                    LogWarning("Ignoring Frameworks due to the kind being set to 'None'")
                else
                    links(self.Frameworks)
                end
            end

            -- Link / depend
            if self.Kind == "None" then
                LogWarning("Ignoring LinkLibraries due to the kind being set to 'None'")
                LogWarning("Ignoring LinkModules due to the kind being set to 'None'")
                LogWarning("Ignoring LinkOptions due to the kind being set to 'None'")
                LogWarning("Ignoring Module due to the kind being set to 'None'")
            else
                links(self.LinkLibraries)
                links(self.LinkModules)
                linkoptions(self.LinkOptions)
                dependson(self.Modules)
            end

            -- Xcode embedding
            filter { "action:xcode4" }
                if self.bEmbedThirdparties then
                    embed(self.Modules)
                    embed(self.ExtraEmbedNames)
                end
            filter {}

            -- Xcode specific settings
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

        -- End project
        project "*"

        -- Reset group
        group("")

        LogHighlight("\n--- Finished generating project files for Project '%s' ---", self.Name)
    end

    -- Base generate (generates project files)
    function self.Generate()
        if self.Workspace == nil then
            LogError("Workspace cannot be nil when generating rule")
            return
        end

        -- Ensure modules are included
        for Index = 1, #self.Modules do
            LogHighlight("\n--- Including module-dependency for module '%s' ---", self.Name)

            local CurrentModuleName = self.Modules[Index]
            if IsModule(CurrentModuleName) then
                LogHighlightWarning("- Dependency '%s' is already included", CurrentModuleName)
            else
                local function TryIncludeModuleAndGenerate(BaseDir)
                    local ModuleDir = JoinPath(BaseDir, CurrentModuleName)
                    local ModuleLua = JoinPath(ModuleDir, "Module.lua")
                    
                    if os.isdir(ModuleDir) and os.isfile(ModuleLua) then
                        LogInfo("- Including Dependency '%s' Path='%s'", CurrentModuleName, ModuleLua)
                        include(ModuleLua)

                        -- Some modules may choose not to register on certain platforms
                        if IsModule(CurrentModuleName) then
                            local CurrentModule = GetModule(CurrentModuleName)
                            CurrentModule.Workspace = self.Workspace
                            CurrentModule.Generate()
                        else
                            LogHighlightWarning("Found '%s' at '%s', but it did not get registered (it may be unsupported on this platform).", CurrentModuleName, ModuleLua)
                        end

                        return true
                    end

                    return false
                end

                -- 1) Try Runtime/<ModuleName>
                local Included = TryIncludeModuleAndGenerate(RuntimeFolderPath)

                -- 2) Try ThirdParty/<ModuleName>
                if not Included then
                    Included = TryIncludeModuleAndGenerate(GetExternalThirdpartyFolderPath())
                end

                -- 3) Give up if not found
                if not Included then
                    LogError("Module '%s' not found. Searched:\n  %s\n  %s", CurrentModuleName, 
                        JoinPath(RuntimeFolderPath, CurrentModuleName), JoinPath(GetExternalThirdpartyFolderPath(), CurrentModuleName))
                end
            end
        end

        -- Setup folder paths
        self.ProjectFilePath = GetSolutionsFolderPath()

        -- Add include root based on module location
        local function HasModuleAt(Dir)
            if not os.isdir(Dir) then
                return false
            end

            local ModuleLua = JoinPath(Dir, "Module.lua")
            return os.isfile(ModuleLua)
        end

        -- <EngineRoot>/Runtime/<ModuleName>
        local RuntimeModuleDir = JoinPath(RuntimeFolderPath, self.Name)
        -- <EngineRoot>/ThirdParty
        local ThirdPartyRootPath = GetExternalThirdpartyFolderPath()
        -- <EngineRoot>/ThirdParty/<ModuleName>
        local ThirdPartyModuleDir = JoinPath(ThirdPartyRootPath, self.Name)

        if HasModuleAt(RuntimeModuleDir) then
            -- Engine modules: allow #include "Core/..."
            self.AddExternalIncludeDirs({
                RuntimeFolderPath
            })
        elseif HasModuleAt(ThirdPartyModuleDir) then
            -- Third-party modules: allow #include <imgui/imgui.h>
            self.AddExternalIncludeDirs({
                ThirdPartyModuleDir
            })

            -- Keep ThirdParty folder organized
            self.Group = "ThirdParty"
            self.OutputPath = "ThirdParty"
        else
            LogWarning(
                "Could not locate module '%s' under Runtime or ThirdParty when setting include roots. " ..
                "Searched:\n  %s\n  %s",
                self.Name, RuntimeModuleDir, ThirdPartyModuleDir
            )
        end

        -- Add framework extension
        self.AddFrameworkExtension()

        -- Solve modules (propagate include/link info)
        for Index = 1, #self.Modules do
            local CurrentModuleName = self.Modules[Index]
            local CurrentModule     = GetModule(CurrentModuleName)

            if CurrentModule then
                if not CurrentModule.bRuntimeLinking then
                    table.insert(self.LinkModules, CurrentModuleName)
                end

                -- Import macro when linking a dynamic module at compile time
                if CurrentModule.bIsDynamic then
                    local ModuleApiName = CurrentModule.Name:upper() .. "_API"
                    if not CurrentModule.bRuntimeLinking then
                        ModuleApiName = ModuleApiName .. "=MODULE_IMPORT"
                    end

                    self.AddDefines({
                        ModuleApiName
                    })
                end

                self.AddLibraryPaths(CurrentModule.LibraryPaths)
                self.AddLinkLibraries(CurrentModule.LinkLibraries)
                self.AddFrameworks(CurrentModule.Frameworks)
                self.AddModules(CurrentModule.Modules)
                self.AddIncludeDirs(CurrentModule.IncludeDirs)
                self.AddExternalIncludeDirs(CurrentModule.ExternalIncludeDirs)
            else
                LogError("Module '%s' has not been included", CurrentModuleName)
            end
        end

        -- Add link options MSVC
        if BuildWithVisualStudio() and IsBuildMonolithic() then
            for i = 1, #self.LinkModules do
                local ModuleName = self.LinkModules[i]
                if ModuleName ~= "Launch" then
                    self.AddLinkOptions({
                        "/INCLUDE:LinkModule_" .. ModuleName
                    })
                end
            end
        end

        -- macOS / Xcode (ld64)
        if IsPlatformMac() and IsBuildMonolithic() then
            for i = 1, #self.LinkModules do
                local ModuleName = self.LinkModules[i]
                if ModuleName ~= "Launch" then
                    -- Leading underscore required for Mach-O symbol names
                    self.AddLinkOptions({
                        "-Wl,-u,_LinkModule_" .. ModuleName
                    })
                end
            end
        end

        -- PCH force-include
        if self.bUsePrecompiledHeaders then
            self.AddForceIncludes({
                "PreCompiled.h"
            })
        end

        -- Make files relative before printing
        self.MakeFileNamesRelativeToPath(self.Files)
        self.MakeFileNamesRelativeToPath(self.ExcludeFiles)

        -- Register with workspace
        self.Workspace.AddRule(self)
    end

    return self
end