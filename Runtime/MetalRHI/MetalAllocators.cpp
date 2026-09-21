#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Math/Math.h"
#include "Core/Threading/ScopedLock.h"

static uint64 GetLastSubmittedValue(FMetalQueue* Queue)
{
    return Queue ? Queue->GetLastSubmittedValue() : 0;
}

FMetalHeapPool::FMetalHeapPool(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , PoolCS()
    , HeapBlocks()
    , PendingHeapFrees()
{
}

FMetalHeapPool::~FMetalHeapPool()
{
    Destroy();
}

bool FMetalHeapPool::TryAllocate(uint64 SizeInBytes, uint64 Alignment, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap)
{
    const uint64 ResolvedAlignment = Math::Max(Alignment, 16ull);

    TScopedLock Lock(PoolCS);

    for (int32 Index = 0; Index < HeapBlocks.Size(); ++Index)
    {
        FHeapBlock& Block = HeapBlocks[Index];
        if (!Block.Heap)
        {
            continue;
        }

        if (TrySuballocate(Block, static_cast<uint32>(Index), SizeInBytes, ResolvedAlignment, OutOffset))
        {
            OutHeapIndex = static_cast<uint32>(Index);
            OutHeap      = Block.Heap;
            return true;
        }
    }

    FHeapBlock* NewBlock = CreateHeapBlock(Math::Max(SizeInBytes, DefaultHeapSize));
    if (!NewBlock)
    {
        return false;
    }

    uint32 HeapIndex = UINT32_MAX;
    for (int32 Index = 0; Index < HeapBlocks.Size(); ++Index)
    {
        if (&HeapBlocks[Index] == NewBlock)
        {
            HeapIndex = static_cast<uint32>(Index);
            break;
        }
    }

    if (HeapIndex == UINT32_MAX || !TrySuballocate(*NewBlock, HeapIndex, SizeInBytes, ResolvedAlignment, OutOffset))
    {
        return false;
    }

    OutHeapIndex = HeapIndex;
    OutHeap      = NewBlock->Heap;
    return true;
}

void FMetalHeapPool::Deallocate(uint32 HeapIndex, uint64 Offset, uint64 Size, FMetalQueue* LastUsedQueue, uint64 LastUsedValue)
{
    TScopedLock Lock(PoolCS);

    FPendingHeapFree PendingFree;
    PendingFree.HeapIndex = HeapIndex;
    PendingFree.Offset    = Offset;
    PendingFree.Size      = Size;
    StampPendingHeapFree(PendingFree, LastUsedQueue, LastUsedValue);
    PendingHeapFrees.Add(PendingFree);
}

void FMetalHeapPool::CleanUp()
{
    TScopedLock Lock(PoolCS);
    RecyclePendingHeapFrees();
    DropUnusedHeaps();
}

void FMetalHeapPool::Destroy()
{
    TScopedLock Lock(PoolCS);

    RecyclePendingHeapFrees();
    PendingHeapFrees.Clear();

    for (FHeapBlock& Block : HeapBlocks)
    {
        if (Block.Heap)
        {
            delete Block.Heap;
            Block.Heap = nullptr;
        }
    }

    HeapBlocks.Clear();
}

FMetalHeapPool::FHeapBlock* FMetalHeapPool::CreateHeapBlock(uint64 MinimumSize)
{
    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();

    MTLHeapDescriptor* HeapDescriptor = [[MTLHeapDescriptor new] autorelease];
    HeapDescriptor.type               = MTLHeapTypePlacement;
    HeapDescriptor.storageMode        = MTLStorageModePrivate;
    HeapDescriptor.cpuCacheMode       = MTLCPUCacheModeDefaultCache;
    HeapDescriptor.hazardTrackingMode = MTLHazardTrackingModeTracked;
    HeapDescriptor.size               = Math::Max(MinimumSize, DefaultHeapSize);

    id<MTLHeap> MetalHeap = [DeviceHandle newHeapWithDescriptor:HeapDescriptor];
    if (!MetalHeap)
    {
        return nullptr;
    }

    MetalHeap.label = @"MetalDeviceHeap";

    FMetalHeap* Heap = new FMetalHeap(GetDevice(), MetalHeap, HeapDescriptor.size);
    Heap->SetDebugName("MetalDeviceHeap");

    for (int32 Index = 0; Index < HeapBlocks.Size(); ++Index)
    {
        FHeapBlock& Existing = HeapBlocks[Index];
        if (Existing.Heap)
        {
            continue;
        }

        Existing.Heap       = Heap;
        Existing.Size       = HeapDescriptor.size;
        Existing.UsedBytes  = 0;
        Existing.AllocCount = 0;
        Existing.FreeRanges.Clear();
        Existing.FreeRanges.Add({ 0, Existing.Size });
        return &Existing;
    }

    FHeapBlock& Block = HeapBlocks.Emplace();
    Block.Heap        = Heap;
    Block.Size        = HeapDescriptor.size;
    Block.UsedBytes   = 0;
    Block.AllocCount  = 0;
    Block.FreeRanges.Add({ 0, Block.Size });
    return &Block;
}

bool FMetalHeapPool::TrySuballocate(FHeapBlock& Block, uint32 HeapIndex, uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset)
{
    UNREFERENCED_VARIABLE(HeapIndex);

    for (int32 RangeIndex = 0; RangeIndex < Block.FreeRanges.Size(); ++RangeIndex)
    {
        FHeapFreeRange& Range = Block.FreeRanges[RangeIndex];
        const uint64 AlignedOffset = Math::AlignUp(Range.Offset, Alignment);
        const uint64 Padding       = AlignedOffset - Range.Offset;
        if (Padding + SizeInBytes > Range.Size)
        {
            continue;
        }

        const uint64 Remaining = Range.Size - Padding - SizeInBytes;
        if (Padding > 0)
        {
            Range.Size = Padding;
            if (Remaining > 0)
            {
                Block.FreeRanges.Insert(RangeIndex + 1, { AlignedOffset + SizeInBytes, Remaining });
            }
        }
        else if (Remaining > 0)
        {
            Range.Offset += SizeInBytes;
            Range.Size    = Remaining;
        }
        else
        {
            Block.FreeRanges.RemoveAt(RangeIndex);
        }

        Block.UsedBytes  += SizeInBytes;
        Block.AllocCount += 1;
        OutOffset         = AlignedOffset;
        return true;
    }

    return false;
}

void FMetalHeapPool::ReturnHeapRange(uint32 HeapIndex, uint64 Offset, uint64 Size)
{
    if (HeapIndex >= static_cast<uint32>(HeapBlocks.Size()) || Size == 0)
    {
        return;
    }

    FHeapBlock& Block = HeapBlocks[HeapIndex];
    if (Block.UsedBytes >= Size)
    {
        Block.UsedBytes -= Size;
    }
    else
    {
        Block.UsedBytes = 0;
    }

    if (Block.AllocCount > 0)
    {
        Block.AllocCount -= 1;
    }

    int32 InsertIndex = 0;
    while (InsertIndex < Block.FreeRanges.Size() && Block.FreeRanges[InsertIndex].Offset < Offset)
    {
        ++InsertIndex;
    }

    Block.FreeRanges.Insert(InsertIndex, { Offset, Size });

    for (int32 Index = 0; Index + 1 < Block.FreeRanges.Size(); )
    {
        FHeapFreeRange& Current = Block.FreeRanges[Index];
        FHeapFreeRange& Next    = Block.FreeRanges[Index + 1];
        if (Current.Offset + Current.Size >= Next.Offset)
        {
            const uint64 End = Math::Max(Current.Offset + Current.Size, Next.Offset + Next.Size);
            Current.Size = End - Current.Offset;
            Block.FreeRanges.RemoveAt(Index + 1);
        }
        else
        {
            ++Index;
        }
    }
}

void FMetalHeapPool::StampPendingHeapFree(FPendingHeapFree& PendingFree, FMetalQueue* LastUsedQueue, uint64 LastUsedValue) const
{
    PendingFree.LastUsedQueue     = LastUsedQueue;
    PendingFree.LastUsedValue     = LastUsedValue;
    PendingFree.DirectFenceValue  = GetLastSubmittedValue(GetDevice()->GetQueue(EMetalQueueType::Direct));
    PendingFree.ComputeFenceValue = GetLastSubmittedValue(GetDevice()->GetQueue(EMetalQueueType::Compute));
    PendingFree.CopyFenceValue    = GetLastSubmittedValue(GetDevice()->GetQueue(EMetalQueueType::Copy));
}

bool FMetalHeapPool::IsHeapFreeEligible(const FPendingHeapFree& PendingFree) const
{
    auto IsComplete = [](FMetalQueue* Queue, uint64 Value) -> bool
    {
        if (!Queue || Value == 0)
        {
            return true;
        }

        return Queue->GetCompletedValue() >= Value;
    };

    if (PendingFree.LastUsedQueue && PendingFree.LastUsedValue > 0)
    {
        return IsComplete(PendingFree.LastUsedQueue, PendingFree.LastUsedValue);
    }

    return IsComplete(GetDevice()->GetQueue(EMetalQueueType::Direct), PendingFree.DirectFenceValue)
        && IsComplete(GetDevice()->GetQueue(EMetalQueueType::Compute), PendingFree.ComputeFenceValue)
        && IsComplete(GetDevice()->GetQueue(EMetalQueueType::Copy), PendingFree.CopyFenceValue);
}

void FMetalHeapPool::RecyclePendingHeapFrees()
{
    for (int32 Index = PendingHeapFrees.Size() - 1; Index >= 0; --Index)
    {
        const FPendingHeapFree& PendingFree = PendingHeapFrees[Index];
        if (!IsHeapFreeEligible(PendingFree))
        {
            continue;
        }

        ReturnHeapRange(PendingFree.HeapIndex, PendingFree.Offset, PendingFree.Size);
        PendingHeapFrees.RemoveAt(Index);
    }
}

void FMetalHeapPool::DropUnusedHeaps()
{
    uint64 UnusedBytes = 0;
    for (const FHeapBlock& Block : HeapBlocks)
    {
        if (Block.Heap && Block.AllocCount == 0)
        {
            UnusedBytes += Block.Size;
        }
    }

    if (UnusedBytes <= MaxUnusedHeapBytes)
    {
        return;
    }

    for (int32 Index = HeapBlocks.Size() - 1; Index >= 0 && UnusedBytes > MaxUnusedHeapBytes; --Index)
    {
        FHeapBlock& Block = HeapBlocks[Index];
        if (!Block.Heap || Block.AllocCount != 0)
        {
            continue;
        }

        bool bHasPending = false;
        for (const FPendingHeapFree& PendingFree : PendingHeapFrees)
        {
            if (PendingFree.HeapIndex == static_cast<uint32>(Index))
            {
                bHasPending = true;
                break;
            }
        }

        if (bHasPending)
        {
            continue;
        }

        UnusedBytes -= Block.Size;
        Block.Heap->DeferredRelease();
        delete Block.Heap;
        Block.Heap       = nullptr;
        Block.UsedBytes  = 0;
        Block.AllocCount = 0;
        Block.FreeRanges.Clear();
    }
}

#if METAL_ENABLE_STATS
void FMetalHeapPool::UpdateMemoryStats(FMetalAllocatorUsage& OutUsage) const
{
    OutUsage = FMetalAllocatorUsage();
    for (const FHeapBlock& Block : HeapBlocks)
    {
        if (!Block.Heap)
        {
            continue;
        }

        OutUsage.AllocatedBytes += Block.Size;
        OutUsage.UsedBytes      += Block.UsedBytes;
    }

    if (OutUsage.AllocatedBytes > OutUsage.UsedBytes)
    {
        OutUsage.FragmentedBytes = OutUsage.AllocatedBytes - OutUsage.UsedBytes;
    }
}
#endif

FMetalLinearAllocator::FMetalLinearAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InLargeAllocationThreshold)
    : FMetalDeviceChild(InDevice)
    , AllocatorsCS()
    , Pages()
    , ActivePage(nullptr)
    , PageSizeBytes(InPageSizeBytes)
    , LargeAllocationThreshold(InLargeAllocationThreshold)
{
}

