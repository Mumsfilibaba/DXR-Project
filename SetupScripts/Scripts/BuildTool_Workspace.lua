include "BuildTool_Module.lua"
include "BuildTool_Target.lua"

-- Function to deduce software version
function GlslangDeduceSoftwareVersion(Directory)
    -- Path to the CHANGES.md file
    local ChangesFile = JoinPath(Directory, "CHANGES.md")

    -- Create a pattern to match the version and date line in CHANGES.md
    local Pattern = "^#*%s*(%d+)%.(%d+)%.(%d+)%s*(-?[%w]*)%s*(%d%d%d%d%-%d%d%-%d%d)%s*"

    -- Read the file line by line
    for Line in io.lines(ChangesFile) do
        local Major, Minor, Patch, Flavor, Date = Line:match(Pattern)
        if Major then
            Flavor = Flavor:gsub("^%-", "") -- Remove leading hyphen from flavor

            return {
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
function GlslangGenerateBuildTimeHeaders()
    LogInfo("            Generating BuildTime Headers for 'glslang'")

    -- NOTE: This requires Python to be installed
    local GlslangPath       = JoinPath(GetEnginePath(), "ThirdParty/glslang")
    local ScriptPath        = JoinPath(GlslangPath, "build_info.py")
    local TemplateFilePath  = JoinPath(GlslangPath, "build_info.h.tmpl")
    local OutputFileDir     = JoinPath(GlslangPath, "glslang/include/glslang")
    local OutputFilePath    = OutputFileDir .. "/build_info.h"

    -- Load the template file
    local File = io.open(TemplateFilePath, "r")
    local Template
    if File then
        LogInfo("            Loaded template file '%s'", TemplateFilePath)
        Template = File:read("*a")
        File:close()
    else
        LogError("Failed to open template file '%s'", TemplateFilePath)
        return
    end

    local SoftwareVersion = GlslangDeduceSoftwareVersion(GlslangPath)
    LogInfo("            SoftwareVersion @major@ '%d'", SoftwareVersion.Major)
    LogInfo("            SoftwareVersion @minor@ '%d'", SoftwareVersion.Minor)
    LogInfo("            SoftwareVersion @patch@ '%d'", SoftwareVersion.Patch)
    LogInfo("            SoftwareVersion @flavor@ '%s'", SoftwareVersion.Flavor)
    LogInfo("            SoftwareVersion @date@ '%s'", SoftwareVersion.Date)
    
    local Output = Template
    Output = string.gsub(Output, "@major@",  SoftwareVersion.Major)
    Output = string.gsub(Output, "@minor@",  SoftwareVersion.Minor)
    Output = string.gsub(Output, "@patch@",  SoftwareVersion.Patch)
    Output = string.gsub(Output, "@flavor@", SoftwareVersion.Flavor)
    Output = string.gsub(Output, "@date@",   SoftwareVersion.Date)

    if not os.isdir(OutputFileDir) then
        LogInfo("            'build_info.h' does not exist yet, creating file...")

        local Success, Err = os.mkdir(OutputFileDir)
        if not Success then
            LogError("Failed to create output directory '%s': %s", OutputFileDir, Err)
            return
        end
    else
        local ExistingFile, OpenErr = io.open(OutputFilePath, "r")
        if ExistingFile then
            local ExistingOutput = ExistingFile:read("*a")
            ExistingFile:close()
    
            if Output == ExistingOutput then
                LogInfo("            'build_info.h' is equal to the generated one, skipping file creation")
                return
            end
        else
            LogInfo("            'build_info.h' does not exist yet, creating file...")
        end
    end

    local OutFile, WriteErr = io.open(OutputFilePath, "w")
    if OutFile then
        OutFile:write(Output)
        OutFile:close()
        LogInfo("            ... finished creating 'build_info.h'")
    else
        LogError("Failed to open output file '%s': %s", OutputFilePath, WriteErr)
        return
    end
end

-- Define platform-specific settings
function GlslangSetPlatformProperties()
    if os.target() == "windows" then
        buildoptions {
            "/Zc:threadSafeInit-"
        }
        defines {
            "GLSLANG_OSINCLUDE_WIN32"
        }
    else
        defines {
            "GLSLANG_OSINCLUDE_UNIX"
        }
    end
end

-- Generate a workspace from an array of target rules
function WorkspaceRules(WorkspaceName)
    -- Must have a valid workspace name
    if WorkspaceName == nil then
        return nil
    end

    LogHighlight("Creating Workspace '%s'", WorkspaceName)

    -- Initialize this object
    local self = {
        -- Name of the workspace being generated
        Name = WorkspaceName,
        
        -- List of targets for this workspace
        TargetRules = {},

        -- Name of the target of the workspace
        TargetName = "",
        
        -- Engine folder path
        EnginePath = GetEnginePath(),

        -- Defines
        Defines = {},

        -- Projects that should have projects generated
        ProjectRules = {},

        -- Name of the project that should be set as startup project
        StartProjectName = "",
    }

    -- Output path for thirdparties (ImGui, etc.)
    function self.GetOutputPath()
        return "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"
    end

    -- Retrieve the current target name
    function self.GetCurrentTargetName()
        return self.TargetName
    end

    -- Retrieve a target added to the workspace
    function self.GetTarget(TargetName)
        return self.TargetRules[TargetName]
    end

    -- Check if a target already exists
    function self.IsTarget(TargetName)
        return self.GetTarget(TargetName) ~= nil
    end

    -- Helper function for adding a target
    function self.AddTarget(Target)
        table.insert(self.TargetRules, Target)
    end

    -- Helper function for adding defines
    function self.AddDefines(Define)
        AddUniqueElements(Define, self.Defines)
    end

    -- Helper function for adding a rule
    function self.AddRule(Rule)
        table.insert(self.ProjectRules, Rule)
    end

    -- Inject thirdparty projects into the workspace
    function self.GenerateThirdpartyProjects()
        local SolutionLocation = GetSolutionsFolderPath()

        group "ThirdParty"
            LogInfo("\n--- External ThirdParty ---")
            
            -- ImGui
            project "ImGui"
                LogInfo("    Generating thirdparty ImGui")

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
                vectorextensions("Default")
                characterset("Ascii")
                flags { "MultiProcessorCompile", "NoIncrementalLink" }
                
                -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                filter "action:vs*"
                    buildoptions { "/Zc:__cplusplus" }
                filter {}

                location(JoinPath(SolutionLocation, "ThirdParty/ImGui"))

                -- Locations
                targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/ImGui/" .. self.GetOutputPath()))
                objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/ImGui/" .. self.GetOutputPath()))

                -- Files
                files {
                    CreateExternalThirdpartyPath("imgui/imconfig.h"),
                    CreateExternalThirdpartyPath("imgui/imgui.h"),
                    CreateExternalThirdpartyPath("imgui/imgui.cpp"),
                    CreateExternalThirdpartyPath("imgui/imgui_demo.cpp"),
                    CreateExternalThirdpartyPath("imgui/imgui_draw.cpp"),
                    CreateExternalThirdpartyPath("imgui/imgui_internal.h"),
                    CreateExternalThirdpartyPath("imgui/imgui_tables.cpp"),
                    CreateExternalThirdpartyPath("imgui/imgui_widgets.cpp"),
                    CreateExternalThirdpartyPath("imgui/imstb_rectpack.h"),
                    CreateExternalThirdpartyPath("imgui/imstb_textedit.h"),
                    CreateExternalThirdpartyPath("imgui/imstb_truetype.h"),
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
            
            -- tinyobjloader
            project "tinyobjloader"
                LogInfo("    Generating thirdparty tinyobjloader")

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
                vectorextensions("Default")
                characterset("Ascii")
                flags { "MultiProcessorCompile", "NoIncrementalLink" }

                -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                filter "action:vs*"
                    buildoptions { "/Zc:__cplusplus" }
                filter {}

                location(JoinPath(SolutionLocation, "ThirdParty/tinyobjloader"))

                -- Locations
                targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/tinyobjloader/" .. self.GetOutputPath()))
                objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/tinyobjloader/" .. self.GetOutputPath()))

                -- Files
                files {
                    CreateExternalThirdpartyPath("tinyobjloader/tiny_obj_loader.h"),
                    CreateExternalThirdpartyPath("tinyobjloader/tiny_obj_loader.cc"),
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
            
            -- OpenFBX
            project "OpenFBX"
                LogInfo("    Generating thirdparty OpenFBX")

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
                vectorextensions("Default")
                characterset("Ascii")
                flags { "MultiProcessorCompile", "NoIncrementalLink" }
                
                -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                filter "action:vs*"
                    buildoptions { "/Zc:__cplusplus" }
                filter {}

                location(JoinPath(SolutionLocation, "ThirdParty/OpenFBX"))
            
                -- Locations
                targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/OpenFBX/" .. self.GetOutputPath()))
                objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/OpenFBX/" .. self.GetOutputPath()))

                -- Files
                files {
                    CreateExternalThirdpartyPath("OpenFBX/src/ofbx.h"),
                    CreateExternalThirdpartyPath("OpenFBX/src/ofbx.cpp"),
                    CreateExternalThirdpartyPath("OpenFBX/src/libdeflate.h"),
                    CreateExternalThirdpartyPath("OpenFBX/src/libdeflate.c"),
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

            -- SPIRV-Cross
            project "SPIRV-Cross"
                LogInfo("    Generating thirdparty SPIRV-Cross")

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
                vectorextensions("Default")
                characterset("Ascii")
                flags { "MultiProcessorCompile", "NoIncrementalLink" }
                
                -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                filter "action:vs*"
                    buildoptions { "/Zc:__cplusplus" }
                filter {}

                location(JoinPath(SolutionLocation, "ThirdParty/SPIRV-Cross"))
            
                -- Locations
                targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/SPIRV-Cross/" .. self.GetOutputPath()))
                objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/SPIRV-Cross/" .. self.GetOutputPath()))

                -- Files
                files {
                    CreateExternalThirdpartyPath("SPIRV-Cross/GLSL.std.450.h"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv.h"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_c.h"),

                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cfg.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_common.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cpp.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_containers.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_error_handling.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_parsed_ir.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_util.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_glsl.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_hlsl.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_msl.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_parser.hpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_reflect.hpp"),

                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cfg.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cpp.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_c.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_parsed_ir.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_cross_util.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_glsl.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_hlsl.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_msl.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_parser.cpp"),
                    CreateExternalThirdpartyPath("SPIRV-Cross/spirv_reflect.cpp"),
                }

                -- Defines 
                defines {
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
            
            -- glslang group
            group "ThirdParty/glslang"
                LogInfo("\n    --- Generating glslang projects ---")

                -- Include directories for build-time generated include files
                local GlslangGeneratedIncludeDir = JoinPath("build/generated/include", "glslang")

                -- GenericCodeGen
                project "GenericCodeGen"
                    LogInfo("        Generating thirdparty GenericCodeGen")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}

                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/GenericCodeGen/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/GenericCodeGen/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/GenericCodeGen/" .. self.GetOutputPath()))

                    -- Files
                    files {
                        CreateExternalThirdpartyPath("glslang/glslang/GenericCodeGen/CodeGen.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/GenericCodeGen/Link.cpp"),
                    }

                    GlslangSetPlatformProperties()

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

                -- OSDependent
                project "OSDependent"
                    LogInfo("        Generating thirdparty OSDependent")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}

                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/OSDependent/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/OSDependent/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/OSDependent/" .. self.GetOutputPath()))

                    -- Files
                    files {
                        CreateExternalThirdpartyPath("glslang/glslang/OSDependent/osinclude.h"),
                    }

                    filter "system:windows"
                        files {
                            CreateExternalThirdpartyPath("glslang/glslang/OSDependent/Windows/ossource.cpp"),
                        }
                    filter "system:macosx"
                        files {
                            CreateExternalThirdpartyPath("glslang/glslang/OSDependent/Unix/ossource.cpp"),
                        }
                    filter {}

                    GlslangSetPlatformProperties()

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

                -- MachineIndependent
                project "MachineIndependent"
                    LogInfo("        Generating thirdparty MachineIndependent")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}

                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/MachineIndependent/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/MachineIndependent/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/MachineIndependent/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs {
                        CreateExternalThirdpartyPath("glslang"),
                        CreateExternalThirdpartyPath("glslang/glslang/include"),
                    }

                    -- Files
                    files {
                        -- Cpp files
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/glslang.y"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/glslang_tab.cpp"),

                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/attribute.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Constant.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/InfoSink.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Initialize.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/intermOut.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/IntermTraverse.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/iomapper.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Intermediate.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/limits.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/linkValidate.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/parseConst.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/ParseContextBase.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/ParseHelper.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/PoolAlloc.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/propagateNoContraction.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/reflection.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/RemoveTree.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Scan.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/ShaderLang.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/SymbolTable.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Versions.cpp"),

                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/Pp.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp"),

                        -- Header Files
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/glslang_tab.cpp.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/gl_types.h"),

                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/attribute.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Initialize.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/iomapper.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/LiveTraverser.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/localintermediate.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/ParseHelper.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/parseVersions.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/propagateNoContraction.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/reflection.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/RemoveTree.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Scan.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/ScanContext.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/span.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/SymbolTable.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/Versions.h"),

                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/PpContext.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/MachineIndependent/preprocessor/PpTokens.h"),
                    }

                    GlslangSetPlatformProperties()
                    GlslangGenerateBuildTimeHeaders()

                    -- Links
                    links {
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

                -- glslang
                project "glslang"
                    LogInfo("        Generating thirdparty glslang")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}

                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/glslang/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/glslang/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/glslang/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs {
                        CreateExternalThirdpartyPath("glslang")
                    }

                    -- Files
                    files {
                        -- Cpp
                        CreateExternalThirdpartyPath("glslang/glslang/CInterface/glslang_c_interface.cpp"),

                        -- Header
                        CreateExternalThirdpartyPath("glslang/glslang/Public/ShaderLang.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/arrays.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/BaseTypes.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/Common.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/ConstantUnion.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/glslang_c_interface.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/glslang_c_shader_types.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/InfoSink.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/InitializeGlobals.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/intermediate.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/PoolAlloc.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/ResourceLimits.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/ShHandle.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/SpirvIntrinsics.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Include/Types.h"),
                    }

                    -- Links
                    links {
                        "OSDependent",
                        "MachineIndependent",
                    }

                    GlslangSetPlatformProperties()

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

                -- ResourceLimits
                project "glslang-default-resource-limits"
                    LogInfo("        Generating thirdparty glslang-default-resource-limits")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}

                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/glslang-default-resource-limits/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/glslang-default-resource-limits/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/glslang-default-resource-limits/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs {
                        CreateExternalThirdpartyPath("glslang")
                    }
                    
                    -- Files
                    files {
                        -- Cpp
                        CreateExternalThirdpartyPath("glslang/glslang/ResourceLimits/ResourceLimits.cpp"),
                        CreateExternalThirdpartyPath("glslang/glslang/ResourceLimits/resource_limits_c.cpp"),

                        -- Header
                        CreateExternalThirdpartyPath("glslang/glslang/Public/ResourceLimits.h"),
                        CreateExternalThirdpartyPath("glslang/glslang/Public/resource_limits_c.h"),
                    }

                    GlslangSetPlatformProperties()

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

                -- SPIRV
                project "SPIRV"
                    LogInfo("        Generating thirdparty SPIRV")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}

                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/SPIRV/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/SPIRV/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/SPIRV/" .. self.GetOutputPath()))

                    -- Include Directories
                    includedirs {
                        CreateExternalThirdpartyPath("glslang"),
                        CreateExternalThirdpartyPath("glslang/glslang/include"),
                    }

                    -- Files
                    files {
                        -- Cpp
                        CreateExternalThirdpartyPath("glslang/SPIRV/GlslangToSpv.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/InReadableOrder.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/Logger.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/SpvBuilder.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/SpvPostProcess.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/doc.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/SpvTools.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/disassemble.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/CInterface/spirv_c_interface.cpp"),

                        -- Headers
                        CreateExternalThirdpartyPath("glslang/SPIRV/bitutils.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/spirv.hpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GLSL.std.450.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GLSL.ext.EXT.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GLSL.ext.KHR.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GlslangToSpv.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/hex_float.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/Logger.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/SpvBuilder.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/spvIR.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/doc.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/SpvTools.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/disassemble.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GLSL.ext.AMD.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GLSL.ext.NV.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/GLSL.ext.ARM.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/NonSemanticDebugPrintf.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/NonSemanticShaderDebugInfo100.h"),
                    }

                    -- Links
                    links {
                        "MachineIndependent",
                    }

                    GlslangSetPlatformProperties()

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

                -- SPVRemapper
                project "SPVRemapper"
                    LogInfo("        Generating thirdparty SPVRemapper")

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
                    vectorextensions("Default")
                    characterset("Ascii")
                    flags { "MultiProcessorCompile", "NoIncrementalLink" }
                    
                    -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
                    filter "action:vs*"
                        buildoptions { "/Zc:__cplusplus" }
                    filter {}
                    
                    location(JoinPath(SolutionLocation, "ThirdParty/glslang/SPVRemapper/"))
                
                    -- Locations
                    targetdir(CreateExternalThirdpartyPath("Build/bin/ThirdParty/glslang/SPVRemapper/" .. self.GetOutputPath()))
                    objdir(CreateExternalThirdpartyPath("Build/bin-int/ThirdParty/glslang/SPVRemapper/" .. self.GetOutputPath()))

                    -- Files
                    files {
                        CreateExternalThirdpartyPath("glslang/SPIRV/SPVRemapper.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/doc.cpp"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/SPVRemapper.h"),
                        CreateExternalThirdpartyPath("glslang/SPIRV/doc.h"),
                    }

                    GlslangSetPlatformProperties()

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
            group "ThirdParty"
        group ""
    end

    -- Generate the actual solution files
    function self.GenerateSolutionFiles()
        LogInfo("\n--- Generating Solution Files for Workspace '%s' ---", self.Name)

        -- Set the name of the workspace
        workspace(self.Name)

        -- Set location of the generated solution file
        local SolutionLocation = GetSolutionsFolderPath()
        location(SolutionLocation)

        LogInfo("    Generated solution location '%s'", SolutionLocation)

        -- Platforms
        platforms { "x64" }

        -- Configurations
        configurations 
        {
            "Debug",
            "Release",
            "Production",
        }

        -- Includes
        local RuntimeFolderPathLocal = GetRuntimeFolderPath()
        includedirs { RuntimeFolderPathLocal }

        -- Workspace defines
        LogInfo("\n--- Workspace Defines (Num Defines=%d) ---", #self.Defines)
        if #self.Defines > 0 then
            PrintTable("    Using Define '%s'", self.Defines)
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
            defines { "ARCHITECTURE_X86=(1)" }
        filter {}

        filter "architecture:x86_x64"
            defines { "PLATFORM_ARCHITECTURE_X86_X64=(1)" }
        filter {}

        filter "architecture:ARM"
            defines { "PLATFORM_ARCHITECTURE_ARM=(1)" }
        filter {}

        -- Startup project name
        LogInfo("    StartProject = '%s'", self.StartProjectName)
        startproject(self.StartProjectName)

        -- Generate projects for all thirdparties
        self.GenerateThirdpartyProjects()

        -- Generate project files for all the rules that have been added
        LogInfo("\n--- Generating module and target project files ---")
        for _, CurrentRule in ipairs(self.ProjectRules) do
            CurrentRule.GenerateProject()
        end
    end

    -- Generate workspace
    function self.Generate()
        LogInfo("\n--- Generating Workspace '%s' ---", self.Name)
        LogInfo("OutputPath = '%s'", self.GetOutputPath())

        if self.TargetRules == nil then
            LogError("TargetRules cannot be nil")
            return
        end

        if #self.TargetRules < 1 then
            LogError("Workspace must contain at least one build rule (Current=%d)", #self.TargetRules)
            return
        end

        -- Define the workspace location; we do this with a Unix path since the engine (C++ side) expects this currently
        local UnixEnginePath = path.translate(GetEnginePath(), "/")
        local EngineLocation = 'ENGINE_LOCATION="' .. UnixEnginePath .. '"'
        self.AddDefines { EngineLocation }
        
        LogInfo("    Engine Path ='%s'", GetEnginePath())
        LogInfo("    RuntimeFolderPath = '%s'", GetRuntimeFolderPath())
        
        -- Check if the command line overrides monolithic builds
        if IsBuildMonolithic() then
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
        for _, CurrentTarget in ipairs(self.TargetRules) do
            self.TargetName = CurrentTarget.Name
            CurrentTarget.Workspace = self
            CurrentTarget.Generate()
        end

        -- Generate the actual solution files
        self.GenerateSolutionFiles()

        LogInfo("\n--- Finished generating workspace ---")
    end

    return self
end
