workspace "DXR-Engine-Tests"
    startproject 	"Container-Tests"
    architecture 	"x64"
    warnings 		"extra"
	language 		"C++"
	cppdialect 		"C++17"
	systemversion 	"latest"
	characterset 	"Ascii"
	flags { "MultiProcessorCompile" }
    
	-- Set output dir
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}-%{cfg.platform}"

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
    }

    filter "configurations:Debug"
        symbols "on"
        runtime "Debug"
        defines
        {
            "_DEBUG",
            "DEBUG_BUILD",
        }
    filter "configurations:Release"
        symbols "on"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "RELEASE_BUILD",
        }
    filter {}

    -- IDE options
	filter "action:vs*"
        defines
        {
            "IDE_VISUAL_STUDIO",
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        }
	filter "action:xcode"
		defines
		{
            "IDE_XCODE",
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
    filter {}

    -- Container Tests
    project "Containers-Tests"
        location 	"Containers-Tests"
        kind 		"ConsoleApp"

        -- Targets
		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	
    
        -- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
			
			"../DXR-Engine/Core/Memory/Memory.cpp",
        }
			
		-- In visual studio show natvis files
		filter "action:vs*"
			vpaths { ["Natvis"] = "**.natvis" }
			
			files 
			{
				"%{prj.name}/**.natvis",
				"../DXR-Engine/Core/Containers/**.natvis",
				"../DXR-Engine/Core/Templates/**.natvis",
			}
		filter {}

        -- Includes
		includedirs
		{
			"%{prj.name}",
        }
		
		-- System includes
		sysincludedirs
		{
			"../DXR-Engine/",
		}
		
    project "*"
	
	-- Math Tests
    project "MathLib-Tests"
        location 	"MathLib-Tests"
        kind 		"ConsoleApp"

        -- Targets
		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	
    
        -- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
        }
			
		-- In visual studio show natvis files
		filter "action:vs*"
			vpaths { ["Natvis"] = "**.natvis" }
			
			files 
			{
				"%{prj.name}/**.natvis",
			}
		filter {}

        -- Includes
		includedirs
		{
			"%{prj.name}",
        }
		
		-- System includes
		sysincludedirs
		{
			"../DXR-Engine/",
		}
		
    project "*"
	
	-- Templates Tests
    project "Templates-Tests"
        location 	"Templates-Tests"
        kind 		"ConsoleApp"

        -- Targets
		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	
    
        -- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
        }
			
		-- In visual studio show natvis files
		filter "action:vs*"
			vpaths { ["Natvis"] = "**.natvis" }
			
			files 
			{
				"%{prj.name}/**.natvis",
			}
		filter {}

        -- Includes
		includedirs
		{
			"%{prj.name}",
        }
		
		-- System includes
		sysincludedirs
		{
			"../DXR-Engine/",
		}
		
    project "*"
	