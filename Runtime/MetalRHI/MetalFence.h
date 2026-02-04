#pragma once
#include "RHI/RHIFence.h"

class FMetalGpuFence final : public FRHIGpuFence
{
public:
    FMetalGpuFence()
        : FRHIGpuFence()
        , DebugName()
    {
    }

    // FRHIGpuFence Interface
    virtual bool IsSignaled() const override final { return true; }
    virtual bool Wait(uint64 TimeoutNs) const override final { (void)TimeoutNs; return true; }
    virtual void SetDebugName(const FString& InName) override final { DebugName = InName; }
    virtual FString GetDebugName() const override final { return DebugName; }

private:
    FString DebugName;
};
