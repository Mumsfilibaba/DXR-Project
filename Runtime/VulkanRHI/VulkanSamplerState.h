#pragma once
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanSamplerState> FVulkanSamplerStateRef;

class FVulkanSamplerState : public FRHISamplerState
{
public:
    FVulkanSamplerState(const FRHISamplerStateDesc& InDesc)
        : FRHISamplerState(InDesc)
    {
    }

    virtual ~FVulkanSamplerState() = default;

    bool Initialize() { return true; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
};
