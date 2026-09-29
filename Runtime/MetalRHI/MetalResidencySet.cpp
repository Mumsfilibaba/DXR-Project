#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalDevice.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"

static TAutoConsoleVariable<bool> CVarForceResidencyFallback(
    "MetalRHI.ForceResidencyFallback",
    "Declares bindless residency with useHeaps/useResources on each encoder, as on macOS 14, even where MTLResidencySet is available",
    false);

FMetalResidencySet::FMetalResidencySet(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , ResidencySet(nil)
    , FallbackHeaps()
    , FallbackResources()
    , FallbackIndices()
    , CriticalSection()
    , FallbackGeneration(0)
    , bDirty(0)
    , bUseEncoderFallback(true)
{
}

FMetalResidencySet::~FMetalResidencySet()
{
    CHECK(ResidencySet == nil);
}

void FMetalResidencySet::Initialize()
{
    bUseEncoderFallback = true;

    if (@available(macOS 15.0, *))
    {
        if (CVarForceResidencyFallback.GetValue())
        {
            METAL_WARNING("MetalRHI.ForceResidencyFallback is set, declaring residency per encoder");
            return;
        }

        SCOPED_AUTORELEASE_POOL();

        MTLResidencySetDescriptor* Descriptor = [[MTLResidencySetDescriptor new] autorelease];
        Descriptor.label = @"MetalRHI.ResidencySet";

        NSError* Error = nil;
        ResidencySet = [GetDevice()->GetMTLDevice() newResidencySetWithDescriptor:Descriptor error:&Error];

        if (!ResidencySet)
        {
            const String ErrorString(Error ? [Error localizedDescription] : @"unknown error");
            METAL_INFO("MTLResidencySet is unavailable on this device (%s), declaring residency per encoder", *ErrorString);
            return;
        }

        GetDevice()->ForEachQueue([this](FMetalQueue& Queue)
        {
            [Queue.GetMTLCommandQueue() addResidencySet:static_cast<id<MTLResidencySet>>(ResidencySet)];
        });

        bUseEncoderFallback = false;
    }
}

void FMetalResidencySet::Release()
{
    if (@available(macOS 15.0, *))
    {
        if (ResidencySet)
        {
            GetDevice()->ForEachQueue([this](FMetalQueue& Queue)
            {
                [Queue.GetMTLCommandQueue() removeResidencySet:static_cast<id<MTLResidencySet>>(ResidencySet)];
            });
        }
    }

    [ResidencySet release];
    ResidencySet = nil;

    TScopedLock Lock(CriticalSection);
    FallbackHeaps.Clear();
    FallbackResources.Clear();
    FallbackIndices.Clear();
}

void FMetalResidencySet::AddHeap(id<MTLHeap> Heap)
{
    CHECK(Heap != nil);

    if (@available(macOS 15.0, *))
    {
        if (!bUseEncoderFallback)
        {
            [static_cast<id<MTLResidencySet>>(ResidencySet) addAllocation:static_cast<id<MTLAllocation>>(Heap)];
            bDirty.Store(1);
            return;
        }
    }

    TScopedLock Lock(CriticalSection);
    FallbackHeaps.Add(Heap);
    FallbackGeneration.Increment();
}

void FMetalResidencySet::Add(id<MTLResource> Resource, bool bBindlessReachable)
{
    CHECK(Resource != nil);

    if (@available(macOS 15.0, *))
    {
        if (!bUseEncoderFallback)
        {
            [static_cast<id<MTLResidencySet>>(ResidencySet) addAllocation:static_cast<id<MTLAllocation>>(Resource)];
            bDirty.Store(1);
            return;
        }
    }

    if (bBindlessReachable)
    {
        TScopedLock Lock(CriticalSection);
        FallbackIndices.Add(reinterpret_cast<const void*>(Resource), FallbackResources.Size());
        FallbackResources.Add(Resource);
        FallbackGeneration.Increment();
    }
}

void FMetalResidencySet::RemoveHeap(id<MTLHeap> Heap)
{
    CHECK(Heap != nil);

    if (@available(macOS 15.0, *))
    {
        if (!bUseEncoderFallback)
        {
            [static_cast<id<MTLResidencySet>>(ResidencySet) removeAllocation:static_cast<id<MTLAllocation>>(Heap)];
            bDirty.Store(1);
            return;
        }
    }

    TScopedLock Lock(CriticalSection);
    for (int32 Index = 0; Index < FallbackHeaps.Size(); ++Index)
    {
        if (FallbackHeaps[Index] == Heap)
        {
            FallbackHeaps.RemoveAtSwap(Index);
            break;
        }
    }
}

void FMetalResidencySet::Remove(id<MTLResource> Resource)
{
    CHECK(Resource != nil);

    if (@available(macOS 15.0, *))
    {
        if (!bUseEncoderFallback)
        {
            [static_cast<id<MTLResidencySet>>(ResidencySet) removeAllocation:static_cast<id<MTLAllocation>>(Resource)];
            bDirty.Store(1);
            return;
        }
    }

    TScopedLock Lock(CriticalSection);

    const int32* FoundIndex = FallbackIndices.Find(reinterpret_cast<const void*>(Resource));

    if (!FoundIndex)
    {
        return;
    }

    const int32 Index = *FoundIndex;
    FallbackIndices.Remove(reinterpret_cast<const void*>(Resource));

    const int32 LastIndex = FallbackResources.Size() - 1;

    if (Index != LastIndex)
    {
        FallbackResources[Index] = FallbackResources[LastIndex];
        FallbackIndices[reinterpret_cast<const void*>(FallbackResources[Index])] = Index;
    }

    FallbackResources.Pop();
}

void FMetalResidencySet::CommitIfDirty()
{
    if (@available(macOS 15.0, *))
    {
        if (ResidencySet && bDirty.Exchange(0) != 0)
        {
            [static_cast<id<MTLResidencySet>>(ResidencySet) commit];
        }
    }
}

template<typename EncoderType>
uint64 FMetalResidencySet::DeclareForEncoder(EncoderType Encoder)
{
    static constexpr MTLResourceUsage Usage = MTLResourceUsageRead | MTLResourceUsageWrite;

    TScopedLock Lock(CriticalSection);

    if constexpr (std::is_same_v<EncoderType, id<MTLRenderCommandEncoder>>)
    {
        const MTLRenderStages Stages = MetalRHI::GetRenderStages();

        if (!FallbackHeaps.IsEmpty())
        {
            [Encoder useHeaps:FallbackHeaps.Data() count:FallbackHeaps.Size() stages:Stages];
        }

        if (!FallbackResources.IsEmpty())
        {
            [Encoder useResources:FallbackResources.Data() count:FallbackResources.Size() usage:Usage stages:Stages];
        }
    }
    else
    {
        if (!FallbackHeaps.IsEmpty())
        {
            [Encoder useHeaps:FallbackHeaps.Data() count:FallbackHeaps.Size()];
        }

        if (!FallbackResources.IsEmpty())
        {
            [Encoder useResources:FallbackResources.Data() count:FallbackResources.Size() usage:Usage];
        }
    }

    return FallbackGeneration.Load();
}

template uint64 FMetalResidencySet::DeclareForEncoder(id<MTLRenderCommandEncoder>);
template uint64 FMetalResidencySet::DeclareForEncoder(id<MTLComputeCommandEncoder>);
