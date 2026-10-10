#include "MetalRHI/MetalHeap.h"
#include "MetalRHI/MetalRHI.h"

FMetalHeap::FMetalHeap(FMetalDevice* InDevice, id<MTLHeap> InHeap, uint64 InSize)
    : FMetalDeviceChild(InDevice)
    , Heap(InHeap)
    , Size(InSize)
    , ResidencyEntry()
{
    CHECK(Heap != nil);

    ResidencyEntry.Allocation  = Heap;
    ResidencyEntry.SizeInBytes = Size;
    ResidencyEntry.bIsHeap     = true;
}

FMetalHeap::~FMetalHeap()
{
    if (Heap)
    {
        [Heap release];
        Heap = nil;
    }
}

void FMetalHeap::DeferredRelease()
{
    if (!Heap)
    {
        return;
    }

    FMetalDeviceRHI::DeferDeletion(Heap);
    [Heap release];
    Heap = nil;
}

void FMetalHeap::SetDebugName(const String& Name)
{
    if (Heap)
    {
        Heap.label = Name.GetNSString();
    }
}
