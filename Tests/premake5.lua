workspace "EngineTests"
    startproject  "Container-Tests"
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
        "CORE_API="
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

    -- Container Tests
    project "Containers-Tests"
        location "Containers-Tests"
        kind     "ConsoleApp"

        -- Targets
        targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
        objdir    ("Build/bin-int/" .. outputdir .. "/%{prj.name}")    
    
        -- Files to include
        files 
        { 
            "%{prj.name}/**.h",
            "%{prj.name}/**.hpp",
            "%{prj.name}/**.inl",
            "%{prj.name}/**.c",
            "%{prj.name}/**.cpp",

            -- Shared test support (logging-backed harness, console device, macros)
            "TestCommon/**.h",
            "TestCommon/**.cpp",

            "../Runtime/Core/Misc/CoreGlobals.cpp",
            "../Runtime/Core/Misc/OutputDeviceLogger.cpp",
            "../Runtime/Core/Memory/Memory.cpp",
            "../Runtime/Core/Memory/Malloc.cpp",
            "../Runtime/Core/Memory/MemoryStats.cpp",
            "../Runtime/Core/Stats/Stats.cpp",
            "../Runtime/Core/Delegates/DelegateInstance.cpp",
            "../Runtime/Core/Generic/GenericPlatformThread.cpp",
            "../Runtime/Core/Generic/GenericPlatformStackTrace.cpp",
            "../Runtime/Core/Threading/ThreadManager.cpp",
            "../Runtime/Core/Misc/CRC.cpp",

            -- TODO: Add Mac specifics
            "../Runtime/Core/Windows/WindowsPlatformStackTrace.cpp",
            "../Runtime/Core/Windows/WindowsPlatformThread.cpp",
            "../Runtime/Core/Windows/WindowsPlatformEvent.cpp",
            "../Runtime/Core/Windows/WindowsPlatformFile.cpp",
        }
            
        -- In visual studio show natvis files
        filter "action:vs*"
            vpaths { ["Natvis"] = "**.natvis" }
            
            files 
            {
                "%{prj.name}/**.natvis",
                "../Runtime/Core/Containers/**.natvis",
                "../Runtime/Core/Templates/**.natvis",
            }
        filter {}

        -- Includes
        includedirs
        {
            ".",
            "%{prj.name}",
        }

        -- The directly-compiled Core platform sources (e.g. WindowsPlatformFile.cpp) expect the
        -- logging macros the real Core build provides through its precompiled header. Force-include
        -- the lightweight logger header so LOG_* resolve without pulling in the full PCH.
        forceincludes
        {
            path.getabsolute(_MAIN_SCRIPT_DIR .. "/../Runtime/Core/Misc/OutputDeviceLogger.h"),
        }

        -- Linking
        filter "system:Windows"
            links
            {
                "Dbghelp.lib",
                "shlwapi.lib",
            }
        filter {}
    project "*"
    
    -- Math Tests
    --
    -- The math library selects its vector backend at preprocess time (highest enabled SSE level
    -- wins; scalar fallback otherwise). On x64/MSVC that always resolves to SSE4.2, so the scalar
    -- and lower-SSE backends are never *selected* and therefore never tested. To exercise every
    -- backend we generate one identical test executable per level and pin the selection by forcing
    -- the PLATFORM_SUPPORT_*_INTRIN macros above the target level to 0 (see Core/CoreDefines.h,
    -- which defines each level behind an #ifndef so these project defines win).
    local function AddMathLibTest(projectName, extraDefines)
        project (projectName)
            location (projectName)
            kind     "ConsoleApp"

            -- Targets
            targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
            objdir    ("Build/bin-int/" .. outputdir .. "/%{prj.name}")

            -- Files to include. NOTE: the test sources live in the fixed MathLib-Tests folder
            -- (not %{prj.name}), so every backend variant compiles the exact same tests.
            files
            {
                "MathLib-Tests/**.h",
                "MathLib-Tests/**.hpp",
                "MathLib-Tests/**.inl",
                "MathLib-Tests/**.c",
                "MathLib-Tests/**.cpp",

                -- Shared test support (logging-backed harness, console device, macros)
                "TestCommon/**.h",
                "TestCommon/**.cpp",

                -- Core dependencies required for the logger + allocator to link standalone.
                "../Runtime/Core/Misc/CoreGlobals.cpp",
                "../Runtime/Core/Misc/OutputDeviceLogger.cpp",
                "../Runtime/Core/Memory/Memory.cpp",
                "../Runtime/Core/Memory/Malloc.cpp",
                "../Runtime/Core/Memory/MemoryStats.cpp",
                "../Runtime/Core/Stats/Stats.cpp",
                "../Runtime/Core/Delegates/DelegateInstance.cpp",
                "../Runtime/Core/Generic/GenericPlatformThread.cpp",
                "../Runtime/Core/Generic/GenericPlatformStackTrace.cpp",
                "../Runtime/Core/Threading/ThreadManager.cpp",

                -- Math sources with out-of-line definitions used by the tests.
                "../Runtime/Core/Math/Vector3.cpp",
                "../Runtime/Core/Math/Quaternion.cpp",

                -- TODO: Add Mac specifics
                "../Runtime/Core/Windows/WindowsPlatformStackTrace.cpp",
                "../Runtime/Core/Windows/WindowsPlatformThread.cpp",
                "../Runtime/Core/Windows/WindowsPlatformEvent.cpp",
                "../Runtime/Core/Windows/WindowsPlatformFile.cpp",
            }

            -- In visual studio show natvis files
            filter "action:vs*"
                vpaths { ["Natvis"] = "**.natvis" }

                files
                {
                    "MathLib-Tests/**.natvis",
                }
            filter {}

            -- Includes
            includedirs
            {
                ".",
                "MathLib-Tests",
            }

            -- The directly-compiled Core platform sources (e.g. WindowsPlatformFile.cpp) expect the
            -- logging macros the real Core build provides through its precompiled header. Force-include
            -- the lightweight logger header so LOG_* resolve without pulling in the full PCH.
            forceincludes
            {
                path.getabsolute(_MAIN_SCRIPT_DIR .. "/../Runtime/Core/Misc/OutputDeviceLogger.h"),
            }

            -- Backend-pinning defines (see comment above). SSE4.2 passes none and uses the default.
            if extraDefines then
                defines (extraDefines)
            end

            -- Linking
            filter "system:Windows"
                links
                {
                    "Dbghelp.lib",
                    "shlwapi.lib",
                }
            filter {}
        project "*"
    end

    AddMathLibTest("MathLib-Tests-Scalar", { "PLATFORM_SUPPORT_SSE_INTRIN=0" })
    AddMathLibTest("MathLib-Tests-SSE",    { "PLATFORM_SUPPORT_SSE2_INTRIN=0", "PLATFORM_SUPPORT_SSE3_INTRIN=0", "PLATFORM_SUPPORT_SSSE3_INTRIN=0", "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathLibTest("MathLib-Tests-SSE2",   { "PLATFORM_SUPPORT_SSE3_INTRIN=0", "PLATFORM_SUPPORT_SSSE3_INTRIN=0", "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathLibTest("MathLib-Tests-SSE3",   { "PLATFORM_SUPPORT_SSSE3_INTRIN=0", "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathLibTest("MathLib-Tests-SSSE3",  { "PLATFORM_SUPPORT_SSE4_1_INTRIN=0", "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathLibTest("MathLib-Tests-SSE4_1", { "PLATFORM_SUPPORT_SSE4_2_INTRIN=0" })
    AddMathLibTest("MathLib-Tests-SSE4_2", nil)
    
    -- Templates Tests
    project "Templates-Tests"
        location "Templates-Tests"
        kind     "ConsoleApp"

        -- Targets
        targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
        objdir    ("Build/bin-int/" .. outputdir .. "/%{prj.name}")    
    
        -- Files to include
        files 
        { 
            "%{prj.name}/**.h",
            "%{prj.name}/**.hpp",
            "%{prj.name}/**.inl",
            "%{prj.name}/**.c",
            "%{prj.name}/**.cpp",

            -- Shared test support (logging-backed harness, console device, macros)
            "TestCommon/**.h",
            "TestCommon/**.cpp",

            -- Core dependencies required for the logger + allocator to link standalone.
            "../Runtime/Core/Misc/CoreGlobals.cpp",
            "../Runtime/Core/Misc/OutputDeviceLogger.cpp",
            "../Runtime/Core/Memory/Memory.cpp",
            "../Runtime/Core/Memory/Malloc.cpp",
            "../Runtime/Core/Memory/MemoryStats.cpp",
            "../Runtime/Core/Stats/Stats.cpp",
            "../Runtime/Core/Delegates/DelegateInstance.cpp",
            "../Runtime/Core/Generic/GenericPlatformThread.cpp",
            "../Runtime/Core/Generic/GenericPlatformStackTrace.cpp",
            "../Runtime/Core/Threading/ThreadManager.cpp",

            -- TODO: Add Mac specifics
            "../Runtime/Core/Windows/WindowsPlatformStackTrace.cpp",
            "../Runtime/Core/Windows/WindowsPlatformThread.cpp",
            "../Runtime/Core/Windows/WindowsPlatformEvent.cpp",
            "../Runtime/Core/Windows/WindowsPlatformFile.cpp",
        }
            
        -- In visual studio show natvis files
        filter "action:vs*"
            vpaths { ["Natvis"] = "**.natvis" }
            
            files 
            {
                "%{prj.name}/**.natvis",
            }
        filter {}

        -- Includes
        includedirs
        {
            ".",
            "%{prj.name}",
        }

        -- The directly-compiled Core platform sources (e.g. WindowsPlatformFile.cpp) expect the
        -- logging macros the real Core build provides through its precompiled header. Force-include
        -- the lightweight logger header so LOG_* resolve without pulling in the full PCH.
        forceincludes
        {
            path.getabsolute(_MAIN_SCRIPT_DIR .. "/../Runtime/Core/Misc/OutputDeviceLogger.h"),
        }

        -- Linking
        filter "system:Windows"
            links
            {
                "Dbghelp.lib",
                "shlwapi.lib",
            }
        filter {}
    project "*"

    -- Core Tests (task graph)
    project "CoreTests"
        location "CoreTests"
        kind     "ConsoleApp"

        -- Targets
        targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
        objdir    ("Build/bin-int/" .. outputdir .. "/%{prj.name}")

        -- Files to include
        files
        {
            "%{prj.name}/**.h",
            "%{prj.name}/**.hpp",
            "%{prj.name}/**.inl",
            "%{prj.name}/**.c",
            "%{prj.name}/**.cpp",

            -- Shared test support (logging-backed harness, console device, macros)
            "TestCommon/**.h",
            "TestCommon/**.cpp",

            -- Core dependencies required to link the task graph standalone.
            "../Runtime/Core/Misc/CoreGlobals.cpp",
            "../Runtime/Core/Misc/OutputDeviceLogger.cpp",
            "../Runtime/Core/Misc/FileOutputDevice.cpp",
            "../Runtime/Core/Misc/ConsoleManager.cpp",
            "../Runtime/Core/Misc/CommandLine.cpp",
            "../Runtime/Core/Misc/EngineConfig.cpp",
            "../Runtime/Core/Misc/Paths.cpp",
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
            "../Runtime/Core/Misc/FrameProfiler.cpp",
            "../Runtime/Core/Time/ElapsedTime.cpp",

            -- TODO: Add Mac specifics
            "../Runtime/Core/Windows/WindowsPlatformStackTrace.cpp",
            "../Runtime/Core/Windows/WindowsPlatformThread.cpp",
            "../Runtime/Core/Windows/WindowsPlatformEvent.cpp",
            "../Runtime/Core/Windows/WindowsPlatformFile.cpp",
        }

        -- Includes
        includedirs
        {
            ".",
            "%{prj.name}",
        }

        -- The Core sources are compiled directly here (rather than linked), so replicate the two
        -- bits of configuration the real Core build provides: the PreCompiled.h force-include (for
        -- LOG_*, platform typedefs, etc.) and the engine-root path define used by EngineConfig.
        forceincludes
        {
            path.getabsolute(_MAIN_SCRIPT_DIR .. "/../Runtime/Core/PreCompiled.h"),
        }

        defines
        {
            'ENGINE_LOCATION="' .. path.translate(path.getabsolute(_MAIN_SCRIPT_DIR .. "/.."), "/") .. '"',
            'PROJECT_LOCATION="' .. path.translate(path.getabsolute(_MAIN_SCRIPT_DIR), "/") .. '"',
            'PROJECT_NAME="CoreTests"',
        }

        -- Linking
        filter "system:Windows"
            links
            {
                "Dbghelp.lib",
                "shlwapi.lib",
            }
        filter {}
    project "*"
    