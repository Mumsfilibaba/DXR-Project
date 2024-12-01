include '../../BuildScripts/Scripts/build_module.lua'

-- Vulkan Helpers

function find_vulkan_include_path()
    -- Vulkan is installed to the local folder on macOS (latest installed version), so we can just return this global path
    if is_platform_mac() then
        return '/usr/local'
    end

    local vulkan_environment_vars = {
        'VK_SDK_PATH',
        'VULKAN_SDK',
    }

    for _, environment_var in ipairs(vulkan_environment_vars) do
        local path = os.getenv(environment_var)
        if path ~= nil then
            log_highlight("Found '%s'='%s'", environment_var, path)
            return path
        else
            log_warning("[WARNING]: Could not find the environment variable '%s'", environment_var)
        end
    end

    log_error("[ERROR]: Failed to find Vulkan SDK path")
    return ''
end

local _g_vulkan_include_path = create_os_path(find_vulkan_include_path())

function get_vulkan_include_path()
    return _g_vulkan_include_path
end

-- VulkanRHI Module

local vulkan_path = get_vulkan_include_path()
log_highlight('VulkanPath=%s', vulkan_path)

local vulkan_binaries = join_path(vulkan_path, 'bin')
log_highlight('Vulkan bin path=%s', vulkan_binaries)

local vulkan_libraries = join_path(vulkan_path, 'lib')
log_highlight('Vulkan lib path=%s', vulkan_libraries)

local vulkan_include = join_path(vulkan_path, 'include')
log_highlight('Vulkan include path=%s', vulkan_include)

local vulkan_rhi = module_build_rules('VulkanRHI')
vulkan_rhi.runtime_linking        = true
vulkan_rhi.use_precompiled_headers = true

vulkan_rhi.add_module_dependencies
{
    'Core',
    'CoreApplication',
    'RHI',
    'Project',
}

if is_platform_mac() then
    vulkan_rhi.add_frameworks
    {
        'QuartzCore',
    }
end

vulkan_rhi.add_system_includes
{
    vulkan_include,
    create_external_dependency_path("SPIRV-Cross"),
}

vulkan_rhi.add_library_paths
{
    vulkan_libraries,
}

vulkan_rhi.add_link_libraries
{
    "SPIRV-Cross",
}
