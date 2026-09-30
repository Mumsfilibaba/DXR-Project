#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalResidencyManager.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalStats.h"
#include "MetalRHI/MetalTexture.h"
#include "Core/Math/Math.h"
#include "Core/Threading/ScopedLock.h"

static constexpr uint64 GMaxHeapAllocationSize = 64ull * 1024ull * 1024ull;

static uint64 GetLastSubmittedValue(FMetalQueue* Queue)
{
    return Queue ? Queue->GetLastSubmittedValue() : 0;
}

static MTLTextureDescriptor* CreateDescriptorFromTexture(id<MTLTexture> Texture)
{
    MTLTextureDescriptor* Descriptor = [[MTLTextureDescriptor new] autorelease];
    Descriptor.textureType               = Texture.textureType;
    Descriptor.pixelFormat               = Texture.pixelFormat;
    Descriptor.width                     = Texture.width;
    Descriptor.height                    = Texture.height;
    Descriptor.depth                     = Texture.depth;
    Descriptor.mipmapLevelCount          = Texture.mipmapLevelCount;
    Descriptor.arrayLength               = Texture.arrayLength;
    Descriptor.sampleCount               = Texture.sampleCount;
    Descriptor.usage                     = Texture.usage;
    Descriptor.storageMode               = Texture.storageMode;
    Descriptor.cpuCacheMode              = Texture.cpuCacheMode;
    Descriptor.hazardTrackingMode        = MTLHazardTrackingModeUntracked;
    Descriptor.allowGPUOptimizedContents = Texture.allowGPUOptimizedContents;
    Descriptor.swizzle                   = Texture.swizzle;

    if (@available(macOS 12.5, *))
    {
        Descriptor.compressionType = Texture.compressionType;
    }

    return Descriptor;
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

bool FMetalHeapPool::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FMetalResourceStorage* Storage, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap)
{
    const uint64 ResolvedAlignment = Math::Max(Alignment, 16ull);

    {
        TScopedLock Lock(PoolCS);

        if (TryAllocateFromBlocks(SizeInBytes, ResolvedAlignment, Storage, OutHeapIndex, OutOffset, OutHeap))
        {
            return true;
        }
    }

    GetDevice()->GetResidencyManager().EvictIfNeeded();

    TScopedLock Lock(PoolCS);

    if (TryAllocateFromBlocks(SizeInBytes, ResolvedAlignment, Storage, OutHeapIndex, OutOffset, OutHeap))
    {
        return true;
    }

    const uint32 HeapIndex = CreateHeapBlock(Math::Max(SizeInBytes, DefaultHeapSize));

    if (HeapIndex == UINT32_MAX)
    {
        return false;
    }

    FHeapBlock& Block = HeapBlocks[HeapIndex];

    if (!TrySuballocate(Block, SizeInBytes, ResolvedAlignment, OutOffset))
    {
        return false;
    }

    RecordAllocation(Block, OutOffset, SizeInBytes, Storage);
    OutHeapIndex = HeapIndex;
    OutHeap      = Block.Heap;
    return true;
}

bool FMetalHeapPool::TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 SourceHeapIndex, FMetalResourceStorage* Storage, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap)
{
    const uint64 ResolvedAlignment = Math::Max(Alignment, 16ull);

    TScopedLock Lock(PoolCS);

    if (SourceHeapIndex >= static_cast<uint32>(HeapBlocks.Size()))
    {
        return false;
    }

    const uint64 SourceUsedBytes = HeapBlocks[SourceHeapIndex].UsedBytes;

    for (int32 Index = 0; Index < HeapBlocks.Size(); ++Index)
    {
        FHeapBlock& Block = HeapBlocks[Index];

        if (!Block.Heap || static_cast<uint32>(Index) == SourceHeapIndex || Block.Allocations.IsEmpty() || Block.UsedBytes < SourceUsedBytes)
        {
            continue;
        }

        if (TrySuballocate(Block, SizeInBytes, ResolvedAlignment, OutOffset))
        {
            RecordAllocation(Block, OutOffset, SizeInBytes, Storage);
            OutHeapIndex = static_cast<uint32>(Index);
            OutHeap      = Block.Heap;
            return true;
        }
    }

    return false;
}

