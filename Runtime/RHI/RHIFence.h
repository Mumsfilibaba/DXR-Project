#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHIResource.h"

class FRHIFence : public FRHIResource 
{ 
protected: 
    FRHIFence()
        : FRHIResource(ERHIResourceType::Fence)
    {
    }

    virtual ~FRHIFence() = default; 
    
public:

    /** @return D3D12: ID3D12Fence*. Vulkan: VkSemaphore (timeline). Metal: nullptr. Null: nullptr. */
    virtual void* GetRHINativeFence() const = 0;

    virtual bool IsSignaled() const = 0; 
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const = 0; 

    virtual void SetDebugName(const String& InName) = 0; 
    virtual void GetDebugName(String& OutDebugName) const = 0; 
}; 