FMetalLinearAllocator::~FMetalLinearAllocator()
{
    Destroy();
}

void* FMetalLinearAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage)
{
    OutStorage.Reset();

    if (SizeInBytes == 0 || !Queue)
    {
        return nullptr;
    }

    const uint64 ResolvedAlignment = (Alignment != 0) ? Alignment : BUFFER_ALIGNMENT;
    const bool   bDedicated        = SizeInBytes > LargeAllocationThreshold;

    TScopedLock Lock(AllocatorsCS);

    FPage* Page = nullptr;
    if (bDedicated)
    {
        Page = CreatePage(SizeInBytes, true);
        if (Page)
        {
            Pages.Add(Page);
        }
    }
    else
    {
        Page = FindFreePage(SizeInBytes, ResolvedAlignment, Queue);
        if (!Page)
        {
            Page = CreatePage(PageSizeBytes, false);
            if (Page)
            {
                Pages.Add(Page);
            }
        }

        ActivePage = Page;
    }

    if (!Page || !Page->Buffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte Metal upload page", SizeInBytes);
        return nullptr;
    }

    [Page->Buffer setPurgeableState:MTLPurgeableStateNonVolatile];

    const uint64 AlignedOffset = Math::AlignUp(Page->Offset, ResolvedAlignment);
    if (AlignedOffset + SizeInBytes > Page->Size)
    {
        METAL_ERROR("Upload page of %llu bytes cannot fit a %llu byte allocation", Page->Size, SizeInBytes);
        return nullptr;
    }

    Page->Offset                 = AlignedOffset + SizeInBytes;
    Page->UsedBytes             += SizeInBytes;
    Page->Queue                  = Queue;
    Page->EligibleFromFenceValue = UINT64_MAX;
    Page->bPendingRetire         = bDedicated;

    if (bDedicated && ActivePage == Page)
    {
        ActivePage = nullptr;
    }

    void* Mapped = static_cast<uint8*>(Page->Buffer.contents) + AlignedOffset;
    OutStorage.InitSuballocatedResource(Page->Buffer, AlignedOffset, SizeInBytes, Mapped, this, nullptr);
    return Mapped;
}