void FMetalHeapPool::SelectDefragCandidates(uint32 MaxMoves, uint64 EligibleBeforeFrame, TArray<FMetalDefragCandidate>& OutCandidates)
{
    TScopedLock Lock(PoolCS);

    int32 SourceIndex = -1;
    for (int32 Index = 0; Index < HeapBlocks.Size(); ++Index)
    {
        const FHeapBlock& Block = HeapBlocks[Index];

        if (!Block.Heap || Block.Allocations.IsEmpty() || Block.FreeRanges.Size() <= 1)
        {
            continue;
        }

        if (SourceIndex < 0 || Block.UsedBytes < HeapBlocks[SourceIndex].UsedBytes)
        {
            SourceIndex = Index;
        }
    }

    if (SourceIndex < 0)
    {
        return;
    }

    for (const FHeapAllocation& Allocation : HeapBlocks[SourceIndex].Allocations)
    {
        if (static_cast<uint32>(OutCandidates.Size()) >= MaxMoves)
        {
            break;
        }

        FMetalResourceStorage* Storage = Allocation.Storage;

        if (!Storage || !Storage->GetOwner() || Storage->IsDefragPending() || Allocation.CreatedFrame >= EligibleBeforeFrame)
        {
            continue;
        }

        FMetalDefragCandidate& Candidate = OutCandidates.Emplace();
        Candidate.Storage   = Storage;
        Candidate.HeapIndex = static_cast<uint32>(SourceIndex);
    }
}

void FMetalHeapPool::RetargetAllocation(uint32 HeapIndex, uint64 Offset, FMetalResourceStorage* NewStorage)
{
    TScopedLock Lock(PoolCS);

    if (HeapIndex >= static_cast<uint32>(HeapBlocks.Size()))
    {
        return;
    }

    for (FHeapAllocation& Allocation : HeapBlocks[HeapIndex].Allocations)
    {
        if (Allocation.Offset == Offset)
        {
            Allocation.Storage = NewStorage;
            return;
        }
    }

    CHECK(false);
}

void FMetalHeapPool::Deallocate(uint32 HeapIndex, uint64 Offset, uint64 Size)
{
    TScopedLock Lock(PoolCS);

    if (HeapIndex < static_cast<uint32>(HeapBlocks.Size()))
    {
        TArray<FHeapAllocation>& Allocations = HeapBlocks[HeapIndex].Allocations;

        for (int32 Index = 0; Index < Allocations.Size(); ++Index)
        {
            if (Allocations[Index].Offset == Offset)
            {
                Allocations.RemoveAtSwap(Index);
                break;
            }
        }
    }

    FPendingHeapFree PendingFree;
    PendingFree.HeapIndex         = HeapIndex;
    PendingFree.Offset            = Offset;
    PendingFree.Size              = Size;
    PendingFree.DirectFenceValue  = GetLastSubmittedValue(GetDevice()->GetQueue(EMetalQueueType::Direct));
    PendingFree.ComputeFenceValue = GetLastSubmittedValue(GetDevice()->GetQueue(EMetalQueueType::Compute));
    PendingFree.CopyFenceValue    = GetLastSubmittedValue(GetDevice()->GetQueue(EMetalQueueType::Copy));
    PendingHeapFrees.Add(PendingFree);
}

void FMetalHeapPool::CleanUp()
{
    TScopedLock Lock(PoolCS);
    RecyclePendingHeapFrees();
    DropUnusedHeaps(MaxUnusedHeapBytes);
}

void FMetalHeapPool::Trim()
{
    TScopedLock Lock(PoolCS);
    RecyclePendingHeapFrees();
    DropUnusedHeaps(0);
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
            GetDevice()->GetResidencyManager().EndTracking(Block.Heap->GetResidencyEntry(), true);
            delete Block.Heap;
            Block.Heap = nullptr;
        }
    }

    HeapBlocks.Clear();
}

