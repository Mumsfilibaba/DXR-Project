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
    SamplerCreateDesc.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCreateDesc.magFilter               = ConvertSamplerFilterToMagFilter(Desc.Filter);
    SamplerCreateDesc.minFilter               = ConvertSamplerFilterToMinFilter(Desc.Filter);
    SamplerCreateDesc.mipmapMode              = ConvertSamplerFilterToMipmapMode(Desc.Filter);
    SamplerCreateDesc.addressModeU            = ConvertSamplerMode(Desc.AddressU);
    SamplerCreateDesc.addressModeV            = ConvertSamplerMode(Desc.AddressV);
    SamplerCreateDesc.addressModeW            = ConvertSamplerMode(Desc.AddressW);
    SamplerCreateDesc.mipLodBias              = Desc.MipLODBias;
    SamplerCreateDesc.anisotropyEnable        = IsAnisotropySampler(Desc.Filter);
    SamplerCreateDesc.maxAnisotropy           = Desc.MaxAnisotropy;
    SamplerCreateDesc.compareEnable           = IsComparisonSampler(Desc.Filter);
    SamplerCreateDesc.compareOp               = ConvertComparisonFunc(Desc.ComparisonFunc);
    SamplerCreateDesc.minLod                  = Desc.MinLOD;
    SamplerCreateDesc.maxLod                  = Desc.MaxLOD;
    SamplerCreateDesc.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateDesc.unnormalizedCoordinates = false;
    
    // If anisotropy isn't enabled, force 1.0f. If it is, clamp to at least 1.0f.
    if (!SamplerCreateDesc.anisotropyEnable)
    {
        SamplerCreateDesc.maxAnisotropy = 1.0f;
    }
    else
    {
        SamplerCreateDesc.maxAnisotropy = Math::Max(1.0f, SamplerCreateDesc.maxAnisotropy);
    }
    
    // Ensure LOD range is sane
    if (SamplerCreateDesc.maxLod < SamplerCreateDesc.minLod)
    {
        Math::Swap(SamplerCreateDesc.minLod, SamplerCreateDesc.maxLod);
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
