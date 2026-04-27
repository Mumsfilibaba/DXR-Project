#pragma once
#include "RHI/RHIFence.h"

class FMetalFenceRHI final : public FRHIFence
{
public:
    FMetalFenceRHI();
    virtual ~FMetalFenceRHI();

    // FRHIFence Interface
    virtual void* GetRHINativeFence() const override final;
    
    virtual bool IsSignaled()           const override final;
    virtual bool Wait(uint64 TimeoutNs) const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

private:
    FString DebugName;
};
