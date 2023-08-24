#pragma once
#include "VulkanLoader.h"
#include "VulkanDeviceObject.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanSamplerState> FVulkanSamplerStateRef;

class FVulkanSamplerState : public FRHISamplerState, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanSamplerState(FVulkanDevice* InDevice, const FRHISamplerStateDesc& InDesc);
    virtual ~FVulkanSamplerState();

    bool Initialize();

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    
    VkSampler GetVkSampler() const
    {
        return Sampler;
    }
    
private:
    VkSampler Sampler;
};
