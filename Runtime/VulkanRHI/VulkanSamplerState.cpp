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

bool FVulkanSamplerStateRHI::Initialize()
{
    VkSamplerCreateInfo SamplerCreateInfo = {};
    SamplerCreateInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateInfo.magFilter               = ConvertSamplerFilterToMagFilter(Desc.Filter);
    SamplerCreateInfo.minFilter               = ConvertSamplerFilterToMinFilter(Desc.Filter);
    SamplerCreateInfo.mipmapMode              = ConvertSamplerFilterToMipmapMode(Desc.Filter);
    SamplerCreateInfo.addressModeU            = ConvertSamplerMode(Desc.AddressU);
    SamplerCreateInfo.addressModeV            = ConvertSamplerMode(Desc.AddressV);
    SamplerCreateInfo.addressModeW            = ConvertSamplerMode(Desc.AddressW);
    SamplerCreateInfo.mipLodBias              = Desc.MipLODBias;
    SamplerCreateInfo.anisotropyEnable        = IsAnisotropySampler(Desc.Filter);
    SamplerCreateInfo.maxAnisotropy           = Desc.MaxAnisotropy;
    SamplerCreateInfo.compareEnable           = IsComparisonSampler(Desc.Filter);
    SamplerCreateInfo.compareOp               = ConvertComparisonFunc(Desc.ComparisonFunc);
    SamplerCreateInfo.minLod                  = Desc.MinLOD;
    SamplerCreateInfo.maxLod                  = Desc.MaxLOD;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;
    
    // If anisotropy isn't enabled, force 1.0f. If it is, clamp to at least 1.0f.
    if (!SamplerCreateInfo.anisotropyEnable)
    {
        SamplerCreateInfo.maxAnisotropy = 1.0f;
    }
    else
    {
        SamplerCreateInfo.maxAnisotropy = Math::Max(1.0f, SamplerCreateInfo.maxAnisotropy);
    }
    
    // Ensure LOD range is sane
    if (SamplerCreateInfo.maxLod < SamplerCreateInfo.minLod)
    {
        Math::Swap(SamplerCreateInfo.minLod, SamplerCreateInfo.maxLod);
    }

    if (!GetDevice()->FindOrCreateSampler(SamplerCreateInfo, Sampler))
    {
        VULKAN_ERROR_CRITICAL("Failed to create sampler");
        return false;
    }
    else
    {
        return true;
    }
}
