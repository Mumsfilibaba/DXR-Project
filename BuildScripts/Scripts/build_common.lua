include "build_log.lua"

-- Custom options
newoption
{
    trigger     = "monolithic",
    description = "Links all modules as static libraries instead of DLLs"
}

newoption
{
    trigger     = "platform",
    value       = "CurrentPlatform",
    description = "Specify the platform to use",
    allowed     = { { "Win32" }, { "macOS" } },
    default     = "Win32"
}

-- Global variables
local g_is_monolithic
local g_modules = {}

-- Check if the module should be built monolithically
function global_is_monolithic()
    if g_is_monolithic == nil then
        g_is_monolithic = (_OPTIONS["monolithic"] ~= nil)
    end
    return g_is_monolithic
end

-- Check if the current platform is Windows
function is_platform_windows()
    return _OPTIONS["platform"] == "Win32"
end

-- Check if the current platform is macOS
function is_platform_mac()
    return _OPTIONS["platform"] == "macOS"
end

-- Check the action being used
function build_with_xcode()
    return _ACTION == "xcode4"
end

local vs_actions =
{
    vs2022 = true, vs2019 = true, vs2017 = true, vs2015 = true,
    vs2013 = true, vs2012 = true, vs2010 = true, vs2008 = true, vs2005 = true
}

function build_with_visual_studio()
    return vs_actions[_ACTION] == true
end

-- Verify language version
local valid_language_versions =
{
    ["c++98"] = true, ["c++11"] = true, ["c++14"] = true,
    ["c++17"] = true, ["c++20"] = true, ["c++latest"] = true
}

function verify_language_version(language_version)
    return valid_language_versions[language_version] == true
end

-- Helper for printing all strings in a table and ending with endline
function print_table(format_str, tbl)
    if tbl == nil then 
        return 
    end
    for _, value in ipairs(tbl) do
        log_info(format_str, value)
    end
end

-- Helper to append multiple unique elements to a table
function add_unique_elements(elements, tbl)
    if tbl == nil or elements == nil then 
        return 
    end

    local element_set = {}
    for _, v in ipairs(tbl) do
        element_set[v] = true
    end
    for _, v in ipairs(elements) do
        if not element_set[v] then
            table.insert(tbl, v)
            element_set[v] = true
        end
    end
end

-- Module management functions
function get_module(module_name)
    return g_modules[module_name]
end

function is_module(module_name)
    return g_modules[module_name] ~= nil
end

function add_module(module_name, module)
    g_modules[module_name] = module
end

-- Path handling
local g_path_separator = is_platform_windows() and '\\' or '/'

function create_os_path(in_path)
    return path.translate(in_path, g_path_separator)
end

-- Main paths
local g_engine_path = create_os_path(path.getabsolute("../../", _PREMAKE_DIR))

function get_engine_path()
    return g_engine_path
end

-- Join two paths
function join_path(path_a, path_b)
    return create_os_path(path.join(path_a, path_b))
end

-- Retrieve the path to the Runtime folder containing all the engine modules
local g_runtime_folder_path = join_path(g_engine_path, "Runtime")

function get_runtime_folder_path()
    return g_runtime_folder_path
end

-- Retrieve the path to the Solutions folder containing solution and project files
local g_solutions_folder_path = join_path(g_engine_path, "Solutions")

function get_solutions_folder_path()
    return g_solutions_folder_path
end

-- Retrieve the path to the Dependencies folder containing external dependency projects
local g_external_dependencies_folder_path = join_path(g_engine_path, "Dependencies")

function get_external_dependencies_folder_path()
    return g_external_dependencies_folder_path
end

-- Make path relative to the dependency folder
function create_external_dependency_path(dependency_path)
    return join_path(get_external_dependencies_folder_path(), dependency_path)
end

-- Deep copy a table
function copy(source)
    return table.deepcopy(source)
end