uint32 FMetalHeapPool::CreateHeapBlock(uint64 MinimumSize)
{
    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();

    MTLHeapDescriptor* HeapDescriptor = [[MTLHeapDescriptor new] autorelease];
    HeapDescriptor.type               = MTLHeapTypePlacement;
    HeapDescriptor.storageMode        = MTLStorageModePrivate;
    HeapDescriptor.cpuCacheMode       = MTLCPUCacheModeDefaultCache;
    HeapDescriptor.hazardTrackingMode = MTLHazardTrackingModeUntracked;
    HeapDescriptor.size               = Math::Max(MinimumSize, DefaultHeapSize);

    id<MTLHeap> MetalHeap = [DeviceHandle newHeapWithDescriptor:HeapDescriptor];

    if (!MetalHeap)
    {
        return UINT32_MAX;
    }

    MetalHeap.label = @"MetalDeviceHeap";

    FMetalHeap* Heap = new FMetalHeap(GetDevice(), MetalHeap, HeapDescriptor.size);
    Heap->SetDebugName("MetalDeviceHeap");
    GetDevice()->GetResidencyManager().BeginTracking(Heap->GetResidencyEntry());

    int32 BlockIndex = 0;
    while (BlockIndex < HeapBlocks.Size() && HeapBlocks[BlockIndex].Heap)
    {
        ++BlockIndex;
    }

    if (BlockIndex == HeapBlocks.Size())
    {
        HeapBlocks.Emplace();
    }

    FHeapBlock& Block = HeapBlocks[BlockIndex];
    Block.Heap      = Heap;
    Block.Size      = HeapDescriptor.size;
    Block.UsedBytes = 0;
    Block.bVolatile = false;
    Block.FreeRanges.Clear();
    Block.FreeRanges.Add({ 0, Block.Size });
    Block.Allocations.Clear();
    return static_cast<uint32>(BlockIndex);
}

void FMetalHeapPool::ReleaseHeapBlock(FHeapBlock& Block)
{
    CHECK(Block.Heap != nullptr && Block.Allocations.IsEmpty());

    GetDevice()->GetResidencyManager().EndTracking(Block.Heap->GetResidencyEntry(), true);
    Block.Heap->DeferredRelease();
    delete Block.Heap;

    Block.Heap      = nullptr;
    Block.UsedBytes = 0;
    Block.bVolatile = false;
    Block.FreeRanges.Clear();
    Block.Allocations.Clear();
}

bool FMetalHeapPool::ReviveHeapBlock(FHeapBlock& Block)
{
    if (!Block.bVolatile)
    {
        return true;
    }

    Block.bVolatile = false;

    if ([Block.Heap->GetMTLHeap() setPurgeableState:MTLPurgeableStateNonVolatile] == MTLPurgeableStateEmpty)
    {
        ReleaseHeapBlock(Block);
        return false;
    }

    return true;
}

bool FMetalHeapPool::TryAllocateFromBlocks(uint64 SizeInBytes, uint64 Alignment, FMetalResourceStorage* Storage, uint32& OutHeapIndex, uint64& OutOffset, FMetalHeap*& OutHeap)
{
    for (int32 Index = 0; Index < HeapBlocks.Size(); ++Index)
    {
        FHeapBlock& Block = HeapBlocks[Index];

        if (!Block.Heap || Block.Size - Block.UsedBytes < SizeInBytes)
        {
            continue;
        }

        if (!ReviveHeapBlock(Block))
        {
            continue;
        }

        if (TrySuballocate(Block, SizeInBytes, Alignment, OutOffset))
        {
            RecordAllocation(Block, OutOffset, SizeInBytes, Storage);
            OutHeapIndex = static_cast<uint32>(Index);
            OutHeap      = Block.Heap;
            return true;
        }
    }

    return false;
}

bool FMetalHeapPool::TrySuballocate(FHeapBlock& Block, uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset)
{
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

        Block.UsedBytes += SizeInBytes;
        OutOffset        = AlignedOffset;
        return true;
    }

    return false;
}

