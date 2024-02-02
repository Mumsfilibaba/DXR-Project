#include "VulkanSamplerState.h"
#include "VulkanDevice.h"

FVulkanSamplerState::FVulkanSamplerState(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InDesc)
    : FRHISamplerState(InDesc)
    , FVulkanDeviceChild(InDevice)
    , Sampler(VK_NULL_HANDLE)
{
}

FVulkanSamplerState::~FVulkanSamplerState()
{
    if (VULKAN_CHECK_HANDLE(Sampler))
    {
        vkDestroySampler(GetDevice()->GetVkDevice(), Sampler, nullptr);
        Sampler = VK_NULL_HANDLE;
    }
}

bool FVulkanSamplerState::Initialize()
{
    VkSamplerCreateInfo SamplerCreateInfo;
    FMemory::Memzero(&SamplerCreateInfo);

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
    SamplerCreateInfo.compareEnable           = IsComparissonSampler(Desc.Filter);
    SamplerCreateInfo.compareOp               = ConvertComparisonFunc(Desc.ComparisonFunc);
    SamplerCreateInfo.minLod                  = Desc.MinLOD;
    SamplerCreateInfo.maxLod                  = Desc.MaxLOD;
    SamplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerCreateInfo.unnormalizedCoordinates = false;

    VkResult Result = vkCreateSampler(GetDevice()->GetVkDevice(), &SamplerCreateInfo, nullptr, &Sampler);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create sampler");
        return false;
    }

    return true;
}
