modulename = "Renderer"

project ( modulename )
	location ( "%{wks.location}/Runtime/" .. modulename )

	-- Build type 
	filter "not options:monolithic"
		kind "SharedLib"
	filter {}

	filter "options:monolithic"
		kind "StaticLib"
	filter {}

	--TODO: Pre-Compiled Headers

	-- All targets except the dependencies
	targetdir 	("%{wks.location}/Build/bin/"     .. outputdir)
	objdir 		("%{wks.location}/Build/bin-int/" .. outputdir)	

	forceincludes  
	{ 
		-- TODO: "PreCompiled.h"
	}

	-- Defines
	filter "not options:monolithic"
		defines
		{
			"RENDERER_API_EXPORT=(1)"
		}
	filter {}

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
		"RHI",
		"Engine",
	}
	
	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{wks.location}/Runtime/%{prj.name}/**.natvis",
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

	-- Remove non-windows files
	filter "system:windows"
		removefiles
		{
			"%{wks.location}/**/Mac/**"
		}
	filter {}