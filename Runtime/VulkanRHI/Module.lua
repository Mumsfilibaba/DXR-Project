include '../../BuildScripts/Scripts/build_module.lua'

---------------------------------------------------------------------------------------------------
-- Vulkan Helpers

function GetVulkanIncludePath()
    local VulkanEnvironmentVars = 
    {
        'VK_SDK_PATH',
        'VULKAN_SDK'
    }

    -- Since premake or lua is retarded we have to hardcode this for now on mac 
    if os.host() == "macosx" then
        return '/Users/dahlle/VulkanSDK/1.3.204.0/macOS'
    end

    for _, EnvironmentVar in ipairs(VulkanEnvironmentVars) do
        local Path = os.getenv(EnvironmentVar)
        if Path ~= nil then
            printf('Found \'%s\'=\'%s\'', EnvironmentVar, Path)
            return Path
        else
            printf('[WARNING]: Could not find the environment variable \'%s\'', EnvironmentVar)
        end
    end

    printf('[ERROR]: Failed to find Vulkan SDK path')
    return ''
end

---------------------------------------------------------------------------------------------------
-- VulkanRHI Module

local VulkanPath = GetVulkanIncludePath()
printf('VulkanPath=%s', VulkanPath)

local VulkanBinaries = VulkanPath .. '/bin'
printf('Vulkan bin path=%s', VulkanBinaries)

local VulkanLibraries = VulkanPath .. '/lib'
printf('Vulkan lib path=%s', VulkanLibraries)

local VulkanInclude = VulkanPath .. '/include'
printf('Vulkan include path=%s', VulkanInclude)

local VulkanRHI = CModuleBuildRules('VulkanRHI')
VulkanRHI.bRuntimeLinking = false

VulkanRHI.AddModuleDependencies( 
{
    'Core',
    'CoreApplication',
    'RHI',
})

if BuildWithXcode() then
    VulkanRHI.AddFrameWorks( 
    {
        'QuartzCore'
    })
end

VulkanRHI.AddSystemIncludes(
{
    VulkanInclude
})

VulkanRHI.AddLibraryPaths(
{
    VulkanLibraries
})

VulkanRHI.Generate()