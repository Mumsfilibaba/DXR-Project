include "build_module.lua"
include "build_target.lua"

-- Function to deduce software version
function Glslang_DeduceSoftwareVersion(Directory)
    -- Path to the CHANGES.md file
    local ChangesFile = Directory .. "/CHANGES.md"

    -- Create a pattern to match the version and date line in CHANGES.md
    local Pattern = "^#*%s*(%d+)%.(%d+)%.(%d+)%s*(-?[%w]*)%s*(%d%d%d%d%-%d%d%-%d%d)%s*"

    -- Read the file line by line
    for Line in io.lines(ChangesFile) do
        local Major, Minor, Patch, Flavor, Date = Line:match(Pattern)
        if Major then
            Flavor = Flavor:gsub("^%-", "") -- Remove leading hyphen from flavor
            return 
            {
                Major  = Major,
                Minor  = Minor,
                Patch  = Patch,
                Flavor = Flavor,
                Date   = Date
            }
        end
    end

    LogError("No version number found in %s", ChangesFile)
end

-- Generate build info headers
function Glslang_GenerateBuildTimeHeaders()
    -- NOTE: This requires python to be installed
    local GlslangPath      = GetEnginePath() .. "/Dependencies/glslang"
    local ScriptPath       = GlslangPath .. "/build_info.py"
    local TemplateFilePath = GlslangPath .. "/build_info.h.tmpl"
    local OutputFilePath   = GlslangPath .. "/glslang/include/glslang/build_info.h"

    -- Local variable to store the template file in
    local Template

    -- Load the template file
    local File = io.open(TemplateFilePath, "r")
    if File then
        Template = File:read("*a")
        File:close()
    else
        LogError("Failed to open file '%s'", TemplateFilePath)
        return
    end

    local SoftwareVersion = Glslang_DeduceSoftwareVersion(GlslangPath)

    local Output = Template;
    Output = string.gsub(Output, "@major@", SoftwareVersion.Major)
    Output = string.gsub(Output, "@minor@", SoftwareVersion.Minor)
    Output = string.gsub(Output, "@patch@", SoftwareVersion.Patch)
    Output = string.gsub(Output, "@flavor@", SoftwareVersion.Flavor)
    Output = string.gsub(Output, "@date@", SoftwareVersion.Date)

    local File = io.open(OutputFilePath, "r")
    if File then
        local ExistingOutput = File:read("*a")
        File:close()

        if Output == ExistingOutput then
            return
        end
    end

    File = io.open(OutputFilePath, "w")
    if File then
        File:write(Output)
        File:close()
    else
        LogError("Failed to open file '%s'", OutputFilePath)
        return
    end
end

-- Define platform-specific settings
function Glslang_SetPlatformProperties()
    if (os.target() == "windows") then
        buildoptions
        {
            "/Zc:threadSafeInit-"
        }

        defines
        {
            "GLSLANG_OSINCLUDE_WIN32"
        }
    else
        defines
        {
            "GLSLANG_OSINCLUDE_UNIX" 
        }
    end
