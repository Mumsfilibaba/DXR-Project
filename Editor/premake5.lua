projectname = "Editor"

project ( projectname )
	location ( "%{wks.location}/" .. projectname )
	kind 	 "WindowedApp"

	--TODO: Pre-Compiled Headers

	-- All targets except the dependencies
	targetdir 	( "%{wks.location}/Build/bin/"     .. outputdir )
	objdir 		( "%{wks.location}/Build/bin-int/" .. outputdir )	

	-- Includes
	includedirs
	{
		"%{wks.location}/" .. projectname,
	}

	sysincludedirs
	{
		"%{wks.location}/Dependencies/imgui",
	}
	
	forceincludes  
	{ 
		-- TODO: "PreCompiled.h"
	}
	
	-- Defines
	defines
	{
		"PROJECT_NAME=" .. "\"" .. projectname .. "\"",
		"PROJECT_EDITOR=(1)",
	}

	-- Files to include
	files 
	{ 
		"%{wks.location}/%{prj.name}/**.h",
		"%{wks.location}/%{prj.name}/**.hpp",
		"%{wks.location}/%{prj.name}/**.inl",
		"%{wks.location}/%{prj.name}/**.c",
		"%{wks.location}/%{prj.name}/**.cpp",
		"%{wks.location}/%{prj.name}/**.hlsl",
		"%{wks.location}/%{prj.name}/**.hlsli",	
	}

	-- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
	filter { "system:macosx", "files:**.cpp" }
		compileas "Objective-C++"
	filter {}

	-- Include EngineLoop | TODO: Make lib?
	files
	{
		"%{wks.location}/Runtime/Main/EngineLoop.h",	
		"%{wks.location}/Runtime/Main/EngineLoop.cpp",	
	}

	-- We do not want to compile HLSL files so exclude them from project
	excludes 
	{	
		"**.hlsl",
		"**.hlsli",
	}

	links
	{ 
		"ImGui",
		"Core",
		"CoreApplication",
		"Interface",
		"RHI",
		"Engine",
		"Renderer",
	}

	filter "options:monolithic"
		links
		{
			"NullRHI",
			"InterfaceRenderer",
			"Sandbox",
		}
	filter {}

	-- Specific linking for windows
	filter { "system:windows", "options:monolithic" }
		links
		{
			"D3D12RHI",
		}

		-- Force references to module function in order to include it in the program
		linkoptions 
		{
			"/INCLUDE:LinkModule_InterfaceRenderer",
			"/INCLUDE:LinkModule_NullRHI",
			"/INCLUDE:LinkModule_D3D12RHI",
			"/INCLUDE:LinkModule_Sandbox",
		}
	filter {}

	-- Include EntryPoint
	filter "system:windows"
		files
		{
			"%{wks.location}/Runtime/Main/Windows/WindowsMain.cpp",	
		}
	filter {}

	filter "system:macosx"
		files
		{
			"%{wks.location}/Runtime/Main/Mac/MacMain.cpp",	
		}
	filter {}
	
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