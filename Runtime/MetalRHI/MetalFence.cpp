#include "MetalRHI/MetalFence.h"

FMetalFenceRHI::FMetalFenceRHI()
    : FRHIFence()
    , DebugName()
{
}

FMetalFenceRHI::~FMetalFenceRHI() = default;

bool FMetalFenceRHI::IsSignaled() const
{
    return true;
}

bool FMetalFenceRHI::Wait(uint64 TimeoutNs) const
{
    (void)TimeoutNs;
    return true;
}

void FMetalFenceRHI::SetDebugName(const FString& InName)
{
    DebugName = InName;
}

void FMetalFenceRHI::GetDebugName(FString& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FMetalFenceRHI::GetRHINativeFence() const
{
    return nullptr;
}