void FMetalHeapPool::RecordAllocation(FHeapBlock& Block, uint64 Offset, uint64 Size, FMetalResourceStorage* Storage)
{
    FHeapAllocation& Allocation = Block.Allocations.Emplace();
    Allocation.Offset       = Offset;
    Allocation.Size         = Size;
    Allocation.Storage      = Storage;
    Allocation.CreatedFrame = GetDevice()->GetFrameCounter();

    GetDevice()->GetResidencyManager().MakeResident(Block.Heap->GetResidencyEntry());
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

void FMetalHeapPool::DropUnusedHeaps(uint64 MaxUnusedBytes)
{
    const auto HasPendingFree = [this](int32 HeapIndex) -> bool
    {
        for (const FPendingHeapFree& PendingFree : PendingHeapFrees)
        {
            if (PendingFree.HeapIndex == static_cast<uint32>(HeapIndex))
            {
                return true;
            }
        }

        return false;
    };

    uint64 UnusedBytes = 0;
    for (const FHeapBlock& Block : HeapBlocks)
    {
        if (Block.Heap && Block.Allocations.IsEmpty())
        {
            UnusedBytes += Block.Size;
        }
    }

    for (int32 Index = HeapBlocks.Size() - 1; Index >= 0; --Index)
    {
        FHeapBlock& Block = HeapBlocks[Index];

        if (!Block.Heap || !Block.Allocations.IsEmpty() || HasPendingFree(Index))
        {
            continue;
        }

        if (UnusedBytes > MaxUnusedBytes)
        {
            UnusedBytes -= Block.Size;
            ReleaseHeapBlock(Block);
        }
        else if (!Block.bVolatile)
        {
            GetDevice()->GetResidencyManager().MakeNonResident(Block.Heap->GetResidencyEntry());
            [Block.Heap->GetMTLHeap() setPurgeableState:MTLPurgeableStateVolatile];
            Block.bVolatile = true;
        }
    }
}

#if METAL_ENABLE_STATS
void FMetalHeapPool::UpdateMemoryStats(FMetalAllocatorUsage& OutUsage) const
{
    TScopedLock Lock(PoolCS);

    OutUsage = FMetalAllocatorUsage();
    for (const FHeapBlock& Block : HeapBlocks)
    {
        if (!Block.Heap)
        {
            continue;
        }

        OutUsage.AllocatedBytes += Block.Size;
        OutUsage.UsedBytes      += Block.UsedBytes;
        OutUsage.NumBlocks      += 1;
    }

    if (OutUsage.AllocatedBytes > OutUsage.UsedBytes)
    {
        OutUsage.FragmentedBytes = OutUsage.AllocatedBytes - OutUsage.UsedBytes;
    }
}
#endif

FMetalLinearAllocator::FMetalLinearAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InLargeAllocationThreshold, MTLResourceOptions InOptions, bool bInBindlessReachable, EMetalAllocationLifetime InLifetime)
    : FMetalDeviceChild(InDevice)
    , AllocatorsCS()
    , Pages()
    , ActivePage(nullptr)
    , PageSizeBytes(InPageSizeBytes)
    , LargeAllocationThreshold(InLargeAllocationThreshold)
    , Options(InOptions)
    , Lifetime(InLifetime)
    , bManaged((InOptions & MTLResourceStorageModeMask) == MTLResourceStorageModeManaged)
    , bBindlessReachable(bInBindlessReachable)
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

    const uint64 AlignedOffset = Math::AlignUp(Page->Offset, ResolvedAlignment);

    if (AlignedOffset + SizeInBytes > Page->Size)
    {
        METAL_ERROR("Upload page of %llu bytes cannot fit a %llu byte allocation", Page->Size, SizeInBytes);
        return nullptr;
    }

    if (bManaged)
    {
        Page->DirtyBegin = Math::Min(Page->DirtyBegin, AlignedOffset);
        Page->DirtyEnd   = Math::Max(Page->DirtyEnd, AlignedOffset + SizeInBytes);
    }

    Page->Offset                 = AlignedOffset + SizeInBytes;
    Page->UsedBytes             += SizeInBytes;
    Page->Queue                  = Queue;
    Page->EligibleFromFenceValue = UINT64_MAX;
    Page->bPendingRetire         = bDedicated && Lifetime == EMetalAllocationLifetime::Submission;

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

    const bool bFrameLifetime = Lifetime == EMetalAllocationLifetime::Frame;

    if (!bFrameLifetime && ActivePage && ActivePage->Queue == Queue && ActivePage->UsedBytes > 0)
    {
        ActivePage->bPendingRetire         = true;
        ActivePage->EligibleFromFenceValue = SubmissionValue;
        ActivePage                         = nullptr;
    }

    for (FPage* Page : Pages)
    {
        if (!Page || Page->Queue != Queue)
        {
            continue;
        }

        if (Page->DirtyEnd > Page->DirtyBegin)
        {
            [Page->Buffer didModifyRange:NSMakeRange(Page->DirtyBegin, Page->DirtyEnd - Page->DirtyBegin)];
            Page->DirtyBegin = UINT64_MAX;
            Page->DirtyEnd   = 0;
        }

        const bool bOpenFramePage = bFrameLifetime && !Page->bPendingRetire && Page->UsedBytes > 0;

        if (bOpenFramePage || (Page->bPendingRetire && Page->EligibleFromFenceValue == UINT64_MAX))
        {
            Page->EligibleFromFenceValue = SubmissionValue;
        }
    }
}

