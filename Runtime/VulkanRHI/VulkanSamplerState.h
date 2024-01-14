#pragma once
#include "VulkanLoader.h"
#include "VulkanDeviceObject.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanSamplerState> FVulkanSamplerStateRef;

class FVulkanSamplerState : public FRHISamplerState, public FVulkanDeviceObject
{
public:
    FVulkanSamplerState(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InDesc);
    virtual ~FVulkanSamplerState();

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    
    bool Initialize();
    
    VkSampler GetVkSampler() const
    {
        return Sampler;
    }
    
private:
    VkSampler Sampler;
};
