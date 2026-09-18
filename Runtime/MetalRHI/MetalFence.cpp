#include "MetalRHI/MetalFence.h"

FMetalFenceRHI::FMetalFenceRHI(id<MTLDevice> Device)
    : FRHIFence()
    , SharedEvent(nil)
    , LastSignaledValue(0)
    , DebugName()
{
    CHECK(Device != nil);
    SharedEvent = [Device newSharedEvent];
}

FMetalFenceRHI::~FMetalFenceRHI()
{
    [SharedEvent release];
    SharedEvent = nil;
}

uint64 FMetalFenceRHI::SignalNextValue()
{
    ++LastSignaledValue;
    return LastSignaledValue;
}

bool FMetalFenceRHI::IsSignaled() const
{
    if (!SharedEvent)
    {
        return false;
    }

    return SharedEvent.signaledValue >= LastSignaledValue;
}

bool FMetalFenceRHI::Wait(uint64 TimeoutNs) const
{
    if (!SharedEvent)
    {
        return false;
    }

    if (LastSignaledValue == 0 || SharedEvent.signaledValue >= LastSignaledValue)
    {
        return true;
    }

    const uint64 TimeoutMS = (TimeoutNs == UINT64_MAX) ? UINT64_MAX : (TimeoutNs / 1000000ull);
    return [SharedEvent waitUntilSignaledValue:LastSignaledValue timeoutMS:TimeoutMS];
}

void FMetalFenceRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
    if (SharedEvent)
    {
        SharedEvent.label = InName.GetNSString();
    }
}

void FMetalFenceRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FMetalFenceRHI::GetRHINativeFence() const
{
    return SharedEvent;
}