void FMetalLinearAllocator::EndFrame()
{
    if (Lifetime != EMetalAllocationLifetime::Frame)
    {
        return;
    }

    TScopedLock Lock(AllocatorsCS);

    for (FPage* Page : Pages)
    {
        if (Page && !Page->bPendingRetire && Page->UsedBytes > 0)
        {
            Page->bPendingRetire = true;
        }
    }

    ActivePage = nullptr;
}

void FMetalLinearAllocator::CleanUp()
{
    TScopedLock Lock(AllocatorsCS);
    RecycleRetiredPages();
    DropUnusedPages(MaxUnusedBytes);
}

void FMetalLinearAllocator::Trim()
{
    TScopedLock Lock(AllocatorsCS);
    RecycleRetiredPages();
    DropUnusedPages(0);
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

    id<MTLBuffer> Buffer = [DeviceHandle newBufferWithLength:PageSize options:Options];

    if (!Buffer)
    {
        return nullptr;
    }

    Buffer.label = bDedicated ? @"MetalUploadDedicated" : @"MetalUploadPage";
    GetDevice()->GetResidencySet().Add(Buffer, bBindlessReachable);

    FPage* Page = new FPage();
    Page->Buffer                   = Buffer;
    Page->Size                     = PageSize;
    Page->Offset                   = 0;
    Page->UsedBytes                = 0;
    Page->Queue                    = nullptr;
    Page->EligibleFromFenceValue   = UINT64_MAX;
    Page->DirtyBegin               = UINT64_MAX;
    Page->DirtyEnd                 = 0;
    Page->bDedicated               = bDedicated;
    Page->bPendingRetire           = false;

    STAT_ADD(STAT_Metal_UploadPagesCreated, 1);
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

        if (ActivePage->UsedBytes > 0 && Lifetime == EMetalAllocationLifetime::Submission)
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
        GetDevice()->GetResidencySet().Remove(Page->Buffer);
        [Page->Buffer release];
        Page->Buffer = nil;
    }

    STAT_ADD(STAT_Metal_UploadPagesReleased, 1);
    delete Page;
}

void FMetalLinearAllocator::RecycleRetiredPages()
{
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
    }
}

void FMetalLinearAllocator::DropUnusedPages(uint64 MaxUnusedPageBytes)
{
    uint64 UnusedBytes = 0;
    for (const FPage* Page : Pages)
    {
        if (Page && !Page->bPendingRetire && Page->UsedBytes == 0)
        {
            UnusedBytes += Page->bDedicated ? (MaxUnusedPageBytes + 1) : Page->Size;
        }
    }

    if (UnusedBytes <= MaxUnusedPageBytes)
    {
        return;
    }

    for (int32 Index = Pages.Size() - 1; Index >= 0 && UnusedBytes > MaxUnusedPageBytes; --Index)
    {
        FPage* Page = Pages[Index];

        if (!Page || Page->bPendingRetire || Page->UsedBytes != 0 || ActivePage == Page)
        {
            continue;
        }

        UnusedBytes -= Math::Min(UnusedBytes, Page->bDedicated ? (MaxUnusedPageBytes + 1) : Page->Size);
        METAL_INFO("Dropping unused Metal upload page (%llu bytes)", Page->Size);
        ReleasePage(Page);
        Pages.RemoveAt(Index);
    }
}