void FMetalLinearAllocator::RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue)
{
    if (!Queue || SubmissionValue == 0)
    {
        return;
    }

    TScopedLock Lock(AllocatorsCS);

    if (ActivePage && ActivePage->Queue == Queue && ActivePage->UsedBytes > 0)
    {
        ActivePage->bPendingRetire         = true;
        ActivePage->EligibleFromFenceValue = SubmissionValue;
        ActivePage                         = nullptr;
    }

    for (FPage* Page : Pages)
    {
        if (Page && Page->bPendingRetire && Page->Queue == Queue && Page->EligibleFromFenceValue == UINT64_MAX)
        {
            Page->EligibleFromFenceValue = SubmissionValue;
        }
    }
}

void FMetalLinearAllocator::CleanUp()
{
    TScopedLock Lock(AllocatorsCS);

    for (FPage* Page : Pages)
    {
        if (!Page || !Page->bPendingRetire || Page->EligibleFromFenceValue == UINT64_MAX)
        {
            continue;
        }

        if (!Page->Queue || Page->Queue->GetCompletedValue() < Page->EligibleFromFenceValue)
        {
            continue;
        }

        Page->Offset                 = 0;
        Page->UsedBytes              = 0;
        Page->Queue                  = nullptr;
        Page->EligibleFromFenceValue = UINT64_MAX;
        Page->bPendingRetire         = false;

        if (Page->Buffer)
        {
            [Page->Buffer setPurgeableState:MTLPurgeableStateVolatile];
        }
    }

    DropUnusedPages();
}

