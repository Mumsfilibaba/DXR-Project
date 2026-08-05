include "BuildTool_Common.lua"

-- The xcode4 exporter drops vectorextensions, so the equivalent clang flag is passed by hand.
local ClangVectorExtensionFlags =
{
    ["AVX512"] = "-mavx512f",
    ["AVX2"]   = "-mavx2",
    ["AVX"]    = "-mavx",
    ["SSE4.2"] = "-msse4.2",
    ["SSE4.1"] = "-msse4.1",
    ["SSSE3"]  = "-mssse3",
    ["SSE3"]   = "-msse3",
    ["SSE2"]   = "-msse2",
    ["SSE"]    = "-msse",
}

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
        MacOSVersion = "15.0",
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

        -- Source paths of extra dylibs to copy into the bundle
        ExtraRuntimeLibraries = {},

        -- Dependencies (module names)
        Modules = {},

        -- External libs / modules for linker
        LinkLibraries = {},
        LinkModules = {},

        -- Linker options (mostly Windows)
        LinkOptions = {},

        -- Names whose LinkModule_ symbol must survive dead-stripping in a monolithic build
        -- even though they are not module dependencies, such as the game library
        ForceLinkNames = {},

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

    -- Most rules have one kind regardless of layout. Module rules and the game target
    -- override this, because Visual Studio switches them per configuration.
    function self.KindIn(Layout)
        return self.Kind
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
    function self.AddExtraRuntimeLibraries(InExtraRuntimeLibraries) AddUniqueElements(InExtraRuntimeLibraries, self.ExtraRuntimeLibraries) end
    function self.AddLinkLibraries(InLinkLibraries) AddUniqueElements(InLinkLibraries, self.LinkLibraries) end
    function self.AddFrameworks(InFrameworks) AddUniqueElements(InFrameworks, self.Frameworks) end
    function self.AddForceIncludes(InForceIncludes) AddUniqueElements(InForceIncludes, self.ForceIncludes) end
    function self.AddLibraryPaths(InLibraryPaths) AddUniqueElements(InLibraryPaths, self.LibraryPaths) end
    function self.AddLinkOptions(InLinkOptions) AddUniqueElements(InLinkOptions, self.LinkOptions) end
    function self.AddForceLinkNames(InForceLinkNames) AddUniqueElements(InForceLinkNames, self.ForceLinkNames) end
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
                filter "configurations:*Debug*"
                    optimize("Full")
                filter {}
            else
                filter "configurations:*Debug*"
                    optimize("Off")
                filter {}
            end

            filter "configurations:*Release*"
                optimize("Full")
            filter {}

            -- Setup warning handles
            if self.bSilenceWarnings then
                warnings("Off")
            else
                warnings("Extra")

                -- Only compile warnings are promoted. Linker diagnostics such as LNK4098 are not
                -- actionable from here and would fail the build for reasons unrelated to the code.
                if IsFatalWarnings() then
                    flags({
                        "FatalCompileWarnings"
                    })
                end
            end

            -- Handle exception settings
            exceptionhandling(self.ExceptionHandling)

            -- Add flags
            flags(self.Flags)

            -- Run-Time Type Information
            rtti(self.bEnableRuntimeTypeInfo and "On" or "Off")
            floatingpoint(self.FloatingPoint)
            vectorextensions(self.VectorExtensions)

            -- Neither of the settings above reaches Xcode, which would leave macOS on clang's
            -- default Penryn baseline and a different VectorMath backend than Windows.
            if IsPlatformMac() then
                local VectorFlag = ClangVectorExtensionFlags[self.VectorExtensions]
                if VectorFlag then
                    filter { "system:macosx" }
                        buildoptions({
                            VectorFlag
                        })
                    filter {}
                elseif self.VectorExtensions and self.VectorExtensions ~= "Default" then
                    LogWarning("No clang flag known for VectorExtensions '%s'", tostring(self.VectorExtensions))
                end

                -- Fast floating point in an optimized build implies -ffinite-math-only, which folds
                -- every NaN/infinity check to false. Keep the rest of fast-math.
                if self.FloatingPoint == "Fast" then
                    filter { "system:macosx" }
                        buildoptions({
                            "-fno-finite-math-only"
                        })
                    filter {}
                end
            end

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

            -- Conforming preprocessor and __cplusplus value for VS. The traditional MSVC
            -- preprocessor mis-expands __VA_ARGS__, which CHECKF relies on.
            filter { "action:vs*" }
                buildoptions({
                    "/Zc:__cplusplus",
                    "/Zc:preprocessor"
                })
            filter {}

            -- System SDK. "latest" picks the newest Windows SDK, but Xcode maps this
            -- straight to MACOSX_DEPLOYMENT_TARGET, where it becomes an unparseable
            -- LSMinimumSystemVersion that no run destination can satisfy.
            if IsPlatformMac() then
                systemversion(self.MacOSVersion)
            else
                systemversion(self.SystemVersion)
            end

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
            local FullTargetFolderPath = self.GetTargetFolderPath()
            LogInfo("Target location '%s'", FullTargetFolderPath)
            targetdir(FullTargetFolderPath)

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

            for _, Layout in ipairs(GetGeneratedLayouts()) do
                local Result     = self.LayoutResults[Layout]
                local LayoutName = (Layout == ELayout.Monolithic) and "Monolithic" or "Modular"

                LogInfo("--- %s link modules for module '%s' (Num LinkModules=%d) ---", LayoutName, self.Name, #Result.LinkModules)
                if #Result.LinkModules > 0 then
                    PrintTable("  Linking module '%s'", Result.LinkModules)
                end

                LogInfo("--- %s link options for module '%s' (Num LinkOptions=%d) ---", LayoutName, self.Name, #Result.LinkOptions)
                if #Result.LinkOptions > 0 then
                    PrintTable("  Link options '%s'", Result.LinkOptions)
                end
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

            -- Link / depend. Kind is emitted here because it varies per layout.
            if self.Kind == "None" then
                kind(self.Kind)

                LogWarning("Ignoring LinkLibraries due to the kind being set to 'None'")
                LogWarning("Ignoring LinkModules due to the kind being set to 'None'")
                LogWarning("Ignoring LinkOptions due to the kind being set to 'None'")
                LogWarning("Ignoring Module due to the kind being set to 'None'")
            else
                for _, Layout in ipairs(GetGeneratedLayouts()) do
                    local Result       = self.LayoutResults[Layout]
                    local LayoutKind   = self.KindIn(Layout)
                    local LayoutFilter = GetLayoutConfigFilter(Layout)

                    -- nil on Xcode, where the generation is a single layout already
                    if LayoutFilter then
                        filter(LayoutFilter)
                    end

                    kind(LayoutKind)
                    defines(Result.Defines)
                    linkoptions(Result.LinkOptions)

                    if LayoutKind ~= "StaticLib" then
                        links(self.LinkLibraries)
                        links(Result.LinkModules)
                    else
                        dependson(Result.LinkModules)
                    end

                    dependson(ExcludeElements(self.Modules, Result.LinkModules))

                    if LayoutFilter then
                        filter({})
                    end
                end
            end

            -- Xcode embedding
            if BuildWithXcode() then
                if self.bEmbedThirdparties then
                    embed(self.Modules)
                    embed(self.ExtraEmbedNames)
                end

                -- embed() only decorates entries that are also linked, so runtime-loaded modules
                -- and thirdparty dylibs never reach the bundle. Copy them in by hand instead.
                if self.Kind == "WindowedApp" then
                    local TargetPath = self.GetTargetFolderPath()

                    -- Xcode only ever generates one layout, so there is exactly one result
                    local Layout = GetGeneratedLayouts()[1]
                    local Result = self.LayoutResults[Layout]

                    local RuntimeLibraries = {}
                    for _, ModuleName in ipairs(ExcludeElements(self.Modules, Result.LinkModules)) do
                        local ModuleRule = GetModuleRule(ModuleName)
                        if ModuleRule and ModuleRule.IsDynamicIn(Layout) then
                            table.insert(RuntimeLibraries, JoinPath(TargetPath, "lib" .. ModuleName .. ".dylib"))
                        end
                    end

                    AddUniqueElements(self.ExtraRuntimeLibraries, RuntimeLibraries)

                    LogInfo("--- Bundled runtime libraries for '%s' (Num=%d) ---", self.Name, #RuntimeLibraries)
                    if #RuntimeLibraries > 0 then
                        PrintTable("  Bundle '%s'", RuntimeLibraries)

                        local FrameworksPath = JoinPath(TargetPath, self.Name .. ".app/Contents/Frameworks")

                        local CopyCommands = {
                            ('mkdir -p "%s"'):format(FrameworksPath)
                        }

                        for _, SourcePath in ipairs(RuntimeLibraries) do
                            table.insert(CopyCommands, ('if [ -f "%s" ]; then cp -f "%s" "%s/"; fi'):format(SourcePath, SourcePath, FrameworksPath))
                        end

                        postbuildcommands(CopyCommands)
                    end
                end
            end

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
                    -- Xcode otherwise defaults to /usr/local/lib, and dyld resolves an absolute
                    -- install name directly rather than against LC_RPATH
                    ["DYLIB_INSTALL_NAME_BASE"] = "@rpath",
                    ["LD_RUNPATH_SEARCH_PATHS"] = "@executable_path/../Frameworks @executable_path @loader_path",
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
            elseif ModuleInfo.Root == "ThirdParty" then
                -- Include the module's actual folder so consumers can do something like '#include <ModuleName/...>'
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
            else
                -- Any other search root (Tests, for one). Expose the module's parent folder
                -- so consumers can keep writing '#include <ModuleName/...>'.
                ModuleRule.AddExternalIncludeDirs({
                    CreateOsPath(path.getdirectory(ModuleInfo.ScriptDir))
                })

                local RootDir      = ModuleInfo.RootDir or GetEnginePath()
                local RelativePath = CreateOsPath(path.getrelative(RootDir, ModuleInfo.ScriptDir))

                -- Fix grouping (only if not already set)
                if (not ModuleRule.Group) or ModuleRule.Group == "" then
                    ModuleRule.SetGroup(ModuleInfo.Root .. "/" .. (RelativePath:gsub("\\", "/")))
                end

                -- Fix output-path (only if not already set)
                if (not ModuleRule.OutputPathOverride) or ModuleRule.OutputPathOverride == "" then
                    ModuleRule.OutputPathOverride = JoinPath(ModuleInfo.Root, RelativePath)
                end

                -- Fix project-path (only if not already set)
                if (not ModuleRule.ProjectFilePathOverride) or ModuleRule.ProjectFilePathOverride == "" then
                    ModuleRule.ProjectFilePathOverride = JoinPath(ModuleInfo.Root, RelativePath)
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

        local function HasLinkModuleSymbol(ModuleName)
            if ModuleName == "Launch" then
                return false
            end

            local ModuleRule = GetModuleRule(ModuleName)
            return not (ModuleRule and ModuleRule.bIsLibrary)
        end

        self.LayoutResults = {}

        for _, Layout in ipairs(GetGeneratedLayouts()) do
            local Result = {
                LinkModules = {},
                Defines     = {},
                LinkOptions = {},
            }

            AddUniqueElements(self.LinkOptions, Result.LinkOptions)

            local Index = 1
            while Index <= #self.Modules do
                local CurrentModuleName = self.Modules[Index]
                local CurrentModule     = GetModuleRule(CurrentModuleName)

                if CurrentModule then
                    if not CurrentModule.IsRuntimeLinkedIn(Layout) then
                        table.insert(Result.LinkModules, CurrentModuleName)
                    end

                    -- Third-party libraries own their API macro, so only engine modules get one here
                    if not CurrentModule.bIsLibrary then
                        local ModuleApiName = CurrentModule.Name:upper() .. "_API"
                        if CurrentModule.IsDynamicIn(Layout) and not CurrentModule.IsRuntimeLinkedIn(Layout) then
                            ModuleApiName = ModuleApiName .. "=MODULE_IMPORT"
                        else
                            ModuleApiName = ModuleApiName .. "="
                        end

                        AddUniqueElements({ ModuleApiName }, Result.Defines)
                    end

                    self.AddLibraryPaths(CurrentModule.LibraryPaths)
                    self.AddLinkLibraries(CurrentModule.LinkLibraries)
                    self.AddFrameworks(CurrentModule.Frameworks)
                    self.AddModules(CurrentModule.Modules)
                    self.AddIncludeDirs(CurrentModule.IncludeDirs)
                    self.AddExternalIncludeDirs(CurrentModule.ExternalIncludeDirs)
                    self.AddExtraRuntimeLibraries(CurrentModule.ExtraRuntimeLibraries)
                else
                    LogError("Module '%s' has not been included", CurrentModuleName)
                end

                Index = Index + 1
            end

            -- The rule's own API macro follows its own kind
            if self.ApiDefineIn then
                AddUniqueElements({ self.ApiDefineIn(Layout) }, Result.Defines)
            end

            if Layout == ELayout.Monolithic then
                AddUniqueElements({ "MONOLITHIC_BUILD=(1)" }, Result.Defines)

                -- Leading underscore required for Mach-O symbol names
                local AnchorPrefix = BuildWithVisualStudio() and "/INCLUDE:LinkModule_"
                                                              or "-Wl,-u,_LinkModule_"

                local AnchorNames = {}
                for _, ModuleName in ipairs(Result.LinkModules) do
                    if HasLinkModuleSymbol(ModuleName) then
                        table.insert(AnchorNames, ModuleName)
                    end
                end

                AddUniqueElements(self.ForceLinkNames, AnchorNames)

                for _, AnchorName in ipairs(AnchorNames) do
                    table.insert(Result.LinkOptions, AnchorPrefix .. AnchorName)
                end
            end

            self.LayoutResults[Layout] = Result
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