#if METAL_ENABLE_STATS
void FMetalLinearAllocator::UpdateMemoryStats(FMetalAllocatorUsage& OutUsage) const
{
    TScopedLock Lock(AllocatorsCS);

    OutUsage = FMetalAllocatorUsage();
    for (const FPage* Page : Pages)
    {
        if (!Page)
        {
            continue;
        }

        OutUsage.AllocatedBytes += Page->Size;
        OutUsage.UsedBytes      += Page->UsedBytes;
        OutUsage.NumBlocks      += 1;
    }

    if (OutUsage.AllocatedBytes > OutUsage.UsedBytes)
    {
        OutUsage.FragmentedBytes = OutUsage.AllocatedBytes - OutUsage.UsedBytes;
    }
}
#endif

FMetalUploadHeapAllocator::FMetalUploadHeapAllocator(FMetalDevice* InDevice, uint64 InPageSizeBytes, uint64 InLargeAllocationThreshold)
    : FMetalDeviceChild(InDevice)
    , UploadAllocator(InDevice, InPageSizeBytes, InLargeAllocationThreshold, MetalRHI::GetMTLResourceOptions(EMetalMemoryClass::CPUWriteGPURead), true, EMetalAllocationLifetime::Frame)
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

void FMetalUploadHeapAllocator::RetireAllocations(FMetalQueue* Queue, uint64 SubmissionValue)
{
    UploadAllocator.RetireAllocations(Queue, SubmissionValue);
}

void FMetalUploadHeapAllocator::EndFrame()
{
    UploadAllocator.EndFrame();
}

void FMetalUploadHeapAllocator::CleanUp()
{
    UploadAllocator.CleanUp();

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

void FMetalUploadHeapAllocator::Trim()
{
    UploadAllocator.Trim();
}

void FMetalUploadHeapAllocator::Destroy()
{
    UploadAllocator.Destroy();
}

#if METAL_ENABLE_STATS
void FMetalUploadHeapAllocator::UpdateMemoryStats()
{
    FMetalAllocatorUsage UploadUsage;
    UploadAllocator.UpdateMemoryStats(UploadUsage);

    STAT_SET(STAT_Metal_UploadHeapAllocated,  UploadUsage.AllocatedBytes);
    STAT_SET(STAT_Metal_UploadHeapUsed,       UploadUsage.UsedBytes);
    STAT_SET(STAT_Metal_UploadHeapFragmented, UploadUsage.FragmentedBytes);
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

bool FMetalBufferAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, MTLResourceOptions Options, bool bBindlessReachable, FMetalResourceStorage& OutStorage)
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

        if (SizeAndAlign.size <= GMaxHeapAllocationSize && HeapPool.TryAllocate(SizeAndAlign.size, SizeAndAlign.align, &OutStorage, HeapIndex, Offset, Heap))
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

    OutStorage.InitStandalone(Buffer, SizeInBytes, bBindlessReachable);
    return true;
}

void FMetalBufferAllocator::Deallocate(FMetalResourceStorage& Storage)
{
    if (Storage.GetStorageType() == EMetalResourceStorageType::SuballocatedHeap)
    {
        HeapPool.Deallocate(Storage.GetHeapIndex(), Storage.GetResourceOffset(), Storage.GetSize());
    }
}