void FMetalLinearAllocator::Destroy()
{
    TScopedLock Lock(AllocatorsCS);

    for (FPage* Page : Pages)
    {
        ReleasePage(Page);
    }

    Pages.Clear();
    ActivePage = nullptr;
}

FMetalLinearAllocator::FPage* FMetalLinearAllocator::CreatePage(uint64 SizeInBytes, bool bDedicated)
{
    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();
    const uint64  PageSize     = Math::Max(SizeInBytes, 1ull);

    id<MTLBuffer> Buffer = [DeviceHandle newBufferWithLength:PageSize
                                                     options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined];
    if (!Buffer)
    {
        return nullptr;
    }

    Buffer.label = bDedicated ? @"MetalUploadDedicated" : @"MetalUploadPage";

    FPage* Page = new FPage();
    Page->Buffer                   = Buffer;
    Page->Size                     = PageSize;
    Page->Offset                   = 0;
    Page->UsedBytes                = 0;
    Page->Queue                    = nullptr;
    Page->EligibleFromFenceValue   = UINT64_MAX;
    Page->bDedicated               = bDedicated;
    Page->bPendingRetire           = false;
    return Page;
}

FMetalLinearAllocator::FPage* FMetalLinearAllocator::FindFreePage(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue)
{
    auto Fits = [SizeInBytes, Alignment](const FPage* Page) -> bool
    {
        if (!Page || Page->bDedicated)
        {
            return false;
        }

        const uint64 AlignedOffset = Math::AlignUp(Page->Offset, Alignment);
        return (AlignedOffset + SizeInBytes) <= Page->Size;
    };

    if (ActivePage)
    {
        if (ActivePage->Queue == Queue && Fits(ActivePage))
        {
            return ActivePage;
        }

        if (ActivePage->UsedBytes > 0)
        {
            ActivePage->bPendingRetire = true;
        }

        ActivePage = nullptr;
    }

    for (FPage* Page : Pages)
    {
        if (!Page || Page->bPendingRetire || Page->UsedBytes != 0)
        {
            continue;
        }

        if (Fits(Page))
        {
            Page->Offset = 0;
            return Page;
        }
    }

    return nullptr;
}

