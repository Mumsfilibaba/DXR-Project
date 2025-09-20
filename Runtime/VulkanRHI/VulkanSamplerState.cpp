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
    SamplerCreateInfo.compareEnable           = IsComparisonSampler(Info.Filter);
    SamplerCreateInfo.compareOp               = ConvertComparisonFunc(Info.ComparisonFunc);
    SamplerCreateInfo.minLod                  = Info.MinLOD;
    SamplerCreateInfo.maxLod                  = Info.MaxLOD;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;
	
    // If anisotropy isn't enabled, force 1.0f. If it is, clamp to at least 1.0f.
	if (!SamplerCreateInfo.anisotropyEnable)
	{
	    SamplerCreateInfo.maxAnisotropy = 1.0f;
	}
	else
	{
	    SamplerCreateInfo.maxAnisotropy = FMath::Max(1.0f, SamplerCreateInfo.maxAnisotropy);
	}
	
    // Ensure LOD range is sane
    if (SamplerCreateInfo.maxLod < SamplerCreateInfo.minLod)
    {
        FMath::Swap(SamplerCreateInfo.minLod, SamplerCreateInfo.maxLod);
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
