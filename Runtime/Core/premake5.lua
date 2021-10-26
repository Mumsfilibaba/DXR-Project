project "Core"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	location 		"%{wks.location}/Runtime/Core"
	kind 			"SharedLib"
	characterset 	"Ascii"

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

	excludes 
	{
		"**/Main/**",
	}

	-- We do not want to compile HLSL files so exclude them from project
	excludes 
	{	
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