void FMetalLinearAllocator::ReleasePage(FPage* Page)
{
    if (!Page)
    {
        return;
    }

    if (Page->Buffer)
    {
        [Page->Buffer setPurgeableState:MTLPurgeableStateEmpty];
        [Page->Buffer release];
        Page->Buffer = nil;
    }

    delete Page;
}

void FMetalLinearAllocator::DropUnusedPages()
{
    uint64 UnusedBytes = 0;
    for (const FPage* Page : Pages)
    {
        if (Page && !Page->bPendingRetire && Page->UsedBytes == 0)
        {
            UnusedBytes += Page->bDedicated ? (MaxUnusedBytes + 1) : Page->Size;
        }
    }

    if (UnusedBytes <= MaxUnusedBytes)
    {
        return;
    }

    for (int32 Index = Pages.Size() - 1; Index >= 0 && UnusedBytes > MaxUnusedBytes; --Index)
    {
        FPage* Page = Pages[Index];
        if (!Page || Page->bPendingRetire || Page->UsedBytes != 0 || ActivePage == Page)
        {
            continue;
        }

        UnusedBytes -= Page->Size;
        METAL_INFO("Dropping unused Metal upload page (%llu bytes)", Page->Size);
        ReleasePage(Page);
        Pages.RemoveAt(Index);
    }
}

#if METAL_ENABLE_STATS
void FMetalLinearAllocator::UpdateMemoryStats(FMetalAllocatorUsage& OutUsage) const
{
    OutUsage = FMetalAllocatorUsage();
    for (const FPage* Page : Pages)
    {
        if (!Page)
        {
            continue;
        }

        OutUsage.AllocatedBytes += Page->Size;
        OutUsage.UsedBytes      += Page->UsedBytes;
    }

    if (OutUsage.AllocatedBytes > OutUsage.UsedBytes)
    {
        OutUsage.FragmentedBytes = OutUsage.AllocatedBytes - OutUsage.UsedBytes;
    }
}
#endif

FMetalUploadHeapAllocator::FMetalUploadHeapAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InConstantPageSizeBytes, uint64 InLargeAllocationThreshold)
    : FMetalDeviceChild(InDevice)
    , UploadAllocator(InDevice, InPageSizeBytes, InLargeAllocationThreshold)
    , ConstantsAllocator(InDevice, InConstantPageSizeBytes, InLargeAllocationThreshold)
{
}

FMetalUploadHeapAllocator::~FMetalUploadHeapAllocator()
{
    Destroy();
}

void* FMetalUploadHeapAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage)
{
    return UploadAllocator.Allocate(SizeInBytes, Alignment, Queue, OutStorage);
}

