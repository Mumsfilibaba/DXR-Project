projectname = "Editor"

project ( projectname )
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	location 		( "%{wks.location}/" .. projectname )
	kind 			"WindowedApp"
	characterset 	"Ascii"

	--TODO: Pre-Compiled Headers

	-- All targets except the dependencies
	targetdir 	( "%{wks.location}/Build/bin/"     .. outputdir )
	objdir 		( "%{wks.location}/Build/bin-int/" .. outputdir )	
	
	-- Includes
	includedirs
	{
		"%{wks.location}/" .. projectname,
		"%{wks.location}/Runtime",
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
		"PROJECT_LOCATION=" .. "\"" .. findProjectDir().. "\"",
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

	-- Include EntryPoint
	filter "system:windows"
		files
		{
			"%{wks.location}/Runtime/Main/Windows/WindowsMain.cpp",	
		}
	filter "system:macosx"
		files
		{
			"%{wks.location}/Runtime/Main/Mac/MacMain.cpp",	
		}

	-- Remove non-macos and add macos-specific files
	filter "system:macosx"
		files 
		{ 
			"%{wks.location}/Runtime/%{prj.name}/**.mm",
		}

		excludes 
		{
			"**/D3D12/**",
			"**/Windows/**",
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