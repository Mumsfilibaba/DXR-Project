#pragma once
#include "RHI/RHIFence.h"

class FMetalGpuFence final : public FRHIFence
{
public:
    FMetalGpuFence()
        : FRHIFence()
        , DebugName()
    {
    }

    // FRHIFence Interface
    virtual bool IsSignaled() const override final { return true; }
    virtual bool Wait(uint64 TimeoutNs) const override final { (void)TimeoutNs; return true; }
    virtual void SetDebugName(const FString& InName) override final { DebugName = InName; }
    virtual void GetDebugName(FString& OutDebugName) const override final { OutDebugName = DebugName; }

private:
    FString DebugName;
};