uint32 FMetalBufferAllocator::RecordDefragMoves(FMetalCommandContext& Context, uint32 MaxMoves, uint64 EligibleBeforeFrame, TArray<FMetalDefragMove>& OutMoves)
{
    if (MaxMoves == 0)
    {
        return 0;
    }

    TArray<FMetalDefragCandidate> Candidates;
    HeapPool.SelectDefragCandidates(MaxMoves, EligibleBeforeFrame, Candidates);

    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();
    uint32        NumMoves     = 0;

    for (const FMetalDefragCandidate& Candidate : Candidates)
    {
        id<MTLBuffer>            OldBuffer    = Candidate.Storage->GetBuffer();
        const NSUInteger         Length       = OldBuffer.length;
        const MTLResourceOptions Options      = OldBuffer.resourceOptions;
        const MTLSizeAndAlign    SizeAndAlign = [DeviceHandle heapBufferSizeAndAlignWithLength:Length options:Options];
        const uint64             Alignment    = Math::Max<uint64>(SizeAndAlign.align, BUFFER_ALIGNMENT);

        TUniquePtr<FMetalResourceStorage> Target = MakeUniquePtr<FMetalResourceStorage>(GetDevice());

        uint32      HeapIndex = UINT32_MAX;
        uint64      Offset    = 0;
        FMetalHeap* Heap      = nullptr;

        if (!HeapPool.TryAllocateForDefrag(Candidate.Storage->GetSize(), Alignment, Candidate.HeapIndex, Target.Get(), HeapIndex, Offset, Heap))
        {
            continue;
        }

        id<MTLBuffer> NewBuffer = [Heap->GetMTLHeap() newBufferWithLength:Length options:Options offset:Offset];

        if (!NewBuffer)
        {
            HeapPool.Deallocate(HeapIndex, Offset, Candidate.Storage->GetSize());
            continue;
        }

        NewBuffer.label = OldBuffer.label;
        Target->InitSuballocatedHeap(NewBuffer, Heap, Offset, Candidate.Storage->GetSize(), HeapIndex, this);

        [Context.GetEncoders().RequireBlitEncoder() copyFromBuffer:OldBuffer sourceOffset:0 toBuffer:NewBuffer destinationOffset:0 size:Length];

        STAT_ADD(STAT_Metal_DefragBytesMoved, Candidate.Storage->GetSize());
        Candidate.Storage->SetDefragPending(true);

        FMetalDefragMove& Move = OutMoves.Emplace();
        Move.Storage = Candidate.Storage;
        Move.Target  = ::Move(Target);
        ++NumMoves;
    }

    return NumMoves;
}

void FMetalBufferAllocator::RetargetAllocation(FMetalResourceStorage& Storage)
{
    HeapPool.RetargetAllocation(Storage.GetHeapIndex(), Storage.GetResourceOffset(), &Storage);
}

