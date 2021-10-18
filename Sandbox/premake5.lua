--TODO: Auto-Generate this file when creating a new project (In editor)
project "Sandbox"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	kind 			"SharedLib"
	location        "%{wks.location}/Sandbox"
	characterset 	"Ascii"

	-- All targets except the dependencies
	targetdir ("%{wks.location}/Build/bin/"     .. outputdir)
	objdir    ("%{wks.location}/Build/bin-int/" .. outputdir)

	sysincludedirs
	{
		"%{wks.location}/DXR-Engine",	
		"%{wks.location}/Dependencies/imgui",
	}

	defines
	{
		"PROJECT_LOCATION=" .. "\"" .. findProjectDir().. "\"",
		"SANDBOX_EXPORT=(1)"
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

	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{wks.location}/%{prj.name}/**.natvis",
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
	
-- Sandbox Project
project "SandboxLauncher"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	kind 			"WindowedApp"
	location        "%{wks.location}/Sandbox"
	characterset 	"Ascii"

	-- All targets except the dependencies
	targetdir ("%{wks.location}/Build/bin/"     .. outputdir)
	objdir    ("%{wks.location}/Build/bin-int/" .. outputdir)

	sysincludedirs
	{
		"%{wks.location}/DXR-Engine"
	}

	defines
	{
		"PROJECT_LOCATION=" .. "\"" .. findProjectDir().. "\""
	}
	
	links
	{ 
		"DXR-Engine"
	}

	-- Include EntryPoint
	filter "system:windows"
		files
		{
			"%{wks.location}/DXR-Engine/Main/Windows/WindowsMain.cpp",	
		}
	filter "system:macosx"
		files
		{
			"%{wks.location}/DXR-Engine/Main/Mac/MacMain.cpp",	
		}
	
	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{wks.location}/%{prj.name}/**.natvis",
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
	