#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanRefCounted.h"

typedef TSharedRef<class FVulkanSamplerState> FVulkanSamplerStateRef;

class FVulkanSamplerState : public FRHISamplerState, public FVulkanDeviceChild
{
public:
    FVulkanSamplerState(FVulkanDevice* InDevice, const FRHISamplerStateInfo& InSamplerInfo);
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
