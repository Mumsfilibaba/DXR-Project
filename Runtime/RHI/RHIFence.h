#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHIResource.h"

class FRHIFence : public FRHIResource 
{ 
public: 
    FRHIFence() = default; 
    virtual ~FRHIFence() = default; 
 
    virtual bool IsSignaled() const = 0; 
    virtual bool Wait(uint64 TimeoutNs = UINT64_MAX) const = 0; 
 
    virtual void SetDebugName(const FString& InName) = 0; 
    virtual void GetDebugName(FString& OutDebugName) const = 0; 
}; 
