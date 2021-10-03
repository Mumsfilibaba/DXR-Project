function findProjectDir()
	local projectName = "/%{prj.name}"
	return os.getcwd() .. projectName 
end

workspace "DXR-Project"
    startproject     "Sandbox"
    architecture      "x64"
    warnings          "extra"
	exceptionhandling "Off"
	rtti              "Off"
	floatingpoint     "Fast"
	vectorextensions  "SSE2"
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
            "DEBUG_BUILD=(1)",
        }
    filter "configurations:Release"
        symbols "on"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "RELEASE_BUILD=(1)",
        }
    filter "configurations:Production"
        symbols "off"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "PRODUCTION_BUILD=(1)",
        }
    filter {}

	-- Architecture defines
	filter "architecture:x86"
		defines
		{
			"ARCHITECTURE_X86=1",
		}
	filter "architecture:x86_x64"
		defines
		{
			"ARCHITECTURE_X86_X64=1",
		}
	filter "architecture:ARM"
		defines
		{
			"ARCHITECTURE_ARM=1",
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

    -- OS
    filter "system:windows"
        defines
        {
            "PLATFORM_WINDOWS",
        }
	filter "System:macosx"
		defines
		{
			"PLATFORM_MACOS",
		}
    filter {}

	-- Dependencies
	group "Dependencies"
		-- Imgui
		include "Dependencies/imgui"
		
		-- tinyobjloader Project
		project "tinyobjloader"
			kind 			"StaticLib"
			language 		"C++"
			cppdialect 		"C++17"
			systemversion 	"latest"
			location 		"Dependencies/projectfiles/tinyobjloader"
			
			filter "configurations:Debug or Release"
				symbols 	"on"
				runtime 	"Release"
				optimize 	"Full"
			filter{}
			
			filter "configurations:Production"
				symbols 	"off"
				runtime 	"Release"
				optimize 	"Full"
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
		
		-- OpenFBX Project
		project "OpenFBX"
			kind 			"StaticLib"
			language 		"C++"
			cppdialect 		"C++17"
			systemversion 	"latest"
			location 		"Dependencies/projectfiles/OpenFBX"
			
			filter "configurations:Debug or Release"
				symbols 	"on"
				runtime 	"Release"
				optimize 	"Full"
			filter{}
			
			filter "configurations:Production"
				symbols 	"off"
				runtime 	"Release"
				optimize 	"Full"
			filter{}
			
			-- Targets
			targetdir ("Dependencies/bin/OpenFBX/" .. outputdir)
			objdir ("Dependencies/bin-int/OpenFBX/" .. outputdir)
					
			-- Files
			files 
			{
				"Dependencies/OpenFBX/src/ofbx.h",
				"Dependencies/OpenFBX/src/ofbx.cpp",
				"Dependencies/OpenFBX/src/miniz.h",
				"Dependencies/OpenFBX/src/miniz.c",
			}
	group ""

    -- Engine Project
    project "DXR-Engine"
        language 		"C++"
        cppdialect 		"C++17"
        systemversion 	"latest"
        location 		"DXR-Engine"
        kind 			"StaticLib"
		characterset 	"Ascii"
		
		-- Pre-Compiled Headers
		pchheader "PreCompiled.h"
		pchsource "%{prj.name}/PreCompiled.cpp"

		forceincludes  
		{ 
			"PreCompiled.h"
		}

        -- Targets
		targetdir 	("Build/bin/" .. outputdir)
		objdir 		("Build/bin-int/" .. outputdir)	
    
        -- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.hlsl",
			"%{prj.name}/**.hlsli",	
        }
		
		excludes 
		{
			"DXR-Engine/Main/**",
			"DXR-Engine/Math/Tests/**",
		}
		
		-- Remove non-macos and add macos-specific files
		filter "system:macosx"
			files 
			{ 
				"%{prj.name}/**.mm",
			}

			excludes 
			{
				"DXR-Engine/D3D12/**",
				"DXR-Engine/Core/Windows/**",
				"DXR-Engine/Core/Application/Windows/**",
				"DXR-Engine/Core/Threading/Windows/**",
				"DXR-Engine/Core/Time/Windows/**",
			}
		filter {}

		-- In visual studio show natvis files
		filter "action:vs*"
			vpaths { ["Natvis"] = "**.natvis" }
			
			files 
			{
				"%{prj.name}/**.natvis",
			}
		filter {}
			
        -- We do not want to compile HLSL files so exclude them from project
        excludes 
        {	
            "**.hlsl",
			"**.hlsli",
        }

		sysincludedirs
		{
			"Dependencies/imgui",
			"Dependencies/stb_image",
            "Dependencies/tinyobjloader",
            "Dependencies/OpenFBX/src",
		}
        
		links 
		{ 
			"ImGui",
			"tinyobjloader",
			"OpenFBX"
		}

		filter "system:macosx"
			links
			{
				-- Native
				"Cocoa.framework",
				"AppKit.framework",
				"MetalKit.framework",
			}
		filter {}

        -- Includes
		includedirs
		{
			"%{prj.name}",
			"%{prj.name}/Include",
        }
		
    project "*"
	
	-- Sandbox Project
    project "Sandbox"
		language 		"C++"
        cppdialect 		"C++17"
        systemversion 	"latest"
        location 		"Sandbox"
        kind 			"WindowedApp"
		characterset 	"Ascii"
	
	    -- Targets
		targetdir 	("Build/bin/" .. outputdir)
		objdir 		("Build/bin-int/" .. outputdir)
	
		sysincludedirs
		{
			"DXR-Engine",	
			"Dependencies/imgui",
		}
		
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
		
		defines
		{
			"PROJECT_LOCATION=" .. "\"" .. findProjectDir().. "\""
		}
		
		-- Include Windows Main
		filter "system:windows"
			files
			{
				"DXR-Engine/Main/Windows/WindowsMain.cpp",	
			}
		filter "system:macosx"
			files
			{
				"DXR-Engine/Main/Mac/MacMain.cpp",	
			}
		filter {}
		
		-- In visual studio show natvis files
		filter "action:vs*"
			vpaths { ["Natvis"] = "**.natvis" }
			
			files 
			{
				"%{prj.name}/**.natvis",
			}
		filter {}
			
        -- We do not want to compile HLSL files so exclude them from project
        excludes 
        {	
            "**.hlsl",
			"**.hlsli",
        }
	
		links
		{ 
			"DXR-Engine",
		}
		
		-- TODO: Check why this is necessary
		filter "system:macosx"
			links
			{
				-- Native
				"Cocoa.framework",
				"AppKit.framework",
				"MetalKit.framework",
			}
		filter {}

	project "*"
	