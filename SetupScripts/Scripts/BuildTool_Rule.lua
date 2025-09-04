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

        -- Group - (Private) solution subfolder
        Group = "",

        -- Location for project files, this overrides the default behavior
        ProjectFilePathOverride = "",

        -- Location for the build, this overrides the default behavior
        OutputPathOverride = "",

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

        -- Is this rule generated yet? (rules should only be generated once)
        bIsGenerated = false
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
        self.Group = NewGroup
    end

    function self.IsGenerated()
        return self.bIsGenerated
    end

    -- Adders
    function self.AddFlags(InFlags) AddUniqueElements(InFlags, self.Flags) end
    function self.AddIncludeDirs(InIncludeDirs) AddUniqueElements(InIncludeDirs, self.IncludeDirs) end
    function self.AddExternalIncludeDirs(InExternalIncludeDirs) AddUniqueElements(InExternalIncludeDirs, self.ExternalIncludeDirs) end
    function self.AddFiles(InFiles) AddUniqueElements(InFiles, self.Files) end
    function self.SetFiles(InFiles) self.Files = InFiles end
    function self.AddExcludeFiles(InExcludeFiles) AddUniqueElements(InExcludeFiles, self.ExcludeFiles) end
    function self.AddDefines(InDefines) AddUniqueElements(InDefines, self.Defines) end
    function self.AddModules(InModules) AddUniqueElements(InModules, self.Modules) end
    function self.AddExtraEmbedNames(InExtraEmbedNames) AddUniqueElements(InExtraEmbedNames, self.ExtraEmbedNames) end
    function self.AddLinkLibraries(InLinkLibraries) AddUniqueElements(InLinkLibraries, self.LinkLibraries) end
    function self.AddFrameworks(InFrameworks) AddUniqueElements(InFrameworks, self.Frameworks) end
    function self.AddForceIncludes(InForceIncludes) AddUniqueElements(InForceIncludes, self.ForceIncludes) end
    function self.AddLibraryPaths(InLibraryPaths) AddUniqueElements(InLibraryPaths, self.LibraryPaths) end
    function self.AddLinkOptions(InLinkOptions) AddUniqueElements(InLinkOptions, self.LinkOptions) end
    function self.AddPostBuildCommands(InPostBuildCommands) AddUniqueElements(InPostBuildCommands, self.PostBuildCommands) end

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

    -- Solution directory
    function self.GetProjectFolderPath()
        local BasePath = CreateOsPath(GetSolutionsFolderPath())
        return (type(self.ProjectFilePathOverride) == "string" and self.ProjectFilePathOverride ~= "") and JoinPath(BasePath, self.ProjectFilePathOverride) or BasePath
    end

    -- Target directory
    function self.GetTargetFolderPath()
        local BasePath = JoinPath(JoinPath(GetBuildFolderPath(), "bin"), GetOutputConfigPath())
        return (type(self.OutputPathOverride) == "string" and self.OutputPathOverride ~= "") and JoinPath(BasePath, self.OutputPathOverride) or BasePath
    end

    -- Object files directory
    function self.GetObjectFilesFolderPath()
        local BasePath = JoinPath(JoinPath(GetBuildFolderPath(), "bin-int"), GetOutputConfigPath())
        local OutputPathOverride = (type(self.OutputPathOverride) == "string" and self.OutputPathOverride ~= "") and JoinPath(BasePath, self.OutputPathOverride) or BasePath  
        -- Need to add project name to make it unique per project (VS limitation)
        return JoinPath(OutputPathOverride, "%{prj.name}")
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

        LogHighlight("--- Generating Project '%s' ---", self.Name)

        -- Always set a group explicitly to avoid state leaking between projects
        local GroupName = (type(self.Group) == "string" and self.Group ~= "") and self.Group or ""
        group(GroupName)

        LogHighlight("Project '%s' is using group/filter '%s'", self.Name, GroupName)

        -- Setting up project
        project(self.Name)

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

            -- Add flags
            flags(self.Flags)

            -- Run-Time Type Information
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

            -- CharacterSet
            local function MapCharacterSet(InCharacterSet)
                local CharacterSetLower = (InCharacterSet or ""):lower()
                if CharacterSetLower == "ascii" or CharacterSetLower == "mbcs" then
                    return "MBCS"
                end

                if CharacterSetLower == "unicode" then
                    return "Unicode"
                end

                return CharacterSetLower
            end

            local CurrentCharacterSet = MapCharacterSet(self.CharacterSet)
            if CurrentCharacterSet ~= "MBCS" and CurrentCharacterSet ~= "Unicode" then
                return AbortGenerateProject("Invalid character set '%s'", tostring(self.CharacterSet))
            else
                characterset(CurrentCharacterSet)
            end

            -- Location
            local FullProjectFolderPath = self.GetProjectFolderPath()
            LogInfo("Project location '%s'", FullProjectFolderPath)
            location(FullProjectFolderPath)

            -- Output dirs
            local FullObjectFolderPath = self.GetTargetFolderPath()
            LogInfo("Target location '%s'", FullObjectFolderPath)
            targetdir(FullObjectFolderPath)

            local FullIntermediateFolderPath = self.GetObjectFilesFolderPath()
            LogInfo("Object files location '%s'", FullIntermediateFolderPath)
            objdir(FullIntermediateFolderPath)

            -- PCH
            if self.bUsePrecompiledHeaders then
                if BuildWithVisualStudio() then
                    local PchSourcePath = JoinPath(self.GetPath(), "PreCompiled.cpp")
                    LogHighlight("PreCompiled source path '%s'", PchSourcePath)
                    
                    local UnixPchSourcePath = path.translate(PchSourcePath, '/')
                    pchheader("PreCompiled.h")
                    pchsource(UnixPchSourcePath)
                else
                    local PchPath = JoinPath(self.GetPath(), "PreCompiled.h")
                    pchheader(PchPath)
                end
                LogInfo("Project is using PreCompiled Headers")
            else
                LogInfo("Project does NOT use PreCompiled Headers")
            end

            -- Debug logging
            LogInfo("--- ForceIncludes for module '%s' (Num ForceIncludes=%d) ---", self.Name, #self.ForceIncludes)
            if #self.ForceIncludes > 0 then 
                PrintTable("  Using ForceInclude '%s'", self.ForceIncludes) 
            end

            LogInfo("--- Defines for module '%s' (Num Defines=%d) ---", self.Name, #self.Defines)
            if #self.Defines > 0 then 
                PrintTable("  Using define '%s'", self.Defines)
            end

            LogInfo("--- Includes for module '%s' (Num Includes=%d) ---", self.Name, #self.IncludeDirs)
            if #self.IncludeDirs > 0 then
                PrintTable("  Using Includes '%s'", self.IncludeDirs)
            end

            LogInfo("--- ExternalIncludes for module '%s' (Num ExternalIncludes=%d) ---", self.Name, #self.ExternalIncludeDirs)
            if #self.ExternalIncludeDirs > 0 then
                PrintTable("  Using ExternalInclude '%s'", self.ExternalIncludeDirs)
            end

            LogInfo("--- LibraryPaths for module '%s' (Num LibraryPaths=%d) ---", self.Name, #self.LibraryPaths)
            if #self.LibraryPaths > 0 then
                PrintTable("  Using LibraryPath '%s'", self.LibraryPaths)
            end

            LogInfo("--- Files for module '%s' (Num Files=%d) ---", self.Name, #self.Files)
            if #self.Files > 0 then
                PrintTable("  Including file '%s'", self.Files)
            end

            LogInfo("--- Exclude files for module '%s' (Num ExcludeFiles=%d) ---", self.Name, #self.ExcludeFiles)
            if #self.ExcludeFiles > 0 then
                PrintTable("  Excluding file '%s'", self.ExcludeFiles)
            end

            LogInfo("--- Frameworks for module '%s' (Num Frameworks=%d) ---", self.Name, #self.Frameworks)
            if #self.Frameworks > 0 then
                PrintTable("  Using framework '%s'", self.Frameworks)
            end

            LogInfo("--- LinkLibraries for module '%s' (Num LinkLibraries=%d) ---", self.Name, #self.LinkLibraries)
            if #self.LinkLibraries > 0 then
                PrintTable("  Linking library '%s'", self.LinkLibraries)
            end

            LogInfo("--- Link modules for module '%s' (Num LinkModules=%d) ---", self.Name, #self.LinkModules)
            if #self.LinkModules > 0 then
                PrintTable("  Linking module '%s'", self.LinkModules)
            end

            LogInfo("--- Link options for module '%s' (Num LinkOptions=%d) ---", self.Name, #self.LinkOptions)
            if #self.LinkOptions > 0 then
                PrintTable("  Link options '%s'", self.LinkOptions)
            end

            LogInfo("--- Modules used by module '%s' (Num Modules=%d) ---", self.Name, #self.Modules)
            if #self.Modules > 0 then
                PrintTable("  Using module '%s'", self.Modules)
            end

            LogInfo("--- Embedded names for module '%s' (Num=%d) ---", self.Name, #self.ExtraEmbedNames)
            if #self.ExtraEmbedNames > 0 then
                PrintTable("  Embed '%s'", self.ExtraEmbedNames)
            end

            LogInfo("--- Post-Build-Commands '%s' (Num Post-Build-Commands=%d) ---", self.Name, #self.PostBuildCommands)
            if #self.PostBuildCommands > 0 then
                PrintTable("  Post-Build-Command '%s'", self.PostBuildCommands)
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
                    ["PRODUCT_BUNDLE_IDENTIFIER"] = "com.DXREngine." .. self.Name,
                    ["CODE_SIGN_STYLE"] = "Automatic",
                    ["ARCHS"] = "x86_64",
                    ["ONLY_ACTIVE_ARCH"] = "YES",
                    ["ENABLE_HARDENED_RUNTIME"] = "NO",
                    ["GENERATE_INFOPLIST_FILE"] = "YES",
                    ["LD_RUNPATH_SEARCH_PATHS"] = "/usr/local/lib/ $(INSTALL_PATH) @executable_path/../Frameworks",
                    ["GCC_ENABLE_AVX2_EXTENSIONS"] = "YES",
                }
            filter {}

        -- End project
        project "*"

        -- Reset group
        group("")

        LogHighlight("--- Finished generating project files for Project '%s' ---", self.Name)
    end

    -- Base generate (generates project files)
    function self.Generate()

        if self.IsGenerated() then
            LogHighlightWarning("Rule '%s' already generated. Skipping ..", self.Name)
            return
        end

        local function GenerateModuleFromIndex(ModuleRule, ModuleInfo)
            -- Source path for the rule (affects file globs, natvis, etc.)
            if type(ModuleRule.SetPath) == "function" then
                ModuleRule.SetPath(ModuleInfo.ScriptDir)
            end

            -- Include roots + grouping/output
            if ModuleInfo.Root == "Runtime" then
                ModuleRule.AddExternalIncludeDirs({
                    GetRuntimeFolderPath()
                })
            else
                -- TODO: We might need to take another look at this if we add other folders than ThirdParty
                -- Other folders: include the module's actual folder so consumers can do something like '#include <ModuleName/...>'
                ModuleRule.AddExternalIncludeDirs({
                    CreateOsPath(ModuleInfo.ScriptDir)
                })

                -- Fix grouping (only if not already set)
                local RelativePath = CreateOsPath(path.getrelative(GetExternalThirdPartyFolderPath(), ModuleInfo.ScriptDir))
                if (not ModuleRule.Group) or ModuleRule.Group == "" then
                    ModuleRule.SetGroup("ThirdParty/" .. (RelativePath:gsub("\\", "/")))
                end

                -- Fix output-path (only if not already set)
                if (not ModuleRule.OutputPathOverride) or ModuleRule.OutputPathOverride == "" then
                    ModuleRule.OutputPathOverride = JoinPath("ThirdParty", RelativePath)
                end

                -- Fix project-path (only if not already set)
                if (not ModuleRule.ProjectFilePathOverride) or ModuleRule.ProjectFilePathOverride == "" then
                    ModuleRule.ProjectFilePathOverride = JoinPath("ThirdParty", RelativePath)
                end
            end

            ModuleRule.Generate()
        end

        for Index = 1, #self.Modules do
            local CurrentModuleName = self.Modules[Index]
            LogHighlight("Checking module-dependency '%s' for module '%s'", CurrentModuleName, self.Name)

            local ModuleInfo = GetIndexedModuleInfo(CurrentModuleName)
            if ModuleInfo and os.isfile(ModuleInfo.ScriptPath) then
                if IsModuleRule(CurrentModuleName) then

                    -- If it was only created (by a multi-module file) but not yet generated, do it now.
                    local ExistingRule = GetModuleRule(CurrentModuleName)
                    if not ExistingRule then
                        LogError("Error: module '%s' reported as included, but GetModuleRule() returned nil.", CurrentModuleName)
                    else
                        if not ExistingRule.IsGenerated() then
                            LogInfo("Module '%s' was created earlier but not generated. Generating now...", CurrentModuleName)
                            GenerateModuleFromIndex(ExistingRule, ModuleInfo)
                        else
                            LogHighlightWarning("Module '%s' is already included in workspace '%s'", CurrentModuleName, GetWorkspaceName())
                        end
                    end
                else
                
                    -- Include the script if it is not included yet
                    LogInfo("Including script '%s' to include module '%s'", CreateOsPath(ModuleInfo.ScriptPath), CurrentModuleName)
                    include(ModuleInfo.ScriptPath)

                    -- Some scripts may choose to not register a module for multiple reasons so check if we actually created a module
                    if IsModuleRule(CurrentModuleName) then
                        local CurrentModule = GetModuleRule(CurrentModuleName)
                        LogInfo("Module '%s' was created in script '%s'. Generating now...", CurrentModule.Name, CreateOsPath(ModuleInfo.ScriptPath))

                        GenerateModuleFromIndex(CurrentModule, ModuleInfo)
                    else
                        LogHighlightWarning("Found '%s' at '%s', but it did not register (it may be unsupported on this platform).", CurrentModuleName, CreateOsPath(ModuleInfo.ScriptPath))
                    end
                end
            else
                LogError("Module '%s' not found in indexed roots. Ensure it lives under 'Runtime' or 'ThirdParty'.", CurrentModuleName)
            end
        end

        -- Add framework extension
        self.AddFrameworkExtension()

        -- Solve modules (propagate include/link info)
        for Index = 1, #self.Modules do
            local CurrentModuleName = self.Modules[Index]
            local CurrentModule     = GetModuleRule(CurrentModuleName)

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

        -- macOS / Xcode
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
        
        -- Set that this rule has been generated and does not need to be generated again
        self.bIsGenerated = true

        -- Register with workspace
        AddProjectRule(self)
    end

    return self
end
