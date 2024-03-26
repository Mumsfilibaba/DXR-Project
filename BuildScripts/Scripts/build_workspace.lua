include "build_module.lua"
include "build_target.lua"

-- Function to deduce software version
function Glslang_DeduceSoftwareVersion(Directory)
    -- Path to the CHANGES.md file
    local ChangesFile = JoinPath(Directory, "CHANGES.md")

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
    local GlslangPath      = JoinPath(GetEnginePath(), "Dependencies/glslang")
    local ScriptPath       = JoinPath(GlslangPath, "build_info.py")
    local TemplateFilePath = JoinPath(GlslangPath, "build_info.h.tmpl")
    local OutputFilePath   = JoinPath(GlslangPath, "glslang/include/glslang/build_info.h")

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
    local _RuntimeFolderPath = JoinPath(self.GetEnginePath(), "Runtime")
    function self.GetRuntimeFolderPath()
        return _RuntimeFolderPath
    end

    -- @brief - Retreive the path of the engine 'Runtime' folder
    local _BuildFolderPath = JoinPath(self.GetEnginePath(), "Build")
    function self.GetBuildFolderPath()
        return _BuildFolderPath
    end

    -- @brief - Retreive the path of the engine 'Solutions' folder
    local _SolutionsFolderPath = JoinPath(self.GetEnginePath(), "Solutions")
    function self.GetSolutionsFolderPath()
        return _SolutionsFolderPath
    end

    -- @brief - Retrieve the path to the dependencies folder containing external dependecy projects
    local _ExternalDependenciesFolderPath = JoinPath(self.GetEnginePath(), "Dependencies")
    function self.GetExternalDependenciesFolderPath()
        return _ExternalDependenciesFolderPath
    end

    -- @brief - Create a path relative to dependency folder
    function self.CreateExternalDependencyPath(Path)
        return JoinPath(GetExternalDependenciesFolderPath(), Path)
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
        local SolutionLocation = self.GetSolutionsFolderPath()

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

                location(JoinPath(SolutionLocation, "Dependencies/ImGui"))

                -- Locations
                targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/ImGui/" .. self.GetOutputPath()))
                objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/ImGui/" .. self.GetOutputPath()))

                -- Files
                files
                {
                    self.CreateExternalDependencyPath("imgui/imconfig.h"),
                    self.CreateExternalDependencyPath("imgui/imgui.h"),
                    self.CreateExternalDependencyPath("imgui/imgui.cpp"),
                    self.CreateExternalDependencyPath("imgui/imgui_demo.cpp"),
                    self.CreateExternalDependencyPath("imgui/imgui_draw.cpp"),
                    self.CreateExternalDependencyPath("imgui/imgui_internal.h"),
                    self.CreateExternalDependencyPath("imgui/imgui_tables.cpp"),
                    self.CreateExternalDependencyPath("imgui/imgui_widgets.cpp"),
                    self.CreateExternalDependencyPath("imgui/imstb_rectpack.h"),
                    self.CreateExternalDependencyPath("imgui/imstb_textedit.h"),
                    self.CreateExternalDependencyPath("imgui/imstb_truetype.h"),
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

                location(JoinPath(SolutionLocation, "Dependencies/tinyobjloader"))

                -- Locations
                targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/tinyobjloader/" .. self.GetOutputPath()))
                objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/tinyobjloader/" .. self.GetOutputPath()))

                -- Files
                files 
                {
                    self.CreateExternalDependencyPath("tinyobjloader/tiny_obj_loader.h"),
                    self.CreateExternalDependencyPath("tinyobjloader/tiny_obj_loader.cc"),
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
                
                location(JoinPath(SolutionLocation, "Dependencies/OpenFBX"))
            
                -- Locations
                targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/OpenFBX/" .. self.GetOutputPath()))
                objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/OpenFBX/" .. self.GetOutputPath()))

                -- Files
                files 
                {
                    self.CreateExternalDependencyPath("OpenFBX/src/ofbx.h"),
                    self.CreateExternalDependencyPath("OpenFBX/src/ofbx.cpp"),
                    self.CreateExternalDependencyPath("OpenFBX/src/libdeflate.h"),
                    self.CreateExternalDependencyPath("OpenFBX/src/libdeflate.c"),
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
                
                location(JoinPath(SolutionLocation, "Dependencies/SPIRV-Cross"))
            
                -- Locations
                targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/SPIRV-Cross/" .. self.GetOutputPath()))
                objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/SPIRV-Cross/" .. self.GetOutputPath()))

                -- Files
                files 
                {
                    self.CreateExternalDependencyPath("SPIRV-Cross/GLSL.std.450.h"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv.h"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_c.h"),

                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cfg.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_common.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cpp.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_containers.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_error_handling.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_parsed_ir.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_util.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_glsl.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_hlsl.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_msl.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_parser.hpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_reflect.hpp"),

                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cfg.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cpp.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_c.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_parsed_ir.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_cross_util.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_glsl.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_hlsl.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_msl.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_parser.cpp"),
                    self.CreateExternalDependencyPath("SPIRV-Cross/spirv_reflect.cpp"),
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
                local GLSLANG_GENERATED_INCLUDEDIR = JoinPath("build/generated/include", "glslang")

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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/GenericCodeGen/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/GenericCodeGen/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/GenericCodeGen/" .. self.GetOutputPath()))

                    -- Files
                    files 
                    {
                        self.CreateExternalDependencyPath("glslang/glslang/GenericCodeGen/CodeGen.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/GenericCodeGen/Link.cpp"),
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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/OSDependent/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/OSDependent/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/OSDependent/" .. self.GetOutputPath()))

                    -- Files
                    files 
                    {
                        self.CreateExternalDependencyPath("glslang/glslang/OSDependent/osinclude.h"),
                    }

                    filter "system:windows"
                        files 
                        {
                            self.CreateExternalDependencyPath("glslang/glslang/OSDependent/Windows/ossource.cpp"),
                        }
                    filter "system:macosx"
                        files 
                        {
                            self.CreateExternalDependencyPath("glslang/glslang/OSDependent/Unix/ossource.cpp"),
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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/MachineIndependent/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/MachineIndependent/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/MachineIndependent/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs
                    {
                        self.CreateExternalDependencyPath("glslang/glslang/include")
                    }

                    -- Files
                    files 
                    {
                        -- Cpp files
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/glslang.y"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/glslang_tab.cpp"),

                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/attribute.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Constant.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/InfoSink.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Initialize.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/intermOut.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/IntermTraverse.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/iomapper.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Intermediate.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/limits.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/linkValidate.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/parseConst.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/ParseContextBase.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/ParseHelper.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/PoolAlloc.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/propagateNoContraction.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/reflection.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/RemoveTree.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Scan.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/ShaderLang.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/SymbolTable.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Versions.cpp"),

                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/Pp.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp"),

                        -- Header Files
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/glslang_tab.cpp.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/gl_types.h"),

                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/attribute.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Initialize.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/iomapper.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/LiveTraverser.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/localintermediate.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/ParseHelper.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/parseVersions.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/propagateNoContraction.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/reflection.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/RemoveTree.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Scan.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/ScanContext.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/span.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/SymbolTable.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/Versions.h"),

                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/PpContext.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/MachineIndependent/preprocessor/PpTokens.h"),
                    }

                    Glslang_SetPlatformProperties()

                    Glslang_GenerateBuildTimeHeaders()

                    -- Links
                    links
                    {
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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/glslang/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/glslang/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/glslang/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs
                    {
                        self.CreateExternalDependencyPath("glslang")
                    }

                    -- Files
                    files 
                    {
                        -- Cpp
                        self.CreateExternalDependencyPath("glslang/glslang/CInterface/glslang_c_interface.cpp"),

                        -- Header
                        self.CreateExternalDependencyPath("glslang/glslang/Public/ShaderLang.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/arrays.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/BaseTypes.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/Common.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/ConstantUnion.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/glslang_c_interface.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/glslang_c_shader_types.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/InfoSink.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/InitializeGlobals.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/intermediate.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/PoolAlloc.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/ResourceLimits.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/ShHandle.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/SpirvIntrinsics.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Include/Types.h"),
                    }

                    -- Links
                    links
                    {
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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/glslang-default-resource-limits/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/glslang-default-resource-limits/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/glslang-default-resource-limits/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs
                    {
                        self.CreateExternalDependencyPath("glslang")
                    }
                    
                    -- Files
                    files 
                    {
                        -- Cpp
                        self.CreateExternalDependencyPath("glslang/glslang/ResourceLimits/ResourceLimits.cpp"),
                        self.CreateExternalDependencyPath("glslang/glslang/ResourceLimits/resource_limits_c.cpp"),

                        -- Header
                        self.CreateExternalDependencyPath("glslang/glslang/Public/ResourceLimits.h"),
                        self.CreateExternalDependencyPath("glslang/glslang/Public/resource_limits_c.h"),
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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/SPIRV/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/SPIRV/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/SPIRV/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs
                    {
                        self.CreateExternalDependencyPath("glslang"),
                        self.CreateExternalDependencyPath("glslang/glslang/include"),
                    }

                    -- Files
                    files
                    {
                        -- Cpp
                        self.CreateExternalDependencyPath("glslang/SPIRV/GlslangToSpv.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/InReadableOrder.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/Logger.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/SpvBuilder.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/SpvPostProcess.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/doc.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/SpvTools.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/disassemble.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/CInterface/spirv_c_interface.cpp"),

                        -- Headers
                        self.CreateExternalDependencyPath("glslang/SPIRV/bitutils.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/spirv.hpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GLSL.std.450.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GLSL.ext.EXT.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GLSL.ext.KHR.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GlslangToSpv.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/hex_float.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/Logger.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/SpvBuilder.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/spvIR.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/doc.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/SpvTools.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/disassemble.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GLSL.ext.AMD.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GLSL.ext.NV.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/GLSL.ext.ARM.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/NonSemanticDebugPrintf.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/NonSemanticShaderDebugInfo100.h"),
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
                    
                    location(JoinPath(SolutionLocation, "Dependencies/glslang/SPVRemapper/"))
                
                    -- Locations
                    targetdir(self.CreateExternalDependencyPath("Build/bin/Dependencies/glslang/SPVRemapper/" .. self.GetOutputPath()))
                    objdir(self.CreateExternalDependencyPath("Build/bin-int/Dependencies/glslang/SPVRemapper/" .. self.GetOutputPath()))

                    -- Files
                    files 
                    {
                        self.CreateExternalDependencyPath("glslang/SPIRV/SPVRemapper.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/doc.cpp"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/SPVRemapper.h"),
                        self.CreateExternalDependencyPath("glslang/SPIRV/doc.h"),
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

        -- Define the workspace location we do this with a Unix Path since the engine (C++ side) expects this currently
        local UnixEnginePath = path.translate(self.GetEnginePath(), "/")
        local EngineLocation = "ENGINE_LOCATION=" .. "\"" .. UnixEnginePath .. "\""
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