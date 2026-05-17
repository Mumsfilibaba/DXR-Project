#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanLoader.h"
#include "VulkanRHI/VulkanDeviceChild.h"
typedef TSharedRef<class FVulkanSamplerStateRHI> FVulkanSamplerStateRHIRef;

class FVulkanSamplerStateRHI : public FRHISamplerState, public FVulkanDeviceChild
{
public:
    FVulkanSamplerStateRHI(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InSamplerDesc);
    virtual ~FVulkanSamplerStateRHI();

    bool Initialize();
    
    // FRHISamplerState Interface
    virtual void* GetRHINativeSampler() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;
    
    VkSampler GetVkSampler() const
    {
        return Sampler;
    }
    
private:
    VkSampler                    Sampler;
    mutable FRHIDescriptorHandle BindlessHandle;
};