void* FMetalUploadHeapAllocator::AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FMetalQueue* Queue, FMetalResourceStorage& OutStorage)
{
    const uint64 ResolvedAlignment = (Alignment != 0) ? Alignment : CONSTANT_BUFFER_ALIGNMENT;
    return ConstantsAllocator.Allocate(SizeInBytes, ResolvedAlignment, Queue, OutStorage);
}

void FMetalUploadHeapAllocator::RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue)
{
    UploadAllocator.RetireAllocations(Queue, SubmissionValue);
    ConstantsAllocator.RetireAllocations(Queue, SubmissionValue);
}

void FMetalUploadHeapAllocator::CleanUp()
{
    UploadAllocator.CleanUp();
    ConstantsAllocator.CleanUp();

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

void FMetalUploadHeapAllocator::Destroy()
{
    UploadAllocator.Destroy();
    ConstantsAllocator.Destroy();
}

#if METAL_ENABLE_STATS
void FMetalUploadHeapAllocator::UpdateMemoryStats()
{
    FMetalAllocatorUsage UploadUsage;
    FMetalAllocatorUsage ConstantsUsage;
    UploadAllocator.UpdateMemoryStats(UploadUsage);
    ConstantsAllocator.UpdateMemoryStats(ConstantsUsage);

    STAT_SET(STAT_Metal_UploadHeapAllocated,  UploadUsage.AllocatedBytes + ConstantsUsage.AllocatedBytes);
    STAT_SET(STAT_Metal_UploadHeapUsed,       UploadUsage.UsedBytes + ConstantsUsage.UsedBytes);
    STAT_SET(STAT_Metal_UploadHeapFragmented, UploadUsage.FragmentedBytes + ConstantsUsage.FragmentedBytes);
}
#endif

FMetalBufferAllocator::FMetalBufferAllocator(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , HeapPool(InDevice)
{
}

FMetalBufferAllocator::~FMetalBufferAllocator()
{
    Destroy();
}

bool FMetalBufferAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, MTLResourceOptions Options, FMetalResourceStorage& OutStorage)
{
    OutStorage.Reset();

    if (SizeInBytes == 0)
    {
        return false;
    }

    const uint64 ResolvedAlignment = Math::Max(Alignment, static_cast<uint64>(BUFFER_ALIGNMENT));
    const MTLStorageMode StorageMode = MTLStorageMode((Options & MTLResourceStorageModeMask) >> MTLResourceStorageModeShift);
    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();

    if (StorageMode == MTLStorageModePrivate)
    {
        MTLSizeAndAlign SizeAndAlign = [DeviceHandle heapBufferSizeAndAlignWithLength:SizeInBytes options:Options];
        if (SizeAndAlign.size == 0)
        {
            SizeAndAlign.size  = SizeInBytes;
            SizeAndAlign.align = ResolvedAlignment;
        }

        SizeAndAlign.align = Math::Max<NSUInteger>(SizeAndAlign.align, ResolvedAlignment);

        uint32      HeapIndex = UINT32_MAX;
        uint64      Offset    = 0;
        FMetalHeap* Heap      = nullptr;
        if (SizeAndAlign.size <= 64ull * 1024ull * 1024ull && HeapPool.TryAllocate(SizeAndAlign.size, SizeAndAlign.align, HeapIndex, Offset, Heap))
        {
            id<MTLBuffer> Buffer = [Heap->GetMTLHeap() newBufferWithLength:SizeInBytes options:Options offset:Offset];
            if (Buffer)
            {
                OutStorage.InitSuballocatedHeap(Buffer, Heap, Offset, SizeAndAlign.size, HeapIndex, this);
                return true;
            }

            HeapPool.Deallocate(HeapIndex, Offset, SizeAndAlign.size);
        }
    }

    id<MTLBuffer> Buffer = [DeviceHandle newBufferWithLength:SizeInBytes options:Options];
    if (!Buffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte Metal buffer", SizeInBytes);
        return false;
    }

    OutStorage.InitStandalone(Buffer, SizeInBytes);
    return true;
}

void FMetalBufferAllocator::Deallocate(FMetalResourceStorage& Storage)
{
    if (Storage.GetStorageType() == EMetalResourceStorageType::SuballocatedHeap)
    {
        HeapPool.Deallocate(Storage.GetHeapIndex(), Storage.GetResourceOffset(), Storage.GetSize(), Storage.GetLastUsedQueue(), Storage.GetLastUsedValue());
    }
}

void FMetalBufferAllocator::CleanUp()
{
    HeapPool.CleanUp();

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

void FMetalBufferAllocator::Destroy()
{
    HeapPool.Destroy();
}

#if METAL_ENABLE_STATS
void FMetalBufferAllocator::UpdateMemoryStats()
{
    FMetalAllocatorUsage HeapUsage;
    HeapPool.UpdateMemoryStats(HeapUsage);
    STAT_SET(STAT_Metal_DeviceHeapAllocated, HeapUsage.AllocatedBytes);
    STAT_SET(STAT_Metal_DeviceHeapUsed, HeapUsage.UsedBytes);
    STAT_SET(STAT_Metal_DeviceHeapFragmented, HeapUsage.FragmentedBytes);
}
#endif

FMetalTextureAllocator::FMetalTextureAllocator(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , HeapPool(InDevice)
{
}

FMetalTextureAllocator::~FMetalTextureAllocator()
{
    Destroy();
}

bool FMetalTextureAllocator::TryAllocate(MTLTextureDescriptor* TextureDescriptor, FMetalResourceStorage& OutStorage)
{
    OutStorage.Reset();

    if (!TextureDescriptor)
    {
        return false;
    }

    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();

    if (TextureDescriptor.storageMode == MTLStorageModePrivate)
    {
        const MTLSizeAndAlign SizeAndAlign = [DeviceHandle heapTextureSizeAndAlignWithDescriptor:TextureDescriptor];
        uint32      HeapIndex = UINT32_MAX;
        uint64      Offset    = 0;
        FMetalHeap* Heap      = nullptr;
        if (SizeAndAlign.size > 0 && SizeAndAlign.size <= 64ull * 1024ull * 1024ull && HeapPool.TryAllocate(SizeAndAlign.size, SizeAndAlign.align, HeapIndex, Offset, Heap))
        {
            id<MTLTexture> Texture = [Heap->GetMTLHeap() newTextureWithDescriptor:TextureDescriptor offset:Offset];
            if (Texture)
            {
                OutStorage.InitSuballocatedHeap(Texture, Heap, Offset, SizeAndAlign.size, HeapIndex, this);
                return true;
            }

            HeapPool.Deallocate(HeapIndex, Offset, SizeAndAlign.size);
        }
    }

    id<MTLTexture> Texture = [DeviceHandle newTextureWithDescriptor:TextureDescriptor];
    if (!Texture)
    {
        METAL_ERROR("Failed to allocate a Metal texture");
        return false;
    }

    OutStorage.InitStandalone(Texture, 0);
    return true;
}

void FMetalTextureAllocator::Deallocate(FMetalResourceStorage& Storage)
{
    if (Storage.GetStorageType() == EMetalResourceStorageType::SuballocatedHeap)
    {
        HeapPool.Deallocate(Storage.GetHeapIndex(), Storage.GetResourceOffset(), Storage.GetSize(), Storage.GetLastUsedQueue(), Storage.GetLastUsedValue());
    }
}

void FMetalTextureAllocator::CleanUp()
{
    HeapPool.CleanUp();

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

void FMetalTextureAllocator::Destroy()
{
    HeapPool.Destroy();
}

#if METAL_ENABLE_STATS
void FMetalTextureAllocator::UpdateMemoryStats()
{
    FMetalAllocatorUsage Usage;
    HeapPool.UpdateMemoryStats(Usage);
    STAT_SET(STAT_Metal_DeviceHeapAllocated,  STAT_GET(STAT_Metal_DeviceHeapAllocated) + static_cast<int64>(Usage.AllocatedBytes));
    STAT_SET(STAT_Metal_DeviceHeapUsed,       STAT_GET(STAT_Metal_DeviceHeapUsed) + static_cast<int64>(Usage.UsedBytes));
    STAT_SET(STAT_Metal_DeviceHeapFragmented, STAT_GET(STAT_Metal_DeviceHeapFragmented) + static_cast<int64>(Usage.FragmentedBytes));
}
#endif
