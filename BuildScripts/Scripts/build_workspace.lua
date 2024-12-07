include "build_module.lua"
include "build_target.lua"

-- Function to deduce software version
function glslang_deduce_software_version(directory)
    -- Path to the CHANGES.md file
    local changes_file = join_path(directory, "CHANGES.md")

    -- Create a pattern to match the version and date line in CHANGES.md
    local pattern = "^#*%s*(%d+)%.(%d+)%.(%d+)%s*(-?[%w]*)%s*(%d%d%d%d%-%d%d%-%d%d)%s*"

    -- Read the file line by line
    for line in io.lines(changes_file) do
        local major, minor, patch, flavor, date = line:match(pattern)
        if major then
            flavor = flavor:gsub("^%-", "") -- Remove leading hyphen from flavor

            return 
            {
                major  = major,
                minor  = minor,
                patch  = patch,
                flavor = flavor,
                date   = date
            }
        end
    end

    log_error("No version number found in %s", changes_file)
end

-- Generate build info headers
function glslang_generate_build_time_headers()
    log_info("            Generating BuildTime Headers for 'glslang'")

    -- NOTE: This requires Python to be installed
    local glslang_path       = join_path(get_engine_path(), "Dependencies/glslang")
    local script_path        = join_path(glslang_path, "build_info.py")
    local template_file_path = join_path(glslang_path, "build_info.h.tmpl")
    local output_file_dir    = join_path(glslang_path, "glslang/include/glslang")
    local output_file_path   = output_file_dir .. "/build_info.h"

    -- Load the template file
    local file = io.open(template_file_path, "r")
    if file then
        log_info("            Loaded template file '%s'", template_file_path)
        template = file:read("*a")
        file:close()
    else
        log_error("Failed to open template file '%s'", template_file_path)
        return
    end

    local software_version = glslang_deduce_software_version(glslang_path)
    log_info("            SoftwareVersion @major@ '%d'", software_version.major)
    log_info("            SoftwareVersion @minor@ '%d'", software_version.minor)
    log_info("            SoftwareVersion @patch@ '%d'", software_version.patch)
    log_info("            SoftwareVersion @flavor@ '%s'", software_version.flavor)
    log_info("            SoftwareVersion @date@ '%s'", software_version.date)
    
    local output = template
    output = string.gsub(output, "@major@", software_version.major)
    output = string.gsub(output, "@minor@", software_version.minor)
    output = string.gsub(output, "@patch@", software_version.patch)
    output = string.gsub(output, "@flavor@", software_version.flavor)
    output = string.gsub(output, "@date@", software_version.date)

    if not os.isdir(output_file_dir) then
        log_info("            'build_info.h' does not exist yet, creating file...")

        local success, err = os.mkdir(output_file_dir)
        if not success then
            log_error("Failed to create output directory '%s': %s", output_file_dir, err)
            return
        end
    else
        local file, err = io.open(output_file_path, "r")
        if file then
            local existing_output = file:read("*a")
            file:close()
    
            if output == existing_output then
                log_info("            'build_info.h' is equal to the generated one, skipping file creation")
                return
            end
        else
            log_info("            'build_info.h' does not exist yet, creating file...")
        end
    end

    local file, err = io.open(output_file_path, "w")
    if file then
        file:write(output)
        file:close()
        log_info("            ... finished creating 'build_info.h'")
    else
        log_error("Failed to open output file '%s': %s", output_file_path, err)
        return
    end
end

