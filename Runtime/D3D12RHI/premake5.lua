project "D3D12RHI"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	location 		"%{wks.location}/Runtime/D3D12RHI"
	kind 			"SharedLib"
	characterset 	"Ascii"

	-- TODO: Add Precompiled for D3D12

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
		"D3D12RHI_API_EXPORT=(1)"
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
		-- We do not want to compile HLSL files so exclude them from project
		"**.hlsl",
		"**.hlsli",
	}

	links 
	{ 
		"Core",
		"CoreApplication",
		"RHI",
	}

	-- In visual studio show natvis files
	filter { "system:windows", "action:vs*" }
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{prj.name}/**.natvis",
		}
	filter {}

	filter "system:macosx"
		removefiles
		{
			"%{wks.location}/**/D3D12/**",
			"%{wks.location}/**/Windows/**"
		}
	filter {}
