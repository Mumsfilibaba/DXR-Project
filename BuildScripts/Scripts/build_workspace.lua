include 'build_module.lua'
include 'build_target.lua'

-- Generate a workspace from an array of target-rules
function GenerateWorkspace(WorkspaceName, TargetRules)
    printf('---Generating workspace \'%s\'', WorkspaceName)
    
    -- Early returns
    if TargetRules == nil then
        printf('TargetRules cannot be nil')
        return
    end

    if #TargetRules < 1 then
        printf('TargetRules must contain atleast one buildrule (Current=%d)', #TargetRules)
        return
    end

    -- Generate workspace
    workspace(WorkspaceName)

    -- Set location of the generated solution file
    local SolutionLocation = GetSolutionsFolderPath()
    printf('    Generated solution location \'%s\'\n', SolutionLocation)
    location(SolutionLocation)

    -- Platforms
    platforms
    {
        'x64',
    }

    -- Configurations
    configurations
    {
        'Debug',
        'Release',
        'Production',
    }

    -- Define the workspace location
    local EngineLocation = 'ENGINE_LOCATION=' .. '\"' .. GetEnginePath() .. '\"'
    printf('    ENGINE_LOCATION = \'%s\'', EngineLocation)
    defines
    {
        EngineLocation
    }

    -- Includes
    local RuntimeFolderPath = GetRuntimeFolderPath()
    printf('    RuntimeFolderPath = \'%s\'', RuntimeFolderPath)
    includedirs
    {
        RuntimeFolderPath,
    }

    filter 'options:monolithic'
        defines
        {
            'MONOLITHIC_BUILD=(1)'
        }
    filter {}

    filter 'configurations:Debug'
        symbols      'on'
        runtime      'Debug'
        optimize     'Off'
        architecture 'x86_64'
        defines
        {
            '_DEBUG',
            'DEBUG_BUILD=(1)',
        }
    filter {}

    filter 'configurations:Release'
        symbols      'on'
        runtime      'Release'
        optimize     'Full'
        architecture 'x86_64'
        defines
        {
            'NDEBUG',
            'RELEASE_BUILD=(1)',
        }
    filter {}

    filter 'configurations:Production'
        symbols      'off'
        runtime      'Release'
        optimize     'Full'
        architecture 'x86_64'
        defines
        {
            'NDEBUG',
            'PRODUCTION_BUILD=(1)',
        }
    filter {}
    
    -- Architecture defines
    filter 'architecture:x86'
        defines
        {
            'ARCHITECTURE_X86=(1)',
        }
    filter {}

    filter 'architecture:x86_x64'
        defines
        {
            'ARCHITECTURE_X86_X64=(1)',
        }
    filter {}

    filter 'architecture:ARM'
        defines
        {
            'ARCHITECTURE_ARM=(1)',
        }
    filter {}

    -- IDE options
    filter 'action:vs*'
        defines
        {
            'IDE_VISUAL_STUDIO',
            '_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING',
            '_CRT_SECURE_NO_WARNINGS',
        }
    filter {}

    -- OS
    filter 'system:windows'
        defines
        {
            'PLATFORM_WINDOWS=(1)',
        }
    filter {}

    filter 'system:macosx'
        defines
        {
            'PLATFORM_MACOS=(1)',
        }
    filter {}
    
    -- TODO: Better way of handling these dependencies
    -- Generate external dependeny projects 
    local ExternalDependecyPath = GetExternalDependenciesFolderPath()
    local SolutionsFolderPath   = GetSolutionsFolderPath()
    group 'Dependencies'
        printf('\n    ---External Dependencies---')
        
        -- Imgui
        project 'ImGui'
            printf('    Generating dependecy ImGui')

            kind('StaticLib')
            warnings('Off')
            intrinsics('On')
            editandcontinue('Off')
            language('C++')
            cppdialect('C++17')
            systemversion('latest')
            architecture('x64')
            exceptionhandling('Off')
            rtti('Off')
            floatingpoint('Fast')
            vectorextensions('SSE2')
            characterset('Ascii')
            flags(
            { 
                'MultiProcessorCompile',
                'NoIncrementalLink',
            })

            location(SolutionsFolderPath .. '/Dependencies/ImGui')

            -- Locations
            targetdir(ExternalDependecyPath .. '/Build/bin/ImGui/' .. GetOutputPath())
            objdir(ExternalDependecyPath .. '/Build/bin-int/ImGui/' .. GetOutputPath())

            -- Files
            files
            {
                (ExternalDependecyPath .. '/imgui/imconfig.h'),
                (ExternalDependecyPath .. '/imgui/imgui.h'),
                (ExternalDependecyPath .. '/imgui/imgui.cpp'),
                (ExternalDependecyPath .. '/imgui/imgui_draw.cpp'),
                (ExternalDependecyPath .. '/imgui/imgui_demo.cpp'),
                (ExternalDependecyPath .. '/imgui/imgui_internal.h'),
                (ExternalDependecyPath .. '/imgui/imgui_tables.cpp'),
                (ExternalDependecyPath .. '/imgui/imgui_widgets.cpp'),
                (ExternalDependecyPath .. '/imgui/imstb_rectpack.h'),
                (ExternalDependecyPath .. '/imgui/imstb_textedit.h'),
                (ExternalDependecyPath .. '/imgui/imstb_truetype.h'),
            }
            
            -- Configurations
            filter 'configurations:Debug or Release'
                symbols  'on'
                runtime  'Release'
                optimize 'Full'
            filter {}
            
            filter 'configurations:Production'
                symbols  'off'
                runtime  'Release'
                optimize 'Full'
            filter {}
        
        -- tinyobjloader Project
        project 'tinyobjloader'
            printf('    Generating dependecy tinyobjloader')

            kind('StaticLib')
            warnings('Off')
            intrinsics('On')
            editandcontinue('Off')
            language('C++')
            cppdialect('C++17')
            systemversion('latest')
            architecture('x64')
            exceptionhandling('Off')
            rtti('Off')
            floatingpoint('Fast')
            vectorextensions('SSE2')
            characterset('Ascii')
            flags(
            { 
                'MultiProcessorCompile',
                'NoIncrementalLink',
            })

            location (SolutionsFolderPath .. '/Dependencies/tinyobjloader')

            -- Locations
            targetdir(ExternalDependecyPath .. '/Build/bin/tinyobjloader/' .. GetOutputPath())
            objdir(ExternalDependecyPath .. '/Build/bin-int/tinyobjloader/' .. GetOutputPath())

            -- Files
            files 
            {
                (ExternalDependecyPath .. '/tinyobjloader/tiny_obj_loader.h'),
                (ExternalDependecyPath .. '/tinyobjloader/tiny_obj_loader.cc'),
            }

            -- Configurations
            filter 'configurations:Debug or Release'
                symbols  'on'
                runtime  'Release'
                optimize 'Full'
            filter {}

            filter 'configurations:Production'
                symbols  'off'
                runtime  'Release'
                optimize 'Full'    
            filter {}
        
        -- OpenFBX Project
        project 'OpenFBX'
            printf('    Generating dependecy OpenFBX')

            kind('StaticLib')
            warnings('Off')
            intrinsics('On')
            editandcontinue('Off')
            language('C++')
            cppdialect('C++17')
            systemversion('latest')
            architecture('x64')
            exceptionhandling('Off')
            rtti('Off')
            floatingpoint('Fast')
            vectorextensions('SSE2')
            characterset('Ascii')
            flags(
            { 
                'MultiProcessorCompile',
                'NoIncrementalLink',
            })
            
            location (SolutionsFolderPath .. '/Dependencies/OpenFBX')
        
            -- Locations
            targetdir(ExternalDependecyPath .. '/Build/bin/OpenFBX/' .. GetOutputPath())
            objdir(ExternalDependecyPath .. '/Build/bin-int/OpenFBX/' .. GetOutputPath())

            -- Files
            files 
            {
                (ExternalDependecyPath .. '/OpenFBX/src/ofbx.h'),
                (ExternalDependecyPath .. '/OpenFBX/src/ofbx.cpp'),
                (ExternalDependecyPath .. '/OpenFBX/src/miniz.h'),
                (ExternalDependecyPath .. '/OpenFBX/src/miniz.c'),
            }

            -- Configurations 
            filter 'configurations:Debug or Release'
                symbols  'on'
                runtime  'Release'
                optimize 'Full'
            filter {}
            
            filter 'configurations:Production'
                symbols  'off'
                runtime  'Release'
                optimize 'Full'
            filter {}

        -- SPIRV-Cross Project
        project 'SPIRV-Cross'
            printf('    Generating dependecy SPIRV-Cross')

            kind('StaticLib')
            warnings('Off')
            intrinsics('On')
            editandcontinue('Off')
            language('C++')
            cppdialect('C++17')
            systemversion('latest')
            architecture('x64')
            exceptionhandling('Off')
            rtti('Off')
            floatingpoint('Fast')
            vectorextensions('SSE2')
            characterset('Ascii')
            flags(
            { 
                'MultiProcessorCompile',
                'NoIncrementalLink',
            })
            
            location(SolutionsFolderPath .. '/Dependencies/SPIRV-Cross')
        
            -- Locations
            targetdir(ExternalDependecyPath .. '/Build/bin/SPIRV-Cross/' .. GetOutputPath())
            objdir(ExternalDependecyPath .. '/Build/bin-int/SPIRV-Cross/' .. GetOutputPath())

            -- Files
            files 
            {
                (ExternalDependecyPath .. '/SPIRV-Cross/GLSL.std.450.h'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv.h'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_c.h'),

                (ExternalDependecyPath .. '/SPIRV-Cross/spirv.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cfg.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_common.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cpp.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_containers.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_error_handling.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_parsed_ir.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_util.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_glsl.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_hlsl.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_msl.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_parser.hpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_reflect.hpp'),

                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cfg.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cpp.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_c.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_parsed_ir.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_cross_util.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_glsl.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_hlsl.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_msl.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_parser.cpp'),
                (ExternalDependecyPath .. '/SPIRV-Cross/spirv_reflect.cpp'),
            }

            -- Defines 
            defines
            {
                'SPIRV_CROSS_C_API_MSL=(1)',
                'SPIRV_CROSS_C_API_HLSL=(1)',
                'SPIRV_CROSS_C_API_GLSL=(1)',
            }

            -- Configurations 
            filter 'configurations:Debug or Release'
                symbols  'on'
                runtime  'Release'
                optimize 'Full'
            filter {}
            
            filter 'configurations:Production'
                symbols  'off'
                runtime  'Release'
                optimize 'Full'
            filter {}
    group ""

    -- Generate projects from targets
    printf('\n    ---Generating Targets (NumTargets=%d)---', #TargetRules)
    for Index = 1, #TargetRules do
        local TempTarget = TargetRules[Index]
        
        local StartProject = ''
        if (TempTarget.TargetType == ETargetType.Client) and (not TempTarget.bIsMonolithic) then
            StartProject = TempTarget.Name .. 'Standalone'
        else
            StartProject = TempTarget.Name
        end

        TempTarget.Generate()

        printf('    StartProject = \'%s\'', StartProject)
        startproject(StartProject)
    end

    printf('---Finished generating workspace\n')
end