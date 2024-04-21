#include "VulkanSamplerState.h"
#include "VulkanDevice.h"

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
    VkSamplerCreateInfo SamplerCreateInfo;
    FMemory::Memzero(&SamplerCreateInfo);

    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = ConvertSamplerFilterToMagFilter(Info.Filter);
    SamplerCreateInfo.minFilter               = ConvertSamplerFilterToMinFilter(Info.Filter);
    SamplerCreateInfo.mipmapMode              = ConvertSamplerFilterToMipmapMode(Info.Filter);
    SamplerCreateInfo.addressModeU            = ConvertSamplerMode(Info.AddressU);
    SamplerCreateInfo.addressModeV            = ConvertSamplerMode(Info.AddressV);
    SamplerCreateInfo.addressModeW            = ConvertSamplerMode(Info.AddressW);
    SamplerCreateInfo.mipLodBias              = Info.MipLODBias;
    SamplerCreateInfo.anisotropyEnable        = IsAnisotropySampler(Info.Filter);
    SamplerCreateInfo.maxAnisotropy           = Info.MaxAnisotropy;
    SamplerCreateInfo.compareEnable           = IsComparissonSampler(Info.Filter);
    SamplerCreateInfo.compareOp               = ConvertComparisonFunc(Info.ComparisonFunc);
    SamplerCreateInfo.minLod                  = Info.MinLOD;
    SamplerCreateInfo.maxLod                  = Info.MaxLOD;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    if (!GetDevice()->FindOrCreateSampler(SamplerCreateInfo, Sampler))
    {
        VULKAN_ERROR("Failed to create sampler");
        return false;
    }
    else
    {
        return true;
    }
}
