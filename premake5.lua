workspace "DXR-Project"
    startproject 	"DXR-Project"
    architecture 	"x64"
    warnings 		"extra"    
	flags { "MultiProcessorCompile" }
    
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
		
		-- tinyobjloader Project
		project "tinyobjloader"
			kind "StaticLib"
			language "C++"
			cppdialect "C++17"
			systemversion "latest"
			location "Dependencies/projectfiles/tinyobjloader"
			
			filter "configurations:Debug or Release"
				symbols "on"
				runtime "Release"
				optimize "Full"
			filter{}
			
			filter "configurations:Production"
				symbols "off"
				runtime "Release"
				optimize "Full"
			filter{}
			
			-- Targets
			targetdir ("Dependencies/bin/tinyobjloader/" .. outputdir)
			objdir ("Dependencies/bin-int/tinyobjloader/" .. outputdir)
					
			-- Files
			files 
			{
				"Dependencies/tinyobjloader/tiny_obj_loader.h",
				"Dependencies/tinyobjloader/tiny_obj_loader.cc",
			}
	group ""

    -- Engine Project
    project "DXR-Project"
        language 		"C++"
        cppdialect 		"C++17"
        systemversion 	"latest"
        location 		"DXR-Project"
        kind 			"WindowedApp"
		characterset 	"Ascii"
		
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
		
		-- In visual studio show natvis files
		filter "action:vs*"
			vpaths { ["Natvis"] = "**.natvis" }
			
			files 
			{
				"Dependencies/Template-Library/Containers/*.natvis"
			}
		filter {}
			
        -- We do not want to compile HLSL files so exclude them from project
        excludes 
        {	
            "**.hlsl",
        }

		sysincludedirs
		{
			"Dependencies/imgui",
			"Dependencies/stb_image",
            "Dependencies/tinyobjloader",
            "Dependencies/Template-Library"
		}
        
		links 
		{ 
			"ImGui",
			"tinyobjloader"
		}

        -- Includes
		includedirs
		{
			"%{prj.name}",
			"%{prj.name}/Include",
        }
    project "*"