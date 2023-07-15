include "build_module.lua"
include "build_target.lua"

-- Generate a workspace from an array of target-rules
function FWorkspaceRules(WorkspaceName)
    
    -- Must have a valid Workspace Name
    if WorkspaceName == nil then
        return nil
    end

    -- Init this object
    local self = 
    {
        -- @brief - Name of the workspace being generated
        Name = WorkspaceName,
        
        -- @brief - List  of targets for this workspace
        TargetRules = { },

        -- @brief - Name of the target of the workspace
        TargetName = "",
        
        -- @brief - Engine folder path
        EnginePath = GetEnginePath(),

        -- @brief - Defines
        Defines = { },

        -- @brief - Projects that should have projects generated
        ProjectRules = { },

        -- @brief - Name of the project that should be set to startup project
        StartProjectName = "",
    }

    -- @brief - Output path for dependencies (ImGui etc.)
    function self.GetOutputPath()
        return "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"
    end

    -- @brief - Retreive the path of the engine
    function self.GetEnginePath()
        return self.EnginePath
    end

    -- @brief - Retreive the current Target Name
    function self.GetCurrentTargetName()
        return self.TargetName
    end

    -- @brief - Retreive the path of the engine 'Runtime' folder
    function self.GetRuntimeFolderPath()
        return self.GetEnginePath() .. "/Runtime"
    end

    -- @brief - Retreive the path of the engine 'Runtime' folder
    function self.GetBuildFolderPath()
        return self.GetEnginePath() .. "/Build"
    end

    -- @brief - Retreive the path of the engine 'Solutions' folder
    function self.GetSolutionsFolderPath()
        return self.GetEnginePath() .. "/Solutions"
    end

    -- @brief - Retrieve the path to the dependencies folder containing external dependecy projects
    function self.GetExternalDependenciesFolderPath()
        return self.GetEnginePath() .. "/Dependencies"
    end

    -- @brief - Create a path relative to dependency folder
    function self.CreateExternalDependencyPath(Path)
        return GetExternalDependenciesFolderPath() .. "/" .. Path
    end

    -- @brief - Retreive a target added to the workspace
    function self.GetTarget(TargetName)
        return self.TargetRules[TargetName]
    end

    -- @brief - Check if a target already exists
    function self.IsTarget(TargetName)
        return self.GetTarget(TargetName) ~= nil
    end

    -- @brief - Helper function for adding a target
    function self.AddTarget(InTarget)
        table.insert(self.TargetRules, InTarget)
    end

    -- @brief - Helper function for adding defines
    function self.AddDefines(InDefine)
        AddUniqueElements(InDefine, self.Defines)
    end

    -- @brief - Helper function for adding a rule
    function self.AddRule(InRule)
        table.insert(self.ProjectRules, InRule)
    end

    -- TODO: Better way of handling these dependencies
    -- Inject dependencies projects into the workspace
    function self.GenerateDependencyProjects()
        local SolutionLocation      = self.GetSolutionsFolderPath()
        local ExternalDependecyPath = self.GetExternalDependenciesFolderPath()
        group "Dependencies"
            LogInfo("\n--- External Dependencies ---")
            
            -- Imgui
            project "ImGui"
                LogInfo("    Generating dependecy ImGui")

                kind("StaticLib")
                warnings("Off")
                intrinsics("On")
                editandcontinue("Off")
                language("C++")
                cppdialect("C++20")
                systemversion("latest")
                architecture("x86_64")
                exceptionhandling("Off")
                rtti("Off")
                floatingpoint("Fast")
                vectorextensions("SSE2")
                characterset("Ascii")
                flags(
                { 
                    "MultiProcessorCompile",
                    "NoIncrementalLink",
                })

                location(SolutionLocation .. "/Dependencies/ImGui")

                -- Locations
                targetdir(ExternalDependecyPath .. "/Build/bin/ImGui/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/ImGui/" .. self.GetOutputPath())

                -- Files
                files
                {
                    (ExternalDependecyPath .. "/imgui/imconfig.h"),
                    (ExternalDependecyPath .. "/imgui/imgui.h"),
                    (ExternalDependecyPath .. "/imgui/imgui.cpp"),
                    (ExternalDependecyPath .. "/imgui/imgui_demo.cpp"),
                    (ExternalDependecyPath .. "/imgui/imgui_draw.cpp"),
                    (ExternalDependecyPath .. "/imgui/imgui_internal.h"),
                    (ExternalDependecyPath .. "/imgui/imgui_tables.cpp"),
                    (ExternalDependecyPath .. "/imgui/imgui_widgets.cpp"),
                    (ExternalDependecyPath .. "/imgui/imstb_rectpack.h"),
                    (ExternalDependecyPath .. "/imgui/imstb_textedit.h"),
                    (ExternalDependecyPath .. "/imgui/imstb_truetype.h"),
                }
                
                -- Configurations
                filter "configurations:Debug or Release"
                    symbols("on")
                    runtime("Release")
                    optimize("Full")
                filter {}
                
                filter "configurations:Production"
                    symbols("off")
                    runtime("Release")
                    optimize("Full")
                filter {}
            
            -- tinyobjloader Project
            project "tinyobjloader"
                LogInfo("    Generating dependecy tinyobjloader")

                kind("StaticLib")
                warnings("Off")
                intrinsics("On")
                editandcontinue("Off")
                language("C++")
                cppdialect("C++20")
                systemversion("latest")
                architecture("x86_64")
                exceptionhandling("Off")
                rtti("Off")
                floatingpoint("Fast")
                vectorextensions("SSE2")
                characterset("Ascii")
                flags(
                { 
                    "MultiProcessorCompile",
                    "NoIncrementalLink",
                })

                location(SolutionLocation .. "/Dependencies/tinyobjloader")

                -- Locations
                targetdir(ExternalDependecyPath .. "/Build/bin/tinyobjloader/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/tinyobjloader/" .. self.GetOutputPath())

                -- Files
                files 
                {
                    (ExternalDependecyPath .. "/tinyobjloader/tiny_obj_loader.h"),
                    (ExternalDependecyPath .. "/tinyobjloader/tiny_obj_loader.cc"),
                }

                -- Configurations
                filter "configurations:Debug or Release"
                    symbols("on")
                    runtime("Release")
                    optimize("Full")
                filter {}

                filter "configurations:Production"
                    symbols("off")
                    runtime("Release")
                    optimize("Full")    
                filter {}
            
            -- OpenFBX Project
            project "OpenFBX"
                LogInfo("    Generating dependecy OpenFBX")

                kind("StaticLib")
                warnings("Off")
                intrinsics("On")
                editandcontinue("Off")
                language("C++")
                cppdialect("C++20")
                systemversion("latest")
                architecture("x86_64")
                exceptionhandling("Off")
                rtti("Off")
                floatingpoint("Fast")
                vectorextensions("SSE2")
                characterset("Ascii")
                flags(
                { 
                    "MultiProcessorCompile",
                    "NoIncrementalLink",
                })
                
                location(SolutionLocation .. "/Dependencies/OpenFBX")
            
                -- Locations
                targetdir(ExternalDependecyPath .. "/Build/bin/OpenFBX/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/OpenFBX/" .. self.GetOutputPath())

                -- Files
                files 
                {
                    (ExternalDependecyPath .. "/OpenFBX/src/ofbx.h"),
                    (ExternalDependecyPath .. "/OpenFBX/src/ofbx.cpp"),
                    (ExternalDependecyPath .. "/OpenFBX/src/miniz.h"),
                    (ExternalDependecyPath .. "/OpenFBX/src/miniz.c"),
                }

                -- Configurations 
                filter "configurations:Debug or Release"
                    symbols("on")
                    runtime("Release")
                    optimize("Full")
                filter {}
                
                filter "configurations:Production"
                    symbols("off")
                    runtime("Release")
                    optimize("Full")
                filter {}

            -- SPIRV-Cross Project
            project "SPIRV-Cross"
                LogInfo("    Generating dependecy SPIRV-Cross")

                kind("StaticLib")
                warnings("Off")
                intrinsics("On")
                editandcontinue("Off")
                language("C++")
                cppdialect("C++20")
                systemversion("latest")
                architecture("x86_64")
                exceptionhandling("On")
                rtti("Off")
                floatingpoint("Fast")
                vectorextensions("SSE2")
                characterset("Ascii")
                flags(
                { 
                    "MultiProcessorCompile",
                    "NoIncrementalLink",
                })
                
                location(SolutionLocation .. "/Dependencies/SPIRV-Cross")
            
                -- Locations
                targetdir(ExternalDependecyPath .. "/Build/bin/SPIRV-Cross/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/SPIRV-Cross/" .. self.GetOutputPath())

                -- Files
                files 
                {
                    (ExternalDependecyPath .. "/SPIRV-Cross/GLSL.std.450.h"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv.h"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_c.h"),

                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cfg.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_common.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cpp.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_containers.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_error_handling.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_parsed_ir.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_util.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_glsl.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_hlsl.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_msl.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_parser.hpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_reflect.hpp"),

                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cfg.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cpp.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_c.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_parsed_ir.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_cross_util.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_glsl.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_hlsl.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_msl.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_parser.cpp"),
                    (ExternalDependecyPath .. "/SPIRV-Cross/spirv_reflect.cpp"),
                }

                -- Defines 
                defines
                {
                    "SPIRV_CROSS_C_API_MSL=(1)",
                    "SPIRV_CROSS_C_API_HLSL=(1)",
                    "SPIRV_CROSS_C_API_GLSL=(1)",
                }

                -- Configurations 
                filter "configurations:Debug or Release"
                    symbols("on")
                    runtime("Release")
                    optimize("Full")
                filter {}
                
                filter "configurations:Production"
                    symbols("off")
                    runtime("Release")
                    optimize("Full")
                filter {}
        group ""
    end

    -- Generate the actual solution files
    function self.GenerateSolutionFiles()
        LogInfo("\n--- Generating Solution Files for Workspace \'%s\' ---", self.Name)

        -- Set the name of the workspace
        workspace(self.Name)

        -- Set location of the generated solution file
        local SolutionLocation = self.GetSolutionsFolderPath()
        location(SolutionLocation)

        LogInfo("    Generated solution location \'%s\'", SolutionLocation)

        -- Platforms
        platforms
        {
            "x64",
        }

        -- Configurations
        configurations
        {
            "Debug",
            "Release",
            "Production",
        }

        -- Includes
        local RuntimeFolderPath = self.GetRuntimeFolderPath()
        includedirs
        {
            RuntimeFolderPath,
        }

        -- Workspace defines
        LogInfo("\n--- Workspace Defines (Num Defines=%d) ---", #self.Defines)
        if #self.Defines > 0 then
            PrintTable("    Using Define \'%s\'", self.Defines)
        else
            LogInfo("")
        end

        defines(self.Defines)

        -- Add settings based on configuration
        filter "configurations:Debug"
            symbols("on")
            runtime("Debug")
            optimize("Off")
            architecture("x86_64")
            defines
            {
                "_DEBUG",
                "DEBUG_BUILD=(1)",
            }
        filter {}

        filter "configurations:Release"
            symbols("on")
            runtime("Release")
            optimize("Full")
            architecture("x86_64")
            defines
            {
                "NDEBUG",
                "RELEASE_BUILD=(1)",
            }
        filter {}

        filter "configurations:Production"
            symbols("off")
            runtime("Release")
            optimize("Full")
            architecture("x86_64")
            defines
            {
                "NDEBUG",
                "PRODUCTION_BUILD=(1)",
            }
        filter {}

        -- Architecture defines
        filter "architecture:x86"
            defines
            {
                "ARCHITECTURE_X86=(1)",
            }
        filter {}

        filter "architecture:x86_x64"
            defines
            {
                "PLATFORM_ARCHITECTURE_X86_X64=(1)",
            }
        filter {}

        filter "architecture:ARM"
            defines
            {
                "PLATFORM_ARCHITECTURE_ARM=(1)",
            }
        filter {}

        -- Startup project name
        LogInfo("    StartProject = \'%s\'", self.StartProjectName)
        startproject(self.StartProjectName)

        -- Generate projects for all dependencies
        self.GenerateDependencyProjects()

        -- Generate project files for all the rules that has been added
        LogInfo("\n--- Generating module and target project files ---")
        for Index = 1, #self.ProjectRules do
            local CurrentRule = self.ProjectRules[Index]
            CurrentRule.GenerateProject()
        end
    end

    -- Generate workspace
    function self.Generate()
        LogInfo("\n--- Generating Workspace \'%s\' ---", self.Name)
        LogInfo("OutputPath = \'%s\'", self.GetOutputPath())

        if self.TargetRules == nil then
            LogError("TargetRules cannot be nil")
            return
        end

        if #self.TargetRules < 1 then
            LogError("Workspace must contain atleast one buildrule (Current=%d)", #self.TargetRules)
            return
        end

        -- Define the workspace location
        local EngineLocation = "ENGINE_LOCATION=" .. "\"" .. self.GetEnginePath() .. "\""
        self.AddDefines({ EngineLocation })
        
        LogInfo("    Engine Path =\'%s\'", self.GetEnginePath())
        LogInfo("    RuntimeFolderPath = \'%s\'", self.GetRuntimeFolderPath())
        
        -- Check if the commandline overrides monolithic builds
        if IsMonolithic() then
            self.AddDefines({ "MONOLITHIC_BUILD=(1)" })
        end

        -- IDE Defines
        if BuildWithVisualStudio() then 
            self.AddDefines({ "IDE_VISUAL_STUDIO" })
            self.AddDefines({ "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" })
            self.AddDefines({ "_CRT_SECURE_NO_WARNINGS" })
        end

        -- OS Defines
        if IsPlatformWindows() then
            self.AddDefines({ "PLATFORM_WINDOWS=(1)" })
        end
        if IsPlatformMac() then
            self.AddDefines({ "PLATFORM_MACOS=(1)" })
        end

        -- Setup startup project
        local StartProjectTarget = self.TargetRules[1]
        if (StartProjectTarget.TargetType == ETargetType.Client) and (not StartProjectTarget.bIsMonolithic) then
            self.StartProjectName = StartProjectTarget.Name .. "Standalone"
        else
            self.StartProjectName = StartProjectTarget.Name
        end
        
        -- Generate projects from targets
        LogInfo("\n--- Generating Targets (NumTargets=%d) ---", #self.TargetRules)
        for Index = 1, #self.TargetRules do
            local CurrentTarget = self.TargetRules[Index]
            self.TargetName = CurrentTarget.Name
            CurrentTarget.Workspace = self
            CurrentTarget.Generate()
        end

        -- Generate the actual solution files
        self.GenerateSolutionFiles()

        LogInfo("\n--- Finished generating workspace ---")
    end

    LogHighlight("Creating Workspace \'%s\'", WorkspaceName)
    return self
end