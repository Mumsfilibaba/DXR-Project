include "build_common.lua"

-- Build rules for a project
function build_rules(name)

    -- Needs to have a valid module name
    if name == nil then
        log_error("BuildRule failed due to invalid name")
        return nil
    end

    log_highlight("Creating BuildRule '%s'", name)

    -- Folder path for engine modules
    local runtime_folder_path = get_runtime_folder_path()

    -- Initialize public members
    local self = 
    {
        -- @brief - Name. Must be the name of the folder as well or specify the location
        name = name,

        -- @brief - Location for IDE project files
        project_file_path = "",

        -- @brief - Location for generated files from the build
        build_folder_path = "",

        -- @brief - Location for the build (inside the build folder specified inside the build-folder)
        output_path = "",

        -- @brief - The workspace that this rule is currently a part of
        workspace = {},

        -- @brief - Should use precompiled headers. Should be named Precompiled.h and Precompiled.cpp
        use_precompiled_headers = false,

        -- @brief - Set to true if C++ files (.cpp) should be compiled as Objective-C++ (.mm), making compilation for all files native to the iOS and Mac platform
        compile_cpp_as_objective_cpp = true,

        -- @brief - Enable runtime type information
        enable_runtime_type_info = false,

        -- @brief - Enable Edit and Continue in Visual Studio
        enable_edit_and_continue = false,

        -- @brief - Enable C++ intrinsics
        enable_intrinsics = true,

        -- @brief - Architecture to compile for
        architecture = "x86_64",

        -- @brief - Warning level to compile with
        warnings = "extra",

        -- @brief - How to handle C++ exceptions
        exception_handling = "Off",

        -- @brief - Floating point settings
        floating_point = "Fast",

        -- @brief - Enable vector extensions
        vector_extensions = "AVX2",

        -- @brief - Language to compile
        language = "C++",

        -- @brief - Language version to compile
        cpp_version = "C++20",

        -- @brief - Version of system SDK
        system_version = "latest",

        -- @brief - ASCII or Unicode
        character_set = "Ascii",

        -- @brief - Premake flags
        flags =
        {
            "MultiProcessorCompile",
            "NoIncrementalLink",
        },

        -- @brief - The kind of project to generate (SharedLib, StaticLib, WindowedApp, ConsoleApp, etc.)
        kind = "SharedLib",

        -- @brief - System includes, e.g., #include <vector>
        system_includes = {},

        -- @brief - Force include these files
        force_includes = {},

        -- @brief - Paths to search library files in
        library_paths = {},

        -- @brief - Files to compile into the module
        files = 
        {
            "**.h",
            "**.hpp",
            "**.inl",
            "**.c",
            "**.cpp",
            "**.hlsl",
            "**.hlsli"
        },

        -- @brief - Files to exclude
        exclude_files = 
        {
            "**.hlsl",
            "**.hlsli"
        },

        -- @brief - Defines
        defines = {},

        -- @brief - Frameworks, only on macOS for now; should only list the names, not .framework
        frameworks = {},

        -- @brief - Should the libraries be embedded into the executable (this only applies to macOS at the moment)
        embed_dependencies = false,

        -- @brief - Extra names to embed (this only applies to macOS at the moment)
        extra_embed_names = {},

        -- @brief - Engine modules that this module depends on
        module_dependencies = {},

        -- @brief - Extra libraries to link
        link_libraries = {},

        -- @brief - A list of dependencies that a module depends on; ensures that the IDE builds all the projects
        link_modules = {},

        -- @brief - A list of link options (ignored on platforms other than Windows)
        link_options = {}
    }

    -- Helper function for retrieving path
    local build_rule_path = join_path(runtime_folder_path, self.name)
    function self.get_path()
        return build_rule_path
    end

    -- Helper functions for adding elements
    function self.add_flags(in_flags)
        add_unique_elements(in_flags, self.flags)
    end

    function self.add_system_includes(in_system_includes)
        add_unique_elements(in_system_includes, self.system_includes)
    end

    function self.add_includes(in_includes)
        add_unique_elements(in_includes, self.includes)
    end

    function self.add_files(in_files)
        add_unique_elements(in_files, self.files)
    end

    function self.add_exclude_files(in_exclude_files)
        add_unique_elements(in_exclude_files, self.exclude_files)
    end

    function self.add_defines(in_defines)
        add_unique_elements(in_defines, self.defines)
    end

    function self.add_module_dependencies(in_module_dependencies)
        add_unique_elements(in_module_dependencies, self.module_dependencies)
    end

    function self.add_extra_embed_names(in_extra_embed_names)
        add_unique_elements(in_extra_embed_names, self.extra_embed_names)
    end

    function self.add_link_libraries(in_libraries)
        add_unique_elements(in_libraries, self.link_libraries)
    end

    function self.add_frameworks(in_frameworks)
        add_unique_elements(in_frameworks, self.frameworks)
    end

    function self.add_force_includes(in_force_includes)
        add_unique_elements(in_force_includes, self.force_includes)
    end

    function self.add_library_paths(in_library_paths)
        add_unique_elements(in_library_paths, self.library_paths)
    end

    function self.add_link_options(in_options)
        add_unique_elements(in_options, self.link_options)
    end

    -- Helper for adding the .framework extension to frameworks
    function self.add_framework_extension()
        for index = 1, #self.frameworks do
            self.frameworks[index] = self.frameworks[index] .. ".framework"
        end
    end

    -- Makes all files relative to runtime folder
    function self.make_file_names_relative_to_path(file_array)
        for index = 1, #file_array do
            local current_file = file_array[index]
            log_info(" -make_file_names_relative_to_path %s", current_file)

            if not path.isabsolute(file_array[index]) then
                file_array[index] = join_path(self.get_path(), file_array[index])
            end
        end
    end

    -- Project generation
    function self.generate_project()
        project(self.name)
            log_highlight("\n--- Generating project files for Project '%s' ---", self.name)

            architecture(self.architecture)
            warnings(self.warnings)
            exceptionhandling(self.exception_handling)

            -- Build type
            kind(self.kind)

            -- Setup runtime type information
            if self.enable_runtime_type_info then
                rtti("On")
            else
                rtti("Off")
            end

            floatingpoint(self.floating_point)
            vectorextensions(self.vector_extensions)

            -- Setup Edit and Continue
            if self.enable_edit_and_continue then
                editandcontinue("On")
            else
                editandcontinue("Off")
            end

            -- Setup intrinsics
            if self.enable_intrinsics then
                intrinsics("On")
            else
                intrinsics("Off")
            end

            -- Setup language
            local current_language = self.language:upper()
            if current_language ~= "C++" then
                log_error("Invalid language '%s'", self.language)
                return nil
            end

            language(self.language)

            -- Setup version
            local current_language_version = self.cpp_version:lower()
            if not verify_language_version(current_language_version) then
                log_error("Invalid language version '%s'", self.cpp_version)
                return nil
            end

            cppdialect(self.cpp_version)

            -- Add the /Zc:__cplusplus switch, otherwise __cplusplus is not defined properly
            filter "action:vs*"
                buildoptions { "/Zc:__cplusplus" }
            filter {}

            -- Setup system version
            systemversion(self.system_version)

            -- Setup character set
            local current_character_set = self.character_set:lower()
            if current_character_set ~= "ascii" and current_character_set ~= "unicode" then
                log_error("Invalid character set '%s'", self.character_set)
                return nil
            end

            characterset(self.character_set)

            -- Setup location
            self.project_file_path = create_os_path(self.project_file_path)
            log_info("    Project location '%s'", self.project_file_path)
            location(self.project_file_path)

            -- Setup all targets except the dependencies
            local full_object_folder_path = join_path(join_path(self.build_folder_path, "bin"), self.output_path)
            log_info("    Target location '%s'", full_object_folder_path)
            targetdir(full_object_folder_path)

            local full_intermediate_folder_path = join_path(join_path(self.build_folder_path, "bin-int"), self.output_path)
            log_info("    Object files location '%s'", full_intermediate_folder_path)
            objdir(full_intermediate_folder_path)

            -- Setup precompiled headers
            if self.use_precompiled_headers then
                if build_with_visual_studio() then
                    -- Specify the full path for everything to work properly on Windows
                    local pch_source_path = join_path(self.get_path(), "PreCompiled.cpp")
                    log_highlight("    PreCompiled source path '%s'", pch_source_path)

                    -- Use the Unix path (this is probably an internal Premake thing)
                    local unix_pch_source_path = path.translate(pch_source_path, '/')
                    pchheader("PreCompiled.h")
                    pchsource(unix_pch_source_path)
                else
                    -- Specify the full path for everything to work properly on non-Windows
                    local pch_path = join_path(self.get_path(), "PreCompiled.h")
                    pchheader(pch_path)
                end

                log_info("    Project is using PreCompiled Headers")
            else
                log_info("    Project does NOT use PreCompiled Headers")
            end

            -- Debug logging
            log_info("\n--- ForceIncludes for module '%s' (Num ForceIncludes=%d) ---", self.name, #self.force_includes)
            if #self.force_includes > 0 then
                print_table("    Using ForceInclude '%s'", self.force_includes)
            end

            log_info("\n--- Defines for module '%s' (Num Defines=%d) ---", self.name, #self.defines)
            if #self.defines > 0 then
                print_table("    Using define '%s'", self.defines)
            end

            log_info("\n--- SystemIncludes for module '%s' (Num SystemIncludes=%d) ---", self.name, #self.system_includes)
            if #self.system_includes > 0 then
                print_table("    Using SystemInclude '%s'", self.system_includes)
            end

            log_info("\n--- LibraryPaths for module '%s' (Num LibraryPaths=%d) ---", self.name, #self.library_paths)
            if #self.library_paths > 0 then
                print_table("    Using LibraryPath '%s'", self.library_paths)
            end

            log_info("\n--- Files for module '%s' (Num Files=%d) ---", self.name, #self.files)
            if #self.files > 0 then
                print_table("    Including file '%s'", self.files)
            end

            log_info("\n--- Exclude files for module '%s' (Num ExcludeFiles=%d) ---", self.name, #self.exclude_files)
            if #self.exclude_files > 0 then
                print_table("    Excluding file '%s'", self.exclude_files)
            end

            log_info("\n--- Frameworks for module '%s' (Num Frameworks=%d) ---", self.name, #self.frameworks)
            if #self.frameworks > 0 then
                print_table("    Using framework dependency '%s'", self.frameworks)
            end

            log_info("\n--- LinkLibraries for module '%s' (Num LinkLibraries=%d) ---", self.name, #self.link_libraries)
            if #self.link_libraries > 0 then
                print_table("    Linking library '%s'", self.link_libraries)
            end

            log_info("\n--- Link modules for module '%s' (Num LinkModules=%d) ---", self.name, #self.link_modules)
            if #self.link_modules > 0 then
                print_table("    Linking module '%s'", self.link_modules)
            end

            log_info("\n--- Link options for module '%s' (Num LinkOptions=%d) ---", self.name, #self.link_options)
            if #self.link_options > 0 then
                print_table("    Link options '%s'", self.link_options)
            end

            log_info("\n--- Module dependencies for module '%s' (Num ModuleDependencies=%d) ---", self.name, #self.module_dependencies)
            if #self.module_dependencies > 0 then
                print_table("    Using module dependency '%s'", self.module_dependencies)
            end

            log_info("\n--- Embedded modules for module '%s' (Num Embedded Modules=%d) ---", self.name, #self.module_dependencies)
            if #self.module_dependencies > 0 then
                print_table("    Embed Module '%s'", self.module_dependencies)
            end

            -- Setup force includes
            forceincludes(self.force_includes)

            defines(self.defines)

            externalincludedirs(self.system_includes)

            libdirs(self.library_paths)

            files(self.files)

            -- Setup exclude OS-specific files
            if is_platform_windows() then
                filter { "files:**/Mac/**.cpp" }
                    flags { "ExcludeFromBuild" }
                filter {}
            elseif is_platform_mac() then
                filter { "files:**/Windows/**.cpp" }
                    flags { "ExcludeFromBuild" }
                filter {}

                -- On macOS, compile all .cpp files as Objective-C++ to avoid pre-processor checks
                if self.compile_cpp_as_objective_cpp then
                    filter { "files:**.cpp" }
                        compileas("Objective-C++")
                    filter {}
                end
            end

            -- In Visual Studio, show .natvis files
            if build_with_visual_studio() then
                vpaths { ["Natvis"] = "**.natvis" }

                local natvis_path = join_path(self.get_path(), "**.natvis")
                log_highlight("NatvisPath='%s'", natvis_path)

                files {
                    natvis_path
                }
            end

            -- Remove files
            removefiles(self.exclude_files)

            -- Setup linking
            if is_platform_mac() then
                -- Ignore linking when kind is set to 'None'
                if self.kind == "None" then
                    log_warning("Ignoring Frameworks due to the kind being set to 'None'")
                else
                    links(self.frameworks)
                end
            end

            -- Ignore linking and dependencies when kind is set to 'None'
            if self.kind == "None" then
                log_warning("Ignoring LinkLibraries due to the kind being set to 'None'")
                log_warning("Ignoring LinkModules due to the kind being set to 'None'")
                log_warning("Ignoring LinkOptions due to the kind being set to 'None'")
                log_warning("Ignoring Dependencies due to the kind being set to 'None'")
            else
                -- Link libraries (external libraries, etc.)
                links(self.link_libraries)
                links(self.link_modules)
                linkoptions(self.link_options)
                -- Setup dependencies
                dependson(self.module_dependencies)
            end

            -- Setup embedded frameworks, etc.
            filter { "action:xcode4" }
                if self.embed_dependencies then
                    -- Embed modules and extra embed names
                    embed(self.module_dependencies)
                    embed(self.extra_embed_names)
                end
            filter {}

            -- Xcode build settings
            filter { "action:xcode4" }
                xcodebuildsettings 
                {
                    ["PRODUCT_BUNDLE_IDENTIFIER"]  = "com.DXREngine." .. self.name,
                    ["CODE_SIGN_STYLE"]            = "Automatic",
                    ["ARCHS"]                      = "x86_64",
                    ["ONLY_ACTIVE_ARCH"]           = "YES",
                    ["ENABLE_HARDENED_RUNTIME"]    = "NO",
                    ["GENERATE_INFOPLIST_FILE"]    = "YES",
                    ["LD_RUNPATH_SEARCH_PATHS"]    = "/usr/local/lib/ $(INSTALL_PATH) @executable_path/../Frameworks",
                    ["GCC_ENABLE_AVX2_EXTENSIONS"] = "YES",
                }
            filter {}

            -- Copy dynamic libraries from dependencies folder
            if is_platform_windows() then
                local dxil_dll_cmd = "copy " .. create_external_dependency_path("DXC/bin/dxil.dll") .. " " .. full_object_folder_path
                log_highlight("dxil.dll Cmd %s", dxil_dll_cmd)

                local dxcompiler_dll_cmd = "copy " .. create_external_dependency_path("DXC/bin/dxcompiler.dll") .. " " .. full_object_folder_path
                log_highlight("dxcompiler.dll Cmd %s", dxcompiler_dll_cmd)

                postbuildcommands
                {
                    dxil_dll_cmd,
                    dxcompiler_dll_cmd
                }
            elseif is_platform_mac() then
                local libdxcompiler_dll_cmd = "cp " .. create_external_dependency_path("DXC/bin/libdxcompiler.dylib") .. " " .. full_object_folder_path
                log_highlight("libdxcompiler.dylib Cmd %s", libdxcompiler_dll_cmd)

                postbuildcommands
                {
                    libdxcompiler_dll_cmd
                }
            end
        project "*"

        log_highlight("\n--- Finished generating project files for Project '%s' ---", self.name)
    end

    -- Base generate (generates project files)
    function self.generate()
        if self.workspace == nil then
            log_error("Workspace cannot be nil when generating Rule")
            return
        end

        -- Ensure dependencies are included
        for index = 1, #self.module_dependencies do
            log_highlight("\n--- Including dependency for project '%s' ---", self.name)

            local current_module_name = self.module_dependencies[index]
            if is_module(current_module_name) then
                log_highlight_warning("-Dependency '%s' is already included", current_module_name)
            else
                local dependency_path = join_path(join_path(runtime_folder_path, current_module_name), "Module.lua")
                log_info("-Including Dependency '%s' Path='%s'", current_module_name, dependency_path)
                include(dependency_path)

                -- Generate module, but check so that it exists since some platforms do not create certain modules (D3D12RHI, MetalRHI, etc.)
                if is_module(current_module_name) then
                    local current_module = get_module(current_module_name)
                    current_module.workspace = self.workspace
                    current_module.generate()
                else
                    log_warning("Failed to properly create module '%s'", current_module_name)
                end
            end
        end

        -- Setup folder paths
        self.build_folder_path = self.workspace.get_build_folder_path()
        self.output_path       = self.workspace.get_output_path()
        self.project_file_path = self.workspace.get_solutions_folder_path()

        -- Ensure that the runtime folder is added to the include folders
        self.add_system_includes { runtime_folder_path }

        -- Add framework extension
        self.add_framework_extension()

        -- Solve dependencies
        for index = 1, #self.module_dependencies do
            local current_module_name = self.module_dependencies[index]
            local current_module      = get_module(current_module_name)

            if current_module then
                if not current_module.runtime_linking then
                    table.insert(self.link_modules, current_module_name)
                end

                if current_module.is_dynamic then
                    local module_api_name = current_module.name:upper() .. "_API"

                    -- This should be linked at compile time
                    if not current_module.runtime_linking then
                        module_api_name = module_api_name .. "=MODULE_IMPORT"
                    end

                    self.add_defines { module_api_name }
                end

                -- TODO: This should probably be separated into public/private dependencies since public should always be pushed up
                -- We always want to add the frameworks and modules as a dependency
                self.add_link_libraries(current_module.link_libraries)
                self.add_frameworks(current_module.frameworks)
                self.add_module_dependencies(current_module.module_dependencies)

                -- System includes can be included in a dependency header and therefore necessary in this module as well
                self.add_system_includes(current_module.system_includes)
                self.add_library_paths(current_module.library_paths)
            else
                log_error("Module '%s' has not been included", current_module_name)
            end
        end

        -- Add link options
        if build_with_visual_studio() then
            -- TODO: We only want this for monolithic builds
            for index = 1, #self.link_modules do
                local current_module_name = self.link_modules[index]
                if current_module_name ~= "Launch" then
                    self.add_link_options { "/INCLUDE:LinkModule_" .. current_module_name }
                end
            end
        end

        -- Setup precompiled headers
        if self.use_precompiled_headers then
            self.add_force_includes { "PreCompiled.h" }
        end

        -- Make files relative before printing
        self.make_file_names_relative_to_path(self.files)
        self.make_file_names_relative_to_path(self.exclude_files)

        -- Add this rule to the workspace
        self.workspace.add_rule(self)
    end

    return self
end
