workspace "DXR-Project"
    startproject "DXR-Project"
    architecture "x64"
    warnings "extra"    

    -- Set output dir
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}-%{cfg.platform}"

    -- Platform
	platforms
	{
		"x64",
    }

    -- Configurations
    configurations
    {
        "Debug",
        "Release",
        "Production",
    }

    filter "configurations:Debug"
        symbols "on"
        runtime "Debug"
        defines
        {
            "_DEBUG",
            "DEBUG_BUILD",
        }
    filter "configurations:Release"
        symbols "on"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "RELEASE_BUILD",
        }
    filter "configurations:Production"
        symbols "off"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "PRODUCTION_BUILD",
        }
    filter {}

    -- Compiler option
	filter "action:vs*"
        defines
        {
            "COMPILER_VISUAL_STUDIO",
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        }

    -- OS
    filter "system:windows"
        defines
        {
            "PLATFORM_WINDOWS",
        }
    filter {}

	-- Dependencies
	group "Dependencies"
		include "Dependencies/imgui"
	group ""

    -- Engine Project
    project "DXR-Project"
        language "C++"
        cppdialect "C++17"
        systemversion "latest"
        location "DXR-Project"
        kind "WindowedApp"
		characterset "Ascii"
		
		-- Pre-Compiled Headers
		pchheader "PreCompiled.h"
		pchsource "%{prj.name}/PreCompiled.cpp"

		forceincludes  
		{ 
			"PreCompiled.h"
		}

        -- Targets
		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	
    
        -- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.hlsl",
        }
        
        -- We do not want to compile HLSL files so exclude them from project
        excludes 
        {	
            "**.hlsl",
        }

		sysincludedirs
		{
			"Dependencies/imgui",
		}
        
		links 
		{ 
			"ImGui",
		}

        -- Includes
		includedirs
		{
			"%{prj.name}",
			"%{prj.name}/Include",
        }
    project "*"