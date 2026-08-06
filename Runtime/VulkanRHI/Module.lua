include "BuildTool.lua"

-- Vulkan Helpers

-- Assume that we have the Vulkan SDK installed (For macOS)
local gVulkanInstalled = true

local function ExistsDir(Path)
    return Path and Path ~= '' and os.isdir(Path)
end

local function ExistsFile(Path)
    return Path and Path ~= '' and os.isfile(Path)
end

function FindVulkanIncludePath()
    -- macOS: verify a complete SDK under /usr/local
    if IsPlatformMac() then
        local Root    = '/usr/local'
        local IncDir  = JoinPath(Root, 'include', 'vulkan')
        local LibDir  = JoinPath(Root, 'lib')
        local BinDir  = JoinPath(Root, 'bin')
        local HeaderH = JoinPath(IncDir, 'vulkan.h')

        -- Typical libs/tools shipped by LunarG SDK on macOS
        local LibCandidates = {
            JoinPath(LibDir, 'libvulkan.1.dylib'),
            JoinPath(LibDir, 'libvulkan.dylib'),
            JoinPath(LibDir, 'libMoltenVK.dylib'),
        }
        local ToolCandidates = {
            JoinPath(BinDir, 'vulkaninfo'),
            JoinPath(BinDir, 'glslc'),
        }

        local HaveLib = false
        for _, f in ipairs(LibCandidates) do
            if ExistsFile(f) then 
                HaveLib = true
                break
            end
        end

        local HaveTool = false
        for _, f in ipairs(ToolCandidates) do
            if ExistsFile(f) then 
                HaveTool = true
                break
            end
        end

        if not ExistsDir(IncDir) or not ExistsFile(HeaderH) then
            LogError("[ERROR]: Vulkan headers not found under %s (expected %s)", Root, HeaderH)
            gVulkanInstalled = false
            return ''
        end
        if not ExistsDir(LibDir) or not HaveLib then
            LogError("[ERROR]: Vulkan libraries not found under %s (looked for libvulkan*/MoltenVK)", LibDir)
            gVulkanInstalled = false
            return ''
        end
        if not ExistsDir(BinDir) or not HaveTool then
            LogError("[ERROR]: Vulkan tools not found under %s (looked for vulkaninfo/glslc)", BinDir)
            gVulkanInstalled = false
            return ''
        end

        LogHighlight("Detected Vulkan SDK at %s (headers/libs/tools present)", Root)
        return Root
    else
        -- Windows/Linux: check common environment variables
        local VulkanEnvironmentVars = {
            'VK_SDK_PATH',
            'VULKAN_SDK',
        }
        
        for _, EnvVar in ipairs(VulkanEnvironmentVars) do
            local PathVal = os.getenv(EnvVar)
            if PathVal ~= nil then
                LogHighlight("Found '%s'='%s'", EnvVar, PathVal)
                return PathVal
            else
                LogWarning("[WARNING]: Could not find the environment variable '%s'", EnvVar)
            end
        end
        
        LogError("[ERROR]: Failed to find Vulkan SDK path")
        gVulkanInstalled = false
        return ''
    end
end

local gVulkanIncludePath = CreateOsPath(FindVulkanIncludePath())

function GetVulkanIncludePath()
    return gVulkanIncludePath
end

-- If the SDK wasn't found, skip the rest of this script.
if not gVulkanInstalled or (gVulkanIncludePath == nil or gVulkanIncludePath == '') then
    LogWarning("[VulkanRHI] Vulkan SDK not found. Skipping VulkanRHI build rules.")
    return
end

local VulkanPath = GetVulkanIncludePath()
LogHighlight('VulkanPath=%s', VulkanPath)

local VulkanBinaries = JoinPath(VulkanPath, 'bin')
LogHighlight('Vulkan bin path=%s', VulkanBinaries)

local VulkanLibraries = JoinPath(VulkanPath, 'lib')
LogHighlight('Vulkan lib path=%s', VulkanLibraries)

local VulkanInclude = JoinPath(VulkanPath, 'include')
LogHighlight('Vulkan include path=%s', VulkanInclude)

-- VulkanRHI Module
local VulkanRHI = ModuleBuildRules('VulkanRHI')
VulkanRHI.bRuntimeLinking         = true
VulkanRHI.bUsePrecompiledHeaders  = true

VulkanRHI.AddModules({
    'Core',
    'CoreApplication',
    'RHI',
    "SPIRV-Cross",
})

if IsPlatformMac() then
    VulkanRHI.AddFrameworks({
        'QuartzCore',
    })
end

VulkanRHI.AddExternalIncludeDirs({
    VulkanInclude,
})

VulkanRHI.AddLibraryPaths({
    VulkanLibraries,
})

-- Fall back to the dxc the Vulkan SDK ships, which is the only one available on platforms where
-- none is committed to ThirdParty.
local DxcExecutable = ResolveDxcExecutable(VulkanBinaries)
LogHighlight('DXC path=%s', DxcExecutable)

-- The buffer-clear shader is compiled to SPIR-V headers as a build step so that creating the clear
-- pipelines never has to touch the shader compiler or the disk.
AddGeneratedShaderHeaderRule(VulkanRHI, {
    Compiler     = DxcExecutable,
    Source       = 'Shaders/Internal/ClearBufferUAV.hlsl',
    Arguments    = GetSpirvShaderArguments('cs_6_2'),
    SymbolPrefix = 'GVulkanClearBufferUAV_',
    Permutations = {
        { Name = 'Float', Defines = { 'CLEAR_ELEMENT_UINT=0', 'CLEAR_ELEMENT_SINT=0' } },
        { Name = 'Uint',  Defines = { 'CLEAR_ELEMENT_UINT=1', 'CLEAR_ELEMENT_SINT=0' } },
        { Name = 'Sint',  Defines = { 'CLEAR_ELEMENT_UINT=0', 'CLEAR_ELEMENT_SINT=1' } },
    },
})
