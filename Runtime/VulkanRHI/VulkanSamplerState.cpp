#include "VulkanRHI/VulkanSamplerState.h"
#include "VulkanRHI/VulkanDevice.h"

FVulkanSamplerStateRHI::FVulkanSamplerStateRHI(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc)
    : FRHISamplerState(InSamplerDesc)
    , FVulkanDeviceChild(InDevice)
    , Sampler(VK_NULL_HANDLE)
{
}

FVulkanSamplerStateRHI::~FVulkanSamplerStateRHI()
{
    Sampler = VK_NULL_HANDLE;
}

void* FVulkanSamplerStateRHI::GetRHINativeSampler() const
{
    return reinterpret_cast<void*>(Sampler);
}

FRHIDescriptorHandle FVulkanSamplerStateRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

bool FVulkanSamplerStateRHI::Initialize()
{
    if (!GetDevice()->FindOrCreateSampler(Desc, Sampler))
    {
        VULKAN_ERROR_CRITICAL("Failed to create sampler");
        return false;
    }

    return true;
}