end

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
            
            -----------
            -- Imgui --
            -----------
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
                targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/ImGui/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/ImGui/" .. self.GetOutputPath())

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
            
            -------------------
            -- tinyobjloader --
            -------------------
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
                targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/tinyobjloader/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/tinyobjloader/" .. self.GetOutputPath())

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
            
            -------------
            -- OpenFBX --
            -------------
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
                targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/OpenFBX/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/OpenFBX/" .. self.GetOutputPath())

                -- Files
                files 
                {
                    (ExternalDependecyPath .. "/OpenFBX/src/ofbx.h"),
                    (ExternalDependecyPath .. "/OpenFBX/src/ofbx.cpp"),
                    (ExternalDependecyPath .. "/OpenFBX/src/libdeflate.h"),
                    (ExternalDependecyPath .. "/OpenFBX/src/libdeflate.c"),
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

            -----------------
            -- SPIRV-Cross --
            -----------------
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
                targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/SPIRV-Cross/" .. self.GetOutputPath())
                objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/SPIRV-Cross/" .. self.GetOutputPath())

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
            
            -------------------
            -- glslang group --
            -------------------
            group "Dependencies/glslang"
                LogInfo("\n    --- Generating glslang projects ---")

                -- Include directories for build-time generated include files
                local GLSLANG_GENERATED_INCLUDEDIR = path.join("build/generated/include", "glslang")

                --------------------
                -- GenericCodeGen --
                --------------------
                project "GenericCodeGen"
                    LogInfo("        Generating dependecy GenericCodeGen")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/GenericCodeGen/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/GenericCodeGen/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/GenericCodeGen/" .. self.GetOutputPath())

                    -- Files
                    files 
                    {
                        (ExternalDependecyPath .. "/glslang/glslang/GenericCodeGen/CodeGen.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/GenericCodeGen/Link.cpp"),
                    }

                    Glslang_SetPlatformProperties()

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

                -----------------
                -- OSDependent --
                -----------------
                project "OSDependent"
                    LogInfo("        Generating dependecy OSDependent")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/OSDependent/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/OSDependent/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/OSDependent/" .. self.GetOutputPath())

                    -- Include Directories
                    includedirs
                    {
                        (ExternalDependecyPath .. "/glslang/OGLCompilersDLL"),
                    }

                    -- Files
                    files 
                    {
                        (ExternalDependecyPath .. "/glslang/glslang/OSDependent/osinclude.h"),
                    }

                    filter "system:windows"
                        files 
                        {
                            (ExternalDependecyPath .. "/glslang/glslang/OSDependent/Windows/main.cpp"),
                            (ExternalDependecyPath .. "/glslang/glslang/OSDependent/Windows/ossource.cpp"),
                        }
                    filter "system:macosx"
                        files 
                        {
                            (ExternalDependecyPath .. "/glslang/glslang/OSDependent/Unix/ossource.cpp"),
                        }
                    filter {}

                    Glslang_SetPlatformProperties()

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

                ------------------------
                -- MachineIndependent --
                ------------------------
                project "MachineIndependent"
                    LogInfo("        Generating dependecy MachineIndependent")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/MachineIndependent/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/MachineIndependent/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/MachineIndependent/" .. self.GetOutputPath())

                    -- Include Directories
                    includedirs
                    {
                        (ExternalDependecyPath .. "/glslang/glslang/include")
                    }

                    -- Files
                    files 
                    {
                        -- Cpp files
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/glslang.y"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/glslang_tab.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/attribute.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Constant.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/iomapper.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/InfoSink.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Initialize.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/IntermTraverse.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Intermediate.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/ParseContextBase.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/ParseHelper.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/PoolAlloc.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/RemoveTree.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Scan.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/ShaderLang.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/SymbolTable.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Versions.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/intermOut.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/limits.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/linkValidate.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/parseConst.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/reflection.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/propagateNoContraction.cpp"),

                        -- Header Files
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/attribute.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/glslang_tab.cpp.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/gl_types.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Initialize.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/iomapper.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/LiveTraverser.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/localintermediate.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/ParseHelper.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/reflection.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/RemoveTree.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Scan.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/ScanContext.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/SymbolTable.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/Versions.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/parseVersions.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/propagateNoContraction.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/PpContext.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/MachineIndependent/preprocessor/PpTokens.h"),
                    }

                    Glslang_SetPlatformProperties()

                    Glslang_GenerateBuildTimeHeaders()

                    -- Links
                    links
                    {
                        "OGLCompiler",
                        "OSDependent",
                        "GenericCodeGen",
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

                -------------
                -- glslang --
                -------------
                project "glslang"
                    LogInfo("        Generating dependecy glslang")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/glslang/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/glslang/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/glslang/" .. self.GetOutputPath())

                    -- Include Directories
                    includedirs
                    {
                        (ExternalDependecyPath .. "/glslang")
                    }

                    -- Files
                    files 
                    {
                        -- Cpp
                        (ExternalDependecyPath .. "/glslang/glslang/CInterface/glslang_c_interface.cpp"),

                        -- Header
                        (ExternalDependecyPath .. "/glslang/glslang/Public/ShaderLang.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/arrays.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/BaseTypes.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/Common.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/ConstantUnion.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/glslang_c_interface.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/glslang_c_shader_types.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/InfoSink.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/InitializeGlobals.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/intermediate.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/PoolAlloc.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/ResourceLimits.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/ShHandle.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/SpirvIntrinsics.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Include/Types.h"),
                    }

                    -- Links
                    links
                    {
                        "OGLCompiler",
                        "OSDependent",
                        "MachineIndependent",
                    }

                    Glslang_SetPlatformProperties()

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

                --------------------
                -- ResourceLimits --
                --------------------
                project "glslang-default-resource-limits"
                    LogInfo("        Generating dependecy glslang-default-resource-limits")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/glslang-default-resource-limits/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/glslang-default-resource-limits/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/glslang-default-resource-limits/" .. self.GetOutputPath())

                    -- Include Directories
                    includedirs
                    {
                        (ExternalDependecyPath .. "/glslang")
                    }
                    
                    -- Files
                    files 
                    {
                        -- Cpp
                        (ExternalDependecyPath .. "/glslang/glslang/ResourceLimits/ResourceLimits.cpp"),
                        (ExternalDependecyPath .. "/glslang/glslang/ResourceLimits/resource_limits_c.cpp"),

                        -- Header
                        (ExternalDependecyPath .. "/glslang/glslang/Public/ResourceLimits.h"),
                        (ExternalDependecyPath .. "/glslang/glslang/Public/resource_limits_c.h"),
                    }

                    Glslang_SetPlatformProperties()

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

                ------------------
                -- OGLCompiler --
                ------------------
                project "OGLCompiler"
                    LogInfo("        Generating dependecy OGLCompiler")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/OGLCompiler/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/OGLCompiler/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/OGLCompiler/" .. self.GetOutputPath())

                    -- Files
                    files 
                    {
                        (ExternalDependecyPath .. "/glslang/OGLCompilersDLL/InitializeDll.h"),
                        (ExternalDependecyPath .. "/glslang/OGLCompilersDLL/InitializeDll.cpp"),
                    }

                    Glslang_SetPlatformProperties()

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

                ------------
                -- SPIR-V --
                ------------
                project "SPIRV"
                    LogInfo("        Generating dependecy SPIRV")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/SPIRV/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/SPIRV/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/SPIRV/" .. self.GetOutputPath())

                    -- Include Directories
                    includedirs
                    {
                        (ExternalDependecyPath .. "/glslang"),
                        (ExternalDependecyPath .. "/glslang/glslang/include"),
                    }

                    -- Files
                    files
                    {
                        -- Cpp
                        (ExternalDependecyPath .. "/glslang/SPIRV/GlslangToSpv.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/InReadableOrder.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/Logger.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/SpvBuilder.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/SpvPostProcess.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/doc.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/SpvTools.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/disassemble.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/CInterface/spirv_c_interface.cpp"),

                        -- Headers
                        (ExternalDependecyPath .. "/glslang/SPIRV/bitutils.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/spirv.hpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GLSL.std.450.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GLSL.ext.EXT.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GLSL.ext.KHR.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GlslangToSpv.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/hex_float.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/Logger.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/SpvBuilder.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/spvIR.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/doc.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/SpvTools.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/disassemble.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GLSL.ext.AMD.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GLSL.ext.NV.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/GLSL.ext.ARM.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/NonSemanticDebugPrintf.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/NonSemanticShaderDebugInfo100.h"),
                    }

                    -- Links
                    links
                    {
                        "MachineIndependent",
                    }

                    Glslang_SetPlatformProperties()

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

                ---------------------
                -- SPIR-V Remapper --
                ---------------------
                project "SPVRemapper"
                    LogInfo("        Generating dependecy SPVRemapper")

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
                    
                    location(SolutionLocation .. "/Dependencies/glslang/SPVRemapper/")
                
                    -- Locations
                    targetdir(ExternalDependecyPath .. "/Build/bin/Dependencies/glslang/SPVRemapper/" .. self.GetOutputPath())
                    objdir(ExternalDependecyPath .. "/Build/bin-int/Dependencies/glslang/SPVRemapper/" .. self.GetOutputPath())

                    -- Files
                    files 
                    {
                        (ExternalDependecyPath .. "/glslang/SPIRV/SPVRemapper.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/doc.cpp"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/SPVRemapper.h"),
                        (ExternalDependecyPath .. "/glslang/SPIRV/doc.h"),
                    }

                    Glslang_SetPlatformProperties()

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
            group "Dependencies"
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
                "DEBUG",
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