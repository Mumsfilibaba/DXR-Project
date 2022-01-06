include '../BuildScripts/Scripts/enginebuild.lua'

local SandboxProject = CreateProject( 'Sandbox' )
SandboxProject:AddModuleDependencies
{
	'Core',
	'CoreApplication',
	'Interface',
	'RHI',
	'Engine',
	'Renderer',
}

SandboxProject:AddDynamicModuleDependencies
{
	'InterfaceRenderer',
	'NullRHI',
}

if BuildWithXcode() then
	printf('BuildWithXcode')

    SandboxProject:AddFrameWorks 
    {
        'Cocoa',
        'AppKit',
        'MetalKit'
    }
else
	SandboxProject:AddDynamicModuleDependencies('D3D12RHI')
end

SandboxProject:Generate()

--[[
--TODO: Auto-Generate this file when creating a new project (In editor)
projectname = "Sandbox"

project ( projectname )
	location ( "%{wks.location}/" .. projectname )

	-- Build type 
	filter "not options:monolithic"
		kind "SharedLib"
	filter {}

	filter "options:monolithic"
		kind "StaticLib"
	filter {}

	-- All targets except the dependencies
	targetdir ( "%{wks.location}/Build/bin/"     .. outputdir )
	objdir    ( "%{wks.location}/Build/bin-int/" .. outputdir )

	sysincludedirs
	{
		"%{wks.location}/Runtime",	
		"%{wks.location}/Dependencies/imgui",
	}

	defines
	{
		"PROJECT_NAME=" .. "\"" .. projectname .. "\"",
		"SANDBOX_EXPORT=(1)",
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

	-- We do not want to compile HLSL files so exclude them from project
	excludes 
	{	
		"**.hlsl",
		"**.hlsli",
	}

	links
	{
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
		}
	filter {}

	-- Specific linking for windows
	filter { "system:windows", "options:monolithic" }
		links
		{
			"D3D12RHI",
		}
	filter {}

	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{wks.location}/%{prj.name}/**.natvis",
		}
	filter {}
	
	-- TODO: Check why this is necessary
	filter "system:macosx"
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
	
-- Sandbox Project
project (projectname .. "Launcher")
	kind 	 "WindowedApp"
	location ( "%{wks.location}/" .. projectname )

	-- All targets except the dependencies
	targetdir ("%{wks.location}/Build/bin/"     .. outputdir)
	objdir    ("%{wks.location}/Build/bin-int/" .. outputdir)

	sysincludedirs
	{
		"%{wks.location}/Runtime",
		"%{wks.location}/Dependencies/imgui",
	}

	defines
	{
		"PROJECT_NAME=" .. "\"" .. projectname .. "\"",
		"PROJECT_LOCATION=" .. "\"" .. findWorkspaceDir() .. "/" .. projectname .. "\"",
	}
	
	filter "not options:monolithic"
		links
		{
			"Core",
			"CoreApplication",
			"Interface",
			"RHI",
			"Engine",
			"Renderer",
		}
	filter{}

	filter "options:monolithic"
		links
		{
			"Core",
			"CoreApplication",
			"Interface",
			"RHI",
			"Engine",
			"Renderer",
			"InterfaceRenderer",
			"NullRHI",
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
			"/INCLUDE:LinkModule_Core",
			"/INCLUDE:LinkModule_CoreApplication",
			"/INCLUDE:LinkModule_Interface",
			"/INCLUDE:LinkModule_RHI",
			"/INCLUDE:LinkModule_Engine",
			"/INCLUDE:LinkModule_Renderer",
			"/INCLUDE:LinkModule_InterfaceRenderer",
			"/INCLUDE:LinkModule_NullRHI",
			"/INCLUDE:LinkModule_D3D12RHI",
			"/INCLUDE:LinkModule_Sandbox",
		}
	filter {}

	-- Specific linking for windows
	filter { "system:macosx", "options:monolithic" }
		-- Force references to module function in order to include it in the program
		linkoptions 
		{
			"-Wl, -force-load, libCore.a",
			"-Wl, -force-load, libCoreApplication.a",
			"-Wl, -force-load, libInterface.a",
			"-Wl, -force-load, libRHI.a",
			"-Wl, -force-load, libEngine.a",
			"-Wl, -force-load, libRenderer.a",
			"-Wl, -force-load, libInterfaceRenderer.a",
			"-Wl, -force-load, libNullRHI.a",
			"-Wl, -force-load, libSandbox.a",
		}
	filter {}

	-- Include EngineLoop | TODO: Make lib?
	files
	{
		"%{wks.location}/Runtime/Main/EngineLoop.cpp",
		"%{wks.location}/Runtime/Main/EngineMain.inl",	
	}

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

	-- On macOS compile all cpp files to objective-C++ to avoid pre-processor check
	filter { "system:macosx", "files:**.cpp" }
		compileas "Objective-C++"
	filter {}
	
	-- In visual studio show natvis files
	filter "action:vs*"
		vpaths { ["Natvis"] = "**.natvis" }
		
		files 
		{
			"%{wks.location}/%{prj.name}/**.natvis",
		}
	filter {}
	
	-- TODO: Check why this is necessary
	filter "system:macosx"
		links
		{
			-- Native
			"Cocoa.framework",
			"AppKit.framework",
			"MetalKit.framework",
		}
		
		removefiles
		{
			"%{wks.location}/**/Windows/**"
		}
	filter {}

	-- TODO: See if there is a better way to handle dependencies
	filter { "system:macosx", "not options:monolithic" }
		dependson
		{
			( projectname ),
			"InterfaceRenderer",
			"NullRHI",
		}
	filter {}

]]--