-- Define platform-specific settings
function glslang_set_platform_properties()
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
function workspace_rules(workspace_name)
    -- Must have a valid workspace name
    if workspace_name == nil then
        return nil
    end

    log_highlight("Creating Workspace '%s'", workspace_name)

    -- Initialize this object
    local self = {
        -- @brief - Name of the workspace being generated
        name = workspace_name,
        
        -- @brief - List of targets for this workspace
        target_rules = {},

        -- @brief - Name of the target of the workspace
        target_name = "",
        
        -- @brief - Engine folder path
        engine_path = get_engine_path(),

        -- @brief - Defines
        defines = {},

        -- @brief - Projects that should have projects generated
        project_rules = {},

        -- @brief - Name of the project that should be set as startup project
        start_project_name = "",
    }

    -- @brief - Output path for dependencies (ImGui, etc.)
    function self.get_output_path()
        return "%{cfg.buildcfg}-%{cfg.system}-%{cfg.platform}"
    end

    -- @brief - Retrieve the path of the engine
    function self.get_engine_path()
        return self.engine_path
    end

    -- @brief - Retrieve the current target name
    function self.get_current_target_name()
        return self.target_name
    end

    -- @brief - Retrieve the path of the engine 'Runtime' folder
    local runtime_folder_path = join_path(self.get_engine_path(), "Runtime")
    function self.get_runtime_folder_path()
        return runtime_folder_path
    end

    -- @brief - Retrieve the path of the engine 'Build' folder
    local build_folder_path = join_path(self.get_engine_path(), "Build")
    function self.get_build_folder_path()
        return build_folder_path
    end

    -- @brief - Retrieve the path of the engine 'Solutions' folder
    local solutions_folder_path = join_path(self.get_engine_path(), "Solutions")
    function self.get_solutions_folder_path()
        return solutions_folder_path
    end

    -- @brief - Retrieve the path to the dependencies folder containing external dependency projects
    local external_dependencies_folder_path = join_path(self.get_engine_path(), "Dependencies")
    function self.get_external_dependencies_folder_path()
        return external_dependencies_folder_path
    end

    -- @brief - Create a path relative to dependency folder
    function self.create_external_dependency_path(path)
        return join_path(self.get_external_dependencies_folder_path(), path)
    end

    -- @brief - Retrieve a target added to the workspace
    function self.get_target(target_name)
        return self.target_rules[target_name]
    end

    -- @brief - Check if a target already exists
    function self.is_target(target_name)
        return self.get_target(target_name) ~= nil
    end

    -- @brief - Helper function for adding a target
    function self.add_target(target)
        table.insert(self.target_rules, target)
    end

    -- @brief - Helper function for adding defines
    function self.add_defines(define)
        add_unique_elements(define, self.defines)
    end

    -- @brief - Helper function for adding a rule
    function self.add_rule(rule)
        table.insert(self.project_rules, rule)
    end

    -- TODO: Better way of handling these dependencies
    -- Inject dependency projects into the workspace
    function self.generate_dependency_projects()
        local solution_location = self.get_solutions_folder_path()

        group "Dependencies"
            log_info("\n--- External Dependencies ---")
            
            -- ImGui
            project "ImGui"
                log_info("    Generating dependency ImGui")

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

                location(join_path(solution_location, "Dependencies/ImGui"))

                -- Locations
                targetdir(self.create_external_dependency_path("Build/bin/Dependencies/ImGui/" .. self.get_output_path()))
                objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/ImGui/" .. self.get_output_path()))

                -- Files
                files {
                    self.create_external_dependency_path("imgui/imconfig.h"),
                    self.create_external_dependency_path("imgui/imgui.h"),
                    self.create_external_dependency_path("imgui/imgui.cpp"),
                    self.create_external_dependency_path("imgui/imgui_demo.cpp"),
                    self.create_external_dependency_path("imgui/imgui_draw.cpp"),
                    self.create_external_dependency_path("imgui/imgui_internal.h"),
                    self.create_external_dependency_path("imgui/imgui_tables.cpp"),
                    self.create_external_dependency_path("imgui/imgui_widgets.cpp"),
                    self.create_external_dependency_path("imgui/imstb_rectpack.h"),
                    self.create_external_dependency_path("imgui/imstb_textedit.h"),
                    self.create_external_dependency_path("imgui/imstb_truetype.h"),
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
                log_info("    Generating dependency tinyobjloader")

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

                location(join_path(solution_location, "Dependencies/tinyobjloader"))

                -- Locations
                targetdir(self.create_external_dependency_path("Build/bin/Dependencies/tinyobjloader/" .. self.get_output_path()))
                objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/tinyobjloader/" .. self.get_output_path()))

                -- Files
                files {
                    self.create_external_dependency_path("tinyobjloader/tiny_obj_loader.h"),
                    self.create_external_dependency_path("tinyobjloader/tiny_obj_loader.cc"),
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
                log_info("    Generating dependency OpenFBX")

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
                
                location(join_path(solution_location, "Dependencies/OpenFBX"))
            
                -- Locations
                targetdir(self.create_external_dependency_path("Build/bin/Dependencies/OpenFBX/" .. self.get_output_path()))
                objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/OpenFBX/" .. self.get_output_path()))

                -- Files
                files {
                    self.create_external_dependency_path("OpenFBX/src/ofbx.h"),
                    self.create_external_dependency_path("OpenFBX/src/ofbx.cpp"),
                    self.create_external_dependency_path("OpenFBX/src/libdeflate.h"),
                    self.create_external_dependency_path("OpenFBX/src/libdeflate.c"),
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
                log_info("    Generating dependency SPIRV-Cross")

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
                
                location(join_path(solution_location, "Dependencies/SPIRV-Cross"))
            
                -- Locations
                targetdir(self.create_external_dependency_path("Build/bin/Dependencies/SPIRV-Cross/" .. self.get_output_path()))
                objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/SPIRV-Cross/" .. self.get_output_path()))

                -- Files
                files {
                    self.create_external_dependency_path("SPIRV-Cross/GLSL.std.450.h"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv.h"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_c.h"),

                    self.create_external_dependency_path("SPIRV-Cross/spirv.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cfg.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_common.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cpp.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_containers.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_error_handling.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_parsed_ir.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_util.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_glsl.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_hlsl.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_msl.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_parser.hpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_reflect.hpp"),

                    self.create_external_dependency_path("SPIRV-Cross/spirv_cfg.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cpp.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_c.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_parsed_ir.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_cross_util.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_glsl.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_hlsl.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_msl.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_parser.cpp"),
                    self.create_external_dependency_path("SPIRV-Cross/spirv_reflect.cpp"),
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
            group "Dependencies/glslang"
                log_info("\n    --- Generating glslang projects ---")

                -- Include directories for build-time generated include files
                local glslang_generated_includedir = join_path("build/generated/include", "glslang")

                -- GenericCodeGen
                project "GenericCodeGen"
                    log_info("        Generating dependency GenericCodeGen")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/GenericCodeGen/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/GenericCodeGen/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/GenericCodeGen/" .. self.get_output_path()))

                    -- Files
                    files {
                        self.create_external_dependency_path("glslang/glslang/GenericCodeGen/CodeGen.cpp"),
                        self.create_external_dependency_path("glslang/glslang/GenericCodeGen/Link.cpp"),
                    }

                    glslang_set_platform_properties()

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
                    log_info("        Generating dependency OSDependent")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/OSDependent/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/OSDependent/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/OSDependent/" .. self.get_output_path()))

                    -- Files
                    files {
                        self.create_external_dependency_path("glslang/glslang/OSDependent/osinclude.h"),
                    }

                    filter "system:windows"
                        files {
                            self.create_external_dependency_path("glslang/glslang/OSDependent/Windows/ossource.cpp"),
                        }
                    filter "system:macosx"
                        files {
                            self.create_external_dependency_path("glslang/glslang/OSDependent/Unix/ossource.cpp"),
                        }
                    filter {}

                    glslang_set_platform_properties()

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
                    log_info("        Generating dependency MachineIndependent")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/MachineIndependent/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/MachineIndependent/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/MachineIndependent/" .. self.get_output_path()))

                    -- Include Directories
                    includedirs {
                        self.create_external_dependency_path("glslang/glslang/include")
                    }

                    -- Files
                    files {
                        -- Cpp files
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/glslang.y"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/glslang_tab.cpp"),

                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/attribute.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Constant.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/InfoSink.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Initialize.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/intermOut.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/IntermTraverse.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/iomapper.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Intermediate.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/limits.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/linkValidate.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/parseConst.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/ParseContextBase.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/ParseHelper.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/PoolAlloc.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/propagateNoContraction.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/reflection.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/RemoveTree.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Scan.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/ShaderLang.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/SymbolTable.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Versions.cpp"),

                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/Pp.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp"),

                        -- Header Files
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/glslang_tab.cpp.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/gl_types.h"),

                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/attribute.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Initialize.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/iomapper.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/LiveTraverser.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/localintermediate.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/ParseHelper.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/parseVersions.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/propagateNoContraction.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/reflection.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/RemoveTree.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Scan.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/ScanContext.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/span.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/SymbolTable.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/Versions.h"),

                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/PpContext.h"),
                        self.create_external_dependency_path("glslang/glslang/MachineIndependent/preprocessor/PpTokens.h"),
                    }

                    glslang_set_platform_properties()
                    glslang_generate_build_time_headers()

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
                    log_info("        Generating dependency glslang")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/glslang/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/glslang/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/glslang/" .. self.get_output_path()))

                    -- Include Directories
                    includedirs {
                        self.create_external_dependency_path("glslang")
                    }

                    -- Files
                    files {
                        -- Cpp
                        self.create_external_dependency_path("glslang/glslang/CInterface/glslang_c_interface.cpp"),

                        -- Header
                        self.create_external_dependency_path("glslang/glslang/Public/ShaderLang.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/arrays.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/BaseTypes.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/Common.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/ConstantUnion.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/glslang_c_interface.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/glslang_c_shader_types.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/InfoSink.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/InitializeGlobals.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/intermediate.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/PoolAlloc.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/ResourceLimits.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/ShHandle.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/SpirvIntrinsics.h"),
                        self.create_external_dependency_path("glslang/glslang/Include/Types.h"),
                    }

                    -- Links
                    links {
                        "OSDependent",
                        "MachineIndependent",
                    }

                    glslang_set_platform_properties()

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
                    log_info("        Generating dependency glslang-default-resource-limits")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/glslang-default-resource-limits/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/glslang-default-resource-limits/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/glslang-default-resource-limits/" .. self.get_output_path()))

                    -- Include Directories
                    includedirs {
                        self.create_external_dependency_path("glslang")
                    }
                    
                    -- Files
                    files {
                        -- Cpp
                        self.create_external_dependency_path("glslang/glslang/ResourceLimits/ResourceLimits.cpp"),
                        self.create_external_dependency_path("glslang/glslang/ResourceLimits/resource_limits_c.cpp"),

                        -- Header
                        self.create_external_dependency_path("glslang/glslang/Public/ResourceLimits.h"),
                        self.create_external_dependency_path("glslang/glslang/Public/resource_limits_c.h"),
                    }

                    glslang_set_platform_properties()

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
                    log_info("        Generating dependency SPIRV")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/SPIRV/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/SPIRV/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/SPIRV/" .. self.get_output_path()))

                    -- Include Directories
                    includedirs {
                        self.create_external_dependency_path("glslang"),
                        self.create_external_dependency_path("glslang/glslang/include"),
                    }

                    -- Files
                    files {
                        -- Cpp
                        self.create_external_dependency_path("glslang/SPIRV/GlslangToSpv.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/InReadableOrder.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/Logger.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/SpvBuilder.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/SpvPostProcess.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/doc.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/SpvTools.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/disassemble.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/CInterface/spirv_c_interface.cpp"),

                        -- Headers
                        self.create_external_dependency_path("glslang/SPIRV/bitutils.h"),
                        self.create_external_dependency_path("glslang/SPIRV/spirv.hpp"),
                        self.create_external_dependency_path("glslang/SPIRV/GLSL.std.450.h"),
                        self.create_external_dependency_path("glslang/SPIRV/GLSL.ext.EXT.h"),
                        self.create_external_dependency_path("glslang/SPIRV/GLSL.ext.KHR.h"),
                        self.create_external_dependency_path("glslang/SPIRV/GlslangToSpv.h"),
                        self.create_external_dependency_path("glslang/SPIRV/hex_float.h"),
                        self.create_external_dependency_path("glslang/SPIRV/Logger.h"),
                        self.create_external_dependency_path("glslang/SPIRV/SpvBuilder.h"),
                        self.create_external_dependency_path("glslang/SPIRV/spvIR.h"),
                        self.create_external_dependency_path("glslang/SPIRV/doc.h"),
                        self.create_external_dependency_path("glslang/SPIRV/SpvTools.h"),
                        self.create_external_dependency_path("glslang/SPIRV/disassemble.h"),
                        self.create_external_dependency_path("glslang/SPIRV/GLSL.ext.AMD.h"),
                        self.create_external_dependency_path("glslang/SPIRV/GLSL.ext.NV.h"),
                        self.create_external_dependency_path("glslang/SPIRV/GLSL.ext.ARM.h"),
                        self.create_external_dependency_path("glslang/SPIRV/NonSemanticDebugPrintf.h"),
                        self.create_external_dependency_path("glslang/SPIRV/NonSemanticShaderDebugInfo100.h"),
                    }

                    -- Links
                    links {
                        "MachineIndependent",
                    }

                    glslang_set_platform_properties()

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
                    log_info("        Generating dependency SPVRemapper")

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
                    
                    location(join_path(solution_location, "Dependencies/glslang/SPVRemapper/"))
                
                    -- Locations
                    targetdir(self.create_external_dependency_path("Build/bin/Dependencies/glslang/SPVRemapper/" .. self.get_output_path()))
                    objdir(self.create_external_dependency_path("Build/bin-int/Dependencies/glslang/SPVRemapper/" .. self.get_output_path()))

                    -- Files
                    files {
                        self.create_external_dependency_path("glslang/SPIRV/SPVRemapper.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/doc.cpp"),
                        self.create_external_dependency_path("glslang/SPIRV/SPVRemapper.h"),
                        self.create_external_dependency_path("glslang/SPIRV/doc.h"),
                    }

                    glslang_set_platform_properties()

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
    function self.generate_solution_files()
        log_info("\n--- Generating Solution Files for Workspace '%s' ---", self.name)

        -- Set the name of the workspace
        workspace(self.name)

        -- Set location of the generated solution file
        local solution_location = self.get_solutions_folder_path()
        location(solution_location)

        log_info("    Generated solution location '%s'", solution_location)

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
        local runtime_folder_path = self.get_runtime_folder_path()
        includedirs { runtime_folder_path }

        -- Workspace defines
        log_info("\n--- Workspace Defines (Num Defines=%d) ---", #self.defines)
        if #self.defines > 0 then
            print_table("    Using Define '%s'", self.defines)
        else
            log_info("")
        end

        defines(self.defines)

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
        log_info("    StartProject = '%s'", self.start_project_name)
        startproject(self.start_project_name)

        -- Generate projects for all dependencies
        self.generate_dependency_projects()

        -- Generate project files for all the rules that have been added
        log_info("\n--- Generating module and target project files ---")
        for _, current_rule in ipairs(self.project_rules) do
            current_rule.generate_project()
        end
    end

    -- Generate workspace
    function self.generate()
        log_info("\n--- Generating Workspace '%s' ---", self.name)
        log_info("OutputPath = '%s'", self.get_output_path())

        if self.target_rules == nil then
            log_error("TargetRules cannot be nil")
            return
        end

        if #self.target_rules < 1 then
            log_error("Workspace must contain at least one build rule (Current=%d)", #self.target_rules)
            return
        end

        -- Define the workspace location; we do this with a Unix path since the engine (C++ side) expects this currently
        local unix_engine_path = path.translate(self.get_engine_path(), "/")
        local engine_location = 'ENGINE_LOCATION="' .. unix_engine_path .. '"'
        self.add_defines { engine_location }
        
        log_info("    Engine Path ='%s'", self.get_engine_path())
        log_info("    RuntimeFolderPath = '%s'", self.get_runtime_folder_path())
        
        -- Check if the command line overrides monolithic builds
        if global_is_monolithic() then
            self.add_defines { "MONOLITHIC_BUILD=(1)" }
        end

        -- IDE Defines
        if build_with_visual_studio() then 
            self.add_defines { "IDE_VISUAL_STUDIO" }
            self.add_defines { "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" }
            self.add_defines { "_CRT_SECURE_NO_WARNINGS" }
        end

        -- OS Defines
        if is_platform_windows() then
            self.add_defines { "PLATFORM_WINDOWS=(1)" }
        end
        if is_platform_mac() then
            self.add_defines { "PLATFORM_MACOS=(1)" }
        end

        -- Setup startup project
        local start_project_target = self.target_rules[1]
        if (start_project_target.target_type == ETargetType.Client) and (not start_project_target.is_monolithic) then
            self.start_project_name = start_project_target.name .. "Standalone"
        else
            self.start_project_name = start_project_target.name
        end
        
        -- Generate projects from targets
        log_info("\n--- Generating Targets (NumTargets=%d) ---", #self.target_rules)
        for _, current_target in ipairs(self.target_rules) do
            self.target_name = current_target.name
            current_target.workspace = self
            current_target.generate()
        end

        -- Generate the actual solution files
        self.generate_solution_files()

        log_info("\n--- Finished generating workspace ---")
    end

    return self
end
