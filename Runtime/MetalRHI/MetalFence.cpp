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
    return LastSignaledValue.Increment();
}

bool FMetalFenceRHI::IsSignaled() const
{
    const uint64 TargetValue = LastSignaledValue.Load();
    if (!SharedEvent || TargetValue == 0)
    {
        return false;
    }

    return SharedEvent.signaledValue >= TargetValue;
}

bool FMetalFenceRHI::Wait(uint64 TimeoutNs) const
{
    const uint64 TargetValue = LastSignaledValue.Load();
    if (!SharedEvent || TargetValue == 0)
    {
        return false;
    }

    if (SharedEvent.signaledValue >= TargetValue)
    {
        return true;
    }

    const uint64 TimeoutMS = (TimeoutNs == UINT64_MAX) ? UINT64_MAX : (TimeoutNs / 1000000ull);
    return [SharedEvent waitUntilSignaledValue:TargetValue timeoutMS:TimeoutMS];
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
