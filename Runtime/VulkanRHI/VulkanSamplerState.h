#pragma once
#include "RHI/RHIResources.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanSamplerState

class CVulkanSamplerState : public CRHISamplerState
{
public:
    CVulkanSamplerState() = default;
    ~CVulkanSamplerState() = default;

    virtual bool IsValid() const override final
    {
        return true;
    }
};
