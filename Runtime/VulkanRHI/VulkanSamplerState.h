#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanRefCounted.h"

typedef TSharedRef<class FVulkanSamplerStateRHI> FVulkanSamplerStateRHIRef;

class FVulkanSamplerStateRHI : public FRHISamplerState, public FVulkanDeviceChild
{
public:
    FVulkanSamplerStateRHI(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc);
    virtual ~FVulkanSamplerStateRHI();

    bool Initialize();
    
    // FRHISamplerState Interface
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    
    VkSampler GetVkSampler() const
    {
        return Sampler;
    }
    
private:
    VkSampler Sampler;
};
