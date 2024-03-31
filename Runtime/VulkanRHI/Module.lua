include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Vulkan Helpers

function FindVulkanIncludePath()
    -- Vulkan is installed to the local folder on mac (Latest installed version) so we can just return this global path
    if IsPlatformMac() then
        return '/usr/local'
    end

    local VulkanEnvironmentVars = 
    {
        'VK_SDK_PATH',
        'VULKAN_SDK'
    }

    for _, EnvironmentVar in ipairs(VulkanEnvironmentVars) do
        local Path = os.getenv(EnvironmentVar)
        if Path ~= nil then
            LogHighlight('Found \'%s\'=\'%s\'', EnvironmentVar, Path)
            return Path
        else
            LogWarning('[WARNING]: Could not find the environment variable \'%s\'', EnvironmentVar)
        end
    end

    LogError('[ERROR]: Failed to find Vulkan SDK path')
    return ''
end

_GVulkanIncludePath = CreateOSPath(FindVulkanIncludePath());
function GetVulkanIncludePath()
    return _GVulkanIncludePath
end

---------------------------------------------------------------------------------------------------
-- VulkanRHI Module

local VulkanPath = GetVulkanIncludePath()
LogHighlight('VulkanPath=%s', VulkanPath)

local VulkanBinaries = JoinPath(VulkanPath, 'bin')
LogHighlight('Vulkan bin path=%s', VulkanBinaries)

local VulkanLibraries = JoinPath(VulkanPath, 'lib')
LogHighlight('Vulkan lib path=%s', VulkanLibraries)

local VulkanInclude = JoinPath(VulkanPath, 'include')
LogHighlight('Vulkan include path=%s', VulkanInclude)

local VulkanRHI = FModuleBuildRules('VulkanRHI')
VulkanRHI.bRuntimeLinking        = true
VulkanRHI.bUsePrecompiledHeaders = true

VulkanRHI.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'RHI',
    'Project',
})

if IsPlatformMac() then
    VulkanRHI.AddFrameWorks( 
    {
        'QuartzCore',
    })
end

VulkanRHI.AddSystemIncludes(
{
    VulkanInclude,
    CreateExternalDependencyPath("SPIRV-Cross"),
})

VulkanRHI.AddLibraryPaths(
{
    VulkanLibraries,
})

VulkanRHI.AddLinkLibraries(
{
    "SPIRV-Cross",
})