function findProjectDir()
	local projectName = "/%{prj.name}"
	return os.getcwd() .. projectName 
end

function findWorkspaceDir()
	local workspaceName = "%{wks.location}"
	--return path.getabsolute(workspaceName, os.getcwd()) I must be missing something because it feels like this should work
	return os.getcwd()
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
	editandcontinue   "Off"
	intrinsics        "On"

	flags 
	{ 
		"MultiProcessorCompile",
		"NoIncrementalLink",
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

	defines
	{
		"WORKSPACE_LOCATION=" .. "\"" .. findWorkspaceDir().. "\"",
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

-- Engine Projects
include "Runtime/Core"
include "Runtime/RHI"
include "Runtime/D3D12RHI"
include "Runtime/NullRHI"
include "Runtime/Engine"
include "Runtime/Renderer"

-- Editor
include "Editor"

-- Project
include "Sandbox"