void FMetalBufferAllocator::CleanUp()
{
    HeapPool.CleanUp();

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

void FMetalBufferAllocator::Trim()
{
    HeapPool.Trim();
}

void FMetalBufferAllocator::Destroy()
{
    HeapPool.Destroy();
}

#if METAL_ENABLE_STATS
void FMetalBufferAllocator::UpdateMemoryStats()
{
    FMetalAllocatorUsage Usage;
    HeapPool.UpdateMemoryStats(Usage);

    STAT_SET(STAT_Metal_BufferHeapAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Metal_BufferHeapUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Metal_BufferHeapFragmented, Usage.FragmentedBytes);
    STAT_SET(STAT_Metal_BufferHeaps,          Usage.NumBlocks);
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
    TextureDescriptor.hazardTrackingMode = MTLHazardTrackingModeUntracked;

    if (TextureDescriptor.storageMode == MTLStorageModePrivate)
    {
        const MTLSizeAndAlign SizeAndAlign = [DeviceHandle heapTextureSizeAndAlignWithDescriptor:TextureDescriptor];
        uint32      HeapIndex = UINT32_MAX;
        uint64      Offset    = 0;
        FMetalHeap* Heap      = nullptr;

        if (SizeAndAlign.size > 0 && SizeAndAlign.size <= GMaxHeapAllocationSize && HeapPool.TryAllocate(SizeAndAlign.size, SizeAndAlign.align, &OutStorage, HeapIndex, Offset, Heap))
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

    const bool bBindlessReachable = (TextureDescriptor.usage & (MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite)) != 0;
    OutStorage.InitStandalone(Texture, 0, bBindlessReachable);
    return true;
}

void FMetalTextureAllocator::Deallocate(FMetalResourceStorage& Storage)
{
    if (Storage.GetStorageType() == EMetalResourceStorageType::SuballocatedHeap)
    {
        HeapPool.Deallocate(Storage.GetHeapIndex(), Storage.GetResourceOffset(), Storage.GetSize());
    }
}

uint32 FMetalTextureAllocator::RecordDefragMoves(FMetalCommandContext& Context, uint32 MaxMoves, uint64 EligibleBeforeFrame, TArray<FMetalDefragMove>& OutMoves)
{
    if (MaxMoves == 0)
    {
        return 0;
    }

    TArray<FMetalDefragCandidate> Candidates;
    HeapPool.SelectDefragCandidates(MaxMoves, EligibleBeforeFrame, Candidates);

    FMetalEncoderManager& Encoders     = Context.GetEncoders();
    id<MTLDevice>         DeviceHandle = GetDevice()->GetMTLDevice();
    uint32                NumMoves     = 0;

    for (const FMetalDefragCandidate& Candidate : Candidates)
    {
        id<MTLTexture> OldTexture = Candidate.Storage->GetTexture();

        if (OldTexture.sampleCount > 1 || MetalRHI::IsStencilPixelFormat(OldTexture.pixelFormat))
        {
            continue;
        }

        MTLTextureDescriptor* Descriptor   = CreateDescriptorFromTexture(OldTexture);
        const MTLSizeAndAlign SizeAndAlign = [DeviceHandle heapTextureSizeAndAlignWithDescriptor:Descriptor];

        TUniquePtr<FMetalResourceStorage> Target = MakeUniquePtr<FMetalResourceStorage>(GetDevice());

        uint32      HeapIndex = UINT32_MAX;
        uint64      Offset    = 0;
        FMetalHeap* Heap      = nullptr;

        if (SizeAndAlign.size == 0 || !HeapPool.TryAllocateForDefrag(SizeAndAlign.size, SizeAndAlign.align, Candidate.HeapIndex, Target.Get(), HeapIndex, Offset, Heap))
        {
            continue;
        }

        id<MTLTexture> NewTexture = [Heap->GetMTLHeap() newTextureWithDescriptor:Descriptor offset:Offset];

        if (!NewTexture)
        {
            HeapPool.Deallocate(HeapIndex, Offset, SizeAndAlign.size);
            continue;
        }

        NewTexture.label = OldTexture.label;
        Target->InitSuballocatedHeap(NewTexture, Heap, Offset, SizeAndAlign.size, HeapIndex, this);

        TArray<MTLRenderPassDescriptor*> InitPasses;
        MetalRHI::CreatePlacementInitPasses(NewTexture, InitPasses);

        for (MTLRenderPassDescriptor* InitPass : InitPasses)
        {
            Encoders.EncodeLoadStorePass(InitPass, "DefragInitialize");
        }

        Encoders.FlushPendingClears(OldTexture);
        [Encoders.RequireBlitEncoder() copyFromTexture:OldTexture toTexture:NewTexture];

        STAT_ADD(STAT_Metal_DefragBytesMoved, SizeAndAlign.size);
        Candidate.Storage->SetDefragPending(true);

        FMetalDefragMove& Move = OutMoves.Emplace();
        Move.Storage = Candidate.Storage;
        Move.Target  = ::Move(Target);
        ++NumMoves;
    }

    return NumMoves;
}

void FMetalTextureAllocator::RetargetAllocation(FMetalResourceStorage& Storage)
{
    HeapPool.RetargetAllocation(Storage.GetHeapIndex(), Storage.GetResourceOffset(), &Storage);
}

void FMetalTextureAllocator::CleanUp()
{
    HeapPool.CleanUp();

#if METAL_ENABLE_STATS
    UpdateMemoryStats();
#endif
}

void FMetalTextureAllocator::Trim()
{
    HeapPool.Trim();
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
    STAT_SET(STAT_Metal_TextureHeapAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Metal_TextureHeapUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Metal_TextureHeapFragmented, Usage.FragmentedBytes);
    STAT_SET(STAT_Metal_TextureHeaps,          Usage.NumBlocks);
}
#endif
