project "Core"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	location 		"%{wks.location}/Runtime/Core"
	characterset 	"Ascii"

	-- Build type 
	filter "not options:monolithic"
		kind "SharedLib"
	filter {}

	filter "options:monolithic"
		kind "StaticLib"
	filter {}

	-- Pre-Compiled Headers
	pchheader "PreCompiled.h"
	pchsource "PreCompiled.cpp"

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
		"PreCompiled.h"
	}

	-- Defines
	defines
	{
		"CORE_API_EXPORT=(1)"
	}

	compileas "Objective-C++"

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
	}

	links 
	{ 
		"ImGui",
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
		
		-- Remove non-macos and add macos-specific files
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