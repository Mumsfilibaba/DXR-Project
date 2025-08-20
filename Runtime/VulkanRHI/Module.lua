include '../../SetupScripts/Scripts/BuildTool_Module.lua'

-- Vulkan Helpers

-- Assume that we have the Vulkan SDK installed (For macOS)
local _g_vulkan_installed = true

local function _exists_dir(p)  
    return p and p ~= '' and os.isdir(p)  
end

local function _exists_file(p) 
    return p and p ~= '' and os.isfile(p)
end

function find_vulkan_include_path()
    -- macOS: verify a complete SDK under /usr/local
    if is_platform_mac() then
        local root     = '/usr/local'
        local inc_dir  = join_path(root, 'include', 'vulkan')
        local lib_dir  = join_path(root, 'lib')
        local bin_dir  = join_path(root, 'bin')
        local header_h = join_path(inc_dir, 'vulkan.h')

        -- Typical libs/tools shipped by LunarG SDK on macOS
        local lib_candidates = {
            join_path(lib_dir, 'libvulkan.1.dylib'),
            join_path(lib_dir, 'libvulkan.dylib'),
            join_path(lib_dir, 'libMoltenVK.dylib'),
        }
        local tool_candidates = {
            join_path(bin_dir, 'vulkaninfo'),
            join_path(bin_dir, 'glslc'),
        }

        local have_lib = false
        for _, f in ipairs(lib_candidates) do
            if _exists_file(f) then have_lib = true; break end
        end

        local have_tool = false
        for _, f in ipairs(tool_candidates) do
            if _exists_file(f) then have_tool = true; break end
        end

        if not _exists_dir(inc_dir) or not _exists_file(header_h) then
            log_error("[ERROR]: Vulkan headers not found under %s (expected %s)", root, header_h)
            _g_vulkan_installed = false
            return ''
        end
        if not _exists_dir(lib_dir) or not have_lib then
            log_error("[ERROR]: Vulkan libraries not found under %s (looked for libvulkan*/MoltenVK)", lib_dir)
            _g_vulkan_installed = false
            return ''
        end
        if not _exists_dir(bin_dir) or not have_tool then
            log_error("[ERROR]: Vulkan tools not found under %s (looked for vulkaninfo/glslc)", bin_dir)
            _g_vulkan_installed = false
            return ''
        end

        log_highlight("Detected Vulkan SDK at %s (headers/libs/tools present)", root)
        return root
    else
        
        -- Windows/Linux: check common environment variables
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
        _g_vulkan_installed = false
        return ''
    end
end

local _g_vulkan_include_path = create_os_path(find_vulkan_include_path())

function get_vulkan_include_path()
    return _g_vulkan_include_path
end

-- If the SDK wasn't found, skip the rest of this script.
if not _g_vulkan_installed or (_g_vulkan_include_path == nil or _g_vulkan_include_path == '') then
    log_warning("[VulkanRHI] Vulkan SDK not found. Skipping VulkanRHI build rules.")
    return
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
vulkan_rhi.runtime_linking         = true
vulkan_rhi.use_precompiled_headers = true

vulkan_rhi.add_module_thirdparties
{
    'Core',
    'CoreApplication',
    'RHI',
}

if is_platform_mac() then
    vulkan_rhi.add_frameworks
    {
        'QuartzCore',
    }
end

vulkan_rhi.add_external_include_dirs
{
    vulkan_include,
    create_external_thirdparty_path("SPIRV-Cross"),
}

vulkan_rhi.add_library_paths
{
    vulkan_libraries,
}

vulkan_rhi.add_link_libraries
{
    "SPIRV-Cross",
}
