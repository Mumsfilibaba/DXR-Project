#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanSamplerState::FVulkanSamplerState(FVulkanDevice* InDevice, const FRHISamplerStateInfo& InSamplerInfo)
    : FRHISamplerState(InSamplerInfo)
    , FVulkanDeviceChild(InDevice)
    , Sampler(VK_NULL_HANDLE)
{
}

FVulkanSamplerState::~FVulkanSamplerState()
{
    Sampler = VK_NULL_HANDLE;
}

bool FVulkanSamplerState::Initialize()
{
    if (!GetDevice()->FindOrCreateSampler(Info, Sampler))
    {
        VULKAN_ERROR_CRITICAL("Failed to create sampler");
        return false;
    }

    return true;
}
