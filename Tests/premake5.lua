workspace "EngineTests"
    startproject  "Core-Containers-Tests"
    architecture  "x64"
    warnings      "extra"
    language      "C++"
    cppdialect    "C++20"
    systemversion "latest"
    characterset  "Ascii"
    flags 
    {
        "MultiProcessorCompile"
    }
    
    -- Set output dir
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"

    -- Platform
    platforms
    {
        "x64",
    }

    -- Configurations
    configurations
    {
        "Debug",
        "Development",
        "Release",
    }

    -- System includes
    externalincludedirs
    {
        "../Runtime/",
    }

    -- Defines
    defines
    {
        "MONOLITHIC_BUILD=(1)",
        -- TODO: Tests should probably be compiled with the normal build pipeline
        "CORE_API=",
        "RHI_API="
    }

    filter "configurations:Debug"
        symbols "on"
        runtime "Debug"
        defines
        {
            "_DEBUG",
            "DEBUG_BUILD=(1)",
        }
    filter {}

    filter "configurations:Development"
        symbols  "on"
        runtime  "Release"
        optimize "On"
        defines
        {
            "NDEBUG",
            "DEVELOPMENT_BUILD=(1)",
        }
    filter {}

    filter "configurations:Release"
        symbols  "off"
        runtime  "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "RELEASE_BUILD=(1)",
        }
    filter {}

    -- optimize "Full" becomes -Ofast on Xcode, which folds away the NaN/infinity checks.
    filter { "configurations:Release", "system:macosx" }
        buildoptions { "-fno-fast-math" }
    filter {}

    -- IDE options
    filter "action:vs*"
        defines
        {
            "IDE_VISUAL_STUDIO",
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        }
    filter {}
    
    filter "action:xcode4"
        defines
        {
            "IDE_XCODE",
        }
    filter {}
    
    -- OS
    filter "system:windows"
        defines
        {
            "PLATFORM_WINDOWS",
        }
    filter {}
    
    filter "system:macosx"
        defines
        {
            "PLATFORM_MACOS",
        }
    filter {}

    -- Both platform folders are listed; AddPlatformRules excludes the wrong one per system.
    local CorePlatformFiles =
    {
        "../Runtime/Core/Windows/WindowsPlatformStackTrace.cpp",
        "../Runtime/Core/Windows/WindowsPlatformThread.cpp",
        "../Runtime/Core/Windows/WindowsPlatformEvent.cpp",
        "../Runtime/Core/Windows/WindowsPlatformFile.cpp",

        "../Runtime/Core/Mac/MacPlatformStackTrace.cpp",
        "../Runtime/Core/Mac/MacPlatformThread.cpp",
        "../Runtime/Core/Mac/MacPlatformEvent.cpp",
        "../Runtime/Core/Mac/MacPlatformFile.cpp",
        "../Runtime/Core/Mac/MacPlatformLibrary.cpp",
        "../Runtime/Core/Mac/MacPlatformMisc.cpp",
    }

    -- Core sources every suite compiles directly rather than linking against a built
    -- Core module. This is the union of what the individual suites used to list, so a
    -- suite that grows a new dependency does not have to rediscover this set.
    local CoreSupportFiles =
    {
        "../Runtime/Core/Misc/Asserts.cpp",
        "../Runtime/Core/Misc/CoreGlobals.cpp",
        "../Runtime/Core/Misc/OutputDeviceLogger.cpp",
        "../Runtime/Core/Misc/FileOutputDevice.cpp",
        "../Runtime/Core/Misc/ConsoleManager.cpp",
        "../Runtime/Core/Misc/CommandLine.cpp",
        "../Runtime/Core/Misc/EngineConfig.cpp",
        "../Runtime/Core/Misc/Paths.cpp",
        "../Runtime/Core/Misc/FrameProfiler.cpp",
        "../Runtime/Core/Misc/CRC.cpp",

        "../Runtime/Core/Memory/Memory.cpp",
        "../Runtime/Core/Memory/Malloc.cpp",
        "../Runtime/Core/Memory/MemoryStats.cpp",

        "../Runtime/Core/Filesystem/File.cpp",
        "../Runtime/Core/Delegates/DelegateInstance.cpp",
        "../Runtime/Core/Stats/Stats.cpp",
        "../Runtime/Core/Threading/ThreadManager.cpp",

        "../Runtime/Core/Generic/GenericPlatformThread.cpp",
        "../Runtime/Core/Generic/GenericPlatformStackTrace.cpp",

        "../Runtime/Core/Tasks/TaskEvent.cpp",
        "../Runtime/Core/Tasks/TaskBase.cpp",
        "../Runtime/Core/Tasks/TaskHandle.cpp",
        "../Runtime/Core/Tasks/TaskWorker.cpp",
        "../Runtime/Core/Tasks/TaskGraph.cpp",
        "../Runtime/Core/Tasks/TaskGraphStats.cpp",
        "../Runtime/Core/Tasks/Tasks.cpp",

        "../Runtime/Core/Time/ElapsedTime.cpp",

        -- Math sources with out-of-line definitions used by the tests.
        "../Runtime/Core/Math/Vector3.cpp",
        "../Runtime/Core/Math/Quaternion.cpp",
    }

    -- The xcode4 exporter drops forceincludes, so macOS gets the header as a compiler flag instead.
    local function AddForceInclude(headerPath)
        forceincludes { headerPath }

        filter "system:macosx"
            buildoptions { "-include", headerPath }
        filter {}
    end

    local function AddPlatformRules()
        filter "system:windows"
            links { "Dbghelp.lib", "shlwapi.lib" }
        filter {}

        filter { "system:windows", "files:**/Mac/**.cpp" }
            flags { "ExcludeFromBuild" }
        filter {}

        filter { "system:macosx", "files:**/Windows/**.cpp" }
            flags { "ExcludeFromBuild" }
        filter {}

        filter { "system:macosx", "files:**.cpp" }
            compileas "Objective-C++"
        filter {}

        filter "system:macosx"
            links { "AppKit.framework" }
        filter {}
    end

    -- Defines one test or benchmark executable.
    --
    -- Options:
    --   SourceDir         Folder holding the suite's own sources (required).
    --   Location          Where the generated project file goes. Defaults to SourceDir;
    --                     the math backend variants override it so several projects can
    --                     share one source folder.
    --   ExtraDefines      Additional preprocessor defines.
    --   ExtraBuildOptions Additional macOS compiler flags.
    local function AddTestProject(ProjectName, Options)
        local SourceDir = Options.SourceDir

        project (ProjectName)
            location (Options.Location or SourceDir)
            kind     "ConsoleApp"

            -- Targets
            targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
            objdir    ("Build/bin-int/" .. outputdir .. "/%{prj.name}")

            -- Files to include
            files
            {
                SourceDir .. "/**.h",
                SourceDir .. "/**.hpp",
                SourceDir .. "/**.inl",
                SourceDir .. "/**.c",
                SourceDir .. "/**.cpp",

                -- Shared test support (logging-backed harness, console device, macros)
                "TestCommon/**.h",
                "TestCommon/**.cpp",
            }

            files (CoreSupportFiles)
            files (CorePlatformFiles)

            -- In visual studio show natvis files
            filter "action:vs*"
                vpaths { ["Natvis"] = "**.natvis" }

                files
                {
                    SourceDir .. "/**.natvis",
                    "../Runtime/Core/Containers/**.natvis",
                    "../Runtime/Core/Templates/**.natvis",
                }
            filter {}

            -- Includes
            includedirs
            {
                ".",
                SourceDir,
            }

            -- The Core sources are compiled directly here (rather than linked), so replicate the two
            -- bits of configuration the real Core build provides: the PreCompiled.h force-include (for
            -- LOG_*, platform typedefs, etc.) and the engine-root path define used by EngineConfig.
            AddForceInclude(path.getabsolute(_MAIN_SCRIPT_DIR .. "/../Runtime/Core/PreCompiled.h"))

            defines
            {
                'ENGINE_LOCATION="' .. path.translate(path.getabsolute(_MAIN_SCRIPT_DIR .. "/.."), "/") .. '"',
                'PROJECT_LOCATION="' .. path.translate(path.getabsolute(_MAIN_SCRIPT_DIR), "/") .. '"',
                'PROJECT_NAME="' .. ProjectName .. '"',
            }

            if Options.ExtraDefines then
                defines (Options.ExtraDefines)
            end

            -- Clang defaults to Penryn on macOS, so levels above SSE4.1 need an explicit flag.
            if Options.ExtraBuildOptions then
                filter "system:macosx"
                    buildoptions (Options.ExtraBuildOptions)
                filter {}
            end

            AddPlatformRules()
        project "*"
    end

    -- ============================================================================
    --  Core
    -- ============================================================================

    AddTestProject("Core-Tests",            { SourceDir = "Core/Core-Tests" })
    AddTestProject("Core-Containers-Tests", { SourceDir = "Core/Core-Containers-Tests" })
    AddTestProject("Core-Templates-Tests",  { SourceDir = "Core/Core-Templates-Tests" })
    AddTestProject("Core-Benchmarks",       { SourceDir = "Core/Core-Benchmarks" })

    -- Math Tests
    --
    -- The math library selects its vector backend at preprocess time (highest enabled SSE level
    -- wins; scalar fallback otherwise). On x64/MSVC that always resolves to SSE4.2, so the scalar
    -- and lower-SSE backends are never *selected* and therefore never tested. To exercise every
    -- backend we generate one identical test executable per level and pin the selection by forcing
    -- the PLATFORM_SUPPORT_*_INTRIN macros above the target level to 0 (see Core/CoreDefines.h,
    -- which defines each level behind an #ifndef so these project defines win).
    --
    -- NOTE: every variant compiles the same Core/Core-Math-Tests sources, so only Location differs.
    local function AddMathTest(Suffix, PinnedDefines, ExtraBuildOptions)
        AddTestProject("Core-Math-Tests-" .. Suffix,
        {
            SourceDir         = "Core/Core-Math-Tests",
            Location          = "Core/Core-Math-Tests-" .. Suffix,
            ExtraDefines      = PinnedDefines,
            ExtraBuildOptions = ExtraBuildOptions,
        })
    end

    AddMathTest("Scalar", { "PLATFORM_SUPPORT_SSE_INTRIN=0" })
    AddMathTest("SSE",    { "PLATFORM_SUPPORT_SSE2_INTRIN=0", "PLATFORM_SUPPORT_SSE3_INTRIN=0", "PLATFORM_SUPPORT_SSSE3_INTRIN=0", "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathTest("SSE2",   { "PLATFORM_SUPPORT_SSE3_INTRIN=0", "PLATFORM_SUPPORT_SSSE3_INTRIN=0", "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathTest("SSE3",   { "PLATFORM_SUPPORT_SSSE3_INTRIN=0", "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathTest("SSSE3",  { "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathTest("SSE4_1", { "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathTest("SSE4_2", nil, { "-msse4.2" })

    -- ============================================================================
    --  RHI
    -- ============================================================================

    AddTestProject("RHI-Tests", { SourceDir = "RHI/RHI-Tests" })
