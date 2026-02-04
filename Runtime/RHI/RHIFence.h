#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHIResource.h"

class FRHIGpuFence : public FRHIResource 
{ 
public: 
    FRHIGpuFence() = default; 
    virtual ~FRHIGpuFence() = default; 
 
    virtual bool IsSignaled() const = 0; 
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const = 0; 
 
    virtual void SetDebugName(const FString& InName) = 0; 
    virtual FString GetDebugName() const = 0; 
}; 
