function findProjectDir()
	local projectName = "/%{prj.name}"
	return os.getcwd() .. projectName 
end

-- Solution
workspace "DXR-Project"
    startproject      "SandboxLauncher"
    architecture      "x64"
    warnings          "extra"
	exceptionhandling "Off"
	rtti              "Off"
	floatingpoint     "Fast"
	vectorextensions  "SSE2"
	
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

-- Dependencies
group "Dependencies"
	-- Imgui
	project "ImGui"
		kind 			"StaticLib"
		language 		"C++"
		cppdialect 		"C++17"
		systemversion 	"latest"
		location 		"Dependencies/projectfiles/ImGui"

		-- Locations
		targetdir 	("Dependencies/Build/bin/"     .. outputdir)
		objdir 	 	("Dependencies/Build/bin-int/" .. outputdir)

		-- Files
		files
		{
			"Dependencies/imgui/imconfig.h",
			"Dependencies/imgui/imgui.h",
			"Dependencies/imgui/imgui.cpp",
			"Dependencies/imgui/imgui_draw.cpp",
			"Dependencies/imgui/imgui_demo.cpp",
			"Dependencies/imgui/imgui_internal.h",
			"Dependencies/imgui/imgui_tables.cpp",
			"Dependencies/imgui/imgui_widgets.cpp",
			"Dependencies/imgui/imstb_rectpack.h",
			"Dependencies/imgui/imstb_textedit.h",
			"Dependencies/imgui/imstb_truetype.h",
		}
		
		-- Configurations
		filter "configurations:Debug or Release"
			symbols 	"on"
			runtime 	"Release"
			optimize 	"Full"
		
		filter "configurations:Production"
			symbols 	"off"
			runtime 	"Release"
			optimize 	"Full"
	
	-- tinyobjloader Project
	project "tinyobjloader"
		kind 			"StaticLib"
		language 		"C++"
		cppdialect 		"C++17"
		systemversion 	"latest"
		location 		"Dependencies/projectfiles/tinyobjloader"

		-- Locations
		targetdir ("Dependencies/Build/bin/tinyobjloader/"     .. outputdir)
		objdir    ("Dependencies/Build/bin-int/tinyobjloader/" .. outputdir)

		-- Files
		files 
		{
			"Dependencies/tinyobjloader/tiny_obj_loader.h",
			"Dependencies/tinyobjloader/tiny_obj_loader.cc",
		}

		-- Configurations
		filter "configurations:Debug or Release"
			symbols 	"on"
			runtime 	"Release"
			optimize 	"Full"
		
		filter "configurations:Production"
			symbols 	"off"
			runtime 	"Release"
			optimize 	"Full"	
				
	
	-- OpenFBX Project
	project "OpenFBX"
		kind 			"StaticLib"
		language 		"C++"
		cppdialect 		"C++17"
		systemversion 	"latest"
		location 		"Dependencies/projectfiles/OpenFBX"
		
		-- Locations
		targetdir ("Dependencies/Build/bin/OpenFBX/"     .. outputdir)
		objdir    ("Dependencies/Build/bin-int/OpenFBX/" .. outputdir)
				
		-- Files
		files 
		{
			"Dependencies/OpenFBX/src/ofbx.h",
			"Dependencies/OpenFBX/src/ofbx.cpp",
			"Dependencies/OpenFBX/src/miniz.h",
			"Dependencies/OpenFBX/src/miniz.c",
		}

		-- Configurations 
		filter "configurations:Debug or Release"
			symbols 	"on"
			runtime 	"Release"
			optimize 	"Full"
		
		filter "configurations:Production"
			symbols 	"off"
			runtime 	"Release"
			optimize 	"Full"
		
group ""

-- Engine Project
project "DXR-Engine"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	location 		"DXR-Engine"
	kind 			"SharedLib"
	characterset 	"Ascii"
	
	-- Pre-Compiled Headers
	pchheader "PreCompiled.h"
	pchsource "%{prj.name}/PreCompiled.cpp"

	-- All targets except the dependencies
	targetdir 	("Build/bin/"     .. outputdir)
	objdir 		("Build/bin-int/" .. outputdir)	

	-- Includes
	includedirs
	{
		"%{prj.name}",
		"%{prj.name}/Include",
	}

	forceincludes  
	{ 
		"PreCompiled.h"
	}

	-- Defines
	defines
	{
		"CORE_API_EXPORT=(1)"
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
		"%{prj.name}/**.hlsli",	
	}
	
	excludes 
	{
		"%{prj.name}/Main/**",
		"%{prj.name}/Math/Tests/**",
	}

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
		"OpenFBX",
	}
	
	-- Remove non-macos and add macos-specific files
	filter "system:macosx"
		files 
		{ 
			"%{prj.name}/**.mm",
		}

		excludes 
		{
			"%{prj.name}/D3D12/**",
			"%{prj.name}/Core/Windows/**",
			"%{prj.name}/Core/Application/Windows/**",
			"%{prj.name}/Core/Threading/Windows/**",
			"%{prj.name}/Core/Time/Windows/**",
		}

	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{prj.name}/**.natvis",
		}        

	filter "system:macosx"
		links
		{
			-- Native
			"Cocoa.framework",
			"AppKit.framework",
			"MetalKit.framework",
		}

-- Project
include "Sandbox"