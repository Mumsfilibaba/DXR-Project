project "InterfaceRenderer"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	location 		"%{wks.location}/Runtime/InterfaceRenderer"
	kind 			"SharedLib"
	characterset 	"Ascii"

	--TODO: Pre-Compiled Headers

	-- All targets except the dependencies
	targetdir 	("%{wks.location}/Build/bin/"     .. outputdir)
	objdir 		("%{wks.location}/Build/bin-int/" .. outputdir)	

	-- Includes
	includedirs
	{
		"%{wks.location}/Runtime",
	}

	forceincludes  
	{ 
		-- TODO: "PreCompiled.h"
	}

	-- Defines
	defines
	{
		"INTERFACE_RENDERER_API_EXPORT=(1)"
	}

	-- Files to include
	files 
	{ 
		"%{wks.location}/Runtime/%{prj.name}/**.h",
		"%{wks.location}/Runtime/%{prj.name}/**.hpp",
		"%{wks.location}/Runtime/%{prj.name}/**.inl",
		"%{wks.location}/Runtime/%{prj.name}/**.c",
		"%{wks.location}/Runtime/%{prj.name}/**.cpp",
		"%{wks.location}/Runtime/%{prj.name}/**.hlsl",
		"%{wks.location}/Runtime/%{prj.name}/**.hlsli",	
	}

	-- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
	filter { "system:macosx", "files:**.cpp" }
		compileas "Objective-C++"
	filter {}

	excludes 
	{	
		"**/Main/**",
		-- We do not want to compile HLSL files so exclude them from project
		"**.hlsl",
		"**.hlsli",
	}

	sysincludedirs
	{
		"%{wks.location}/Dependencies/imgui",
		"%{wks.location}/Dependencies/stb_image",
		"%{wks.location}/Dependencies/tinyobjloader",
		"%{wks.location}/Dependencies/OpenFBX/src",
	}

	links 
	{ 
		"ImGui",
		"Core",
		"CoreApplication",
		"Interface",
		"Engine",
		"RHI",
	}
	
	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{prj.name}/**.natvis",
		}
	filter {}
	
	filter "system:macosx"
		files 
		{ 
			"%{wks.location}/Runtime/%{prj.name}/**.mm",
		}

		removefiles
		{
			"%{wks.location}/**/Windows/**"
		}

		links
		{
			-- Native
			"Cocoa.framework",
			"AppKit.framework",
			"MetalKit.framework",
		}
	filter {}