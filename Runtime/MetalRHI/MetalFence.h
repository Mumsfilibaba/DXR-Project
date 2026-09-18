#pragma once
#include "RHI/RHIFence.h"
#include "MetalRHI/MetalCore.h"

class FMetalFenceRHI final : public FRHIFence
{
public:
    FMetalFenceRHI(id<MTLDevice> Device);
    virtual ~FMetalFenceRHI();

    // FRHIFence Interface
    virtual void* GetRHINativeFence() const override final;

    virtual bool IsSignaled()           const override final;
    virtual bool Wait(uint64 TimeoutNs) const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    uint64 SignalNextValue();

    id<MTLSharedEvent> GetMTLSharedEvent() const
    {
        return SharedEvent;
    }

private:
    id<MTLSharedEvent> SharedEvent;
    uint64             LastSignaledValue;
    String             DebugName;
};
