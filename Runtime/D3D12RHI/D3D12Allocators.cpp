#include "Core/Math/Math.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12DeletionQueue.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"

FD3D12BuddyAllocator::FD3D12BuddyAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(Math::Max<uint64>(InMinBlockBytes, 256ull))
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , BackingHeap(nullptr)
    , BackingResource(nullptr)
    , FreeOffsets()
    , MappedBaseAddress(nullptr)
    , BaseGpuVirtualAddress(0)
    , AllocatorCS()
{
}

FD3D12BuddyAllocator::~FD3D12BuddyAllocator()
{
    Shutdown();
}

bool FD3D12BuddyAllocator::Initialize()
{
    SCOPED_LOCK(AllocatorCS);
    Shutdown();
    if (!GetDevice())
    {
        return false;
    }

    uint64 BackingSize = MinBlockBytes;
    while (BackingSize < PageSizeBytes)
    {
        BackingSize <<= 1;
    }

    PageSizeBytes = BackingSize;
    
    BackingHeap           = nullptr;
    BackingResource       = nullptr;
    MappedBaseAddress     = nullptr;
    BaseGpuVirtualAddress = 0;
    
    if (AllocationStrategy == EAllocationStrategy::SuballocatedHeap)
    {
        D3D12_HEAP_DESC HeapDesc = {};
        HeapDesc.Properties.Type                 = HeapType;
        HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapDesc.Properties.VisibleNodeMask      = 1;
        HeapDesc.Properties.CreationNodeMask     = 1;
        HeapDesc.SizeInBytes                     = PageSizeBytes;
        HeapDesc.Alignment                       = 0;
        HeapDesc.Flags                           = D3D12_HEAP_FLAG_NONE;

        FD3D12HeapRef NewHeap;
        if (!GetDevice()->CreateHeap(HeapDesc, NewHeap))
        {
            return false;
        }

        BackingHeap = NewHeap;
    }
    else
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = PageSizeBytes;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, NewResource))
        {
            return false;
        }

        BackingResource = NewResource;
    }

    if (BackingResource)
    {
        BaseGpuVirtualAddress = BackingResource->GetGPUVirtualAddress();
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = static_cast<uint8*>(BackingResource->MapRange(0, nullptr));
        }
    }
    
    const uint32 MaxOrder = GetOrderForSize(PageSizeBytes);
    FreeOffsets.Resize(MaxOrder + 1);
    FreeOffsets[MaxOrder].Add(0);

    return true;
}

void FD3D12BuddyAllocator::Shutdown()
{
    BackingResource = nullptr;
    BackingHeap     = nullptr;

    FreeOffsets.Clear();

    MappedBaseAddress     = nullptr;
    BaseGpuVirtualAddress = 0;
}

uint32 FD3D12BuddyAllocator::GetOrderForSize(uint64 Size) const
{
    uint32 Order     = 0;
    uint64 BlockSize = MinBlockBytes;
    
    while (BlockSize < Size)
    {
        BlockSize <<= 1;
        ++Order;
    }

    return Order;
}

uint64 FD3D12BuddyAllocator::GetOrderBlockSize(uint32 Order) const
{
    return MinBlockBytes << Order;
}

bool FD3D12BuddyAllocator::TryAllocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Size == 0 || FreeOffsets.IsEmpty())
    {
        return false;
    }

    const uint64 UsedAlignment  = Alignment ? Alignment : MinBlockBytes;
    const uint64 AllocationSize = Math::AlignUp<uint64>(Math::Max(Size, UsedAlignment), MinBlockBytes);

    SCOPED_LOCK(AllocatorCS);

    uint64 Offset = 0;
    uint32 Order  = 0;
    {
        const uint64 RequestedSize = Math::Max(AllocationSize, UsedAlignment);
        const uint32 TargetOrder   = GetOrderForSize(RequestedSize);

        bool bAllocated = false;
        for (uint32 CurrentOrder = TargetOrder; CurrentOrder < static_cast<uint32>(FreeOffsets.Size()); ++CurrentOrder)
        {
            TArray<uint64>& CurrentList = FreeOffsets[CurrentOrder];
            if (CurrentList.IsEmpty())
            {
                continue;
            }

            const int32 LastIndex = CurrentList.Size() - 1;
            uint64 BlockOffset    = CurrentList[LastIndex];
            CurrentList.Pop();

            while (CurrentOrder > TargetOrder)
            {
                --CurrentOrder;
                const uint64 SplitBlockSize = GetOrderBlockSize(CurrentOrder);
                FreeOffsets[CurrentOrder].Add(BlockOffset + SplitBlockSize);
            }

            Offset     = BlockOffset;
            Order      = TargetOrder;
            bAllocated = true;
            break;
        }

        if (!bAllocated)
        {
            return false;
        }
    }

    const uint64 BlockSize = GetOrderBlockSize(Order);
    OutStorage.Reset();
    OutStorage.SetSize(BlockSize);

    if (BackingResource)
    {
        OutStorage.SetResource(BackingResource);
        OutStorage.SetResourceOffset(Offset);
        OutStorage.SetGpuVirtualAddress(BaseGpuVirtualAddress + Offset);
        OutStorage.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + Offset) : nullptr);
    }
    else
    {
        OutStorage.SetResourceOffset(Offset);
        OutStorage.SetGpuVirtualAddress(0);
        OutStorage.SetMappedBaseAddress(nullptr);
    }

    FD3D12BuddyAllocatorAllocationData AllocationData = {};
    AllocationData.Order         = Order;
    AllocationData.Offset        = Offset;
    AllocationData.bBackedByHeap = (AllocationStrategy == EAllocationStrategy::SuballocatedHeap);
    AllocationData.BackingHeap   = BackingHeap.Get();
    
    OutStorage.SetBuddyAllocationData(AllocationData);
    OutStorage.SetBuddyAllocator(this);
    return true;
}

void FD3D12BuddyAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12BuddyAllocatorAllocationData AllocationData = Storage.GetBuddyAllocationData();
    FD3D12RHI::DeferDeletion(ED3D12DeferredAllocatorType::Buddy, this, AllocationData);
}

void FD3D12BuddyAllocator::RecycleAllocation(const FD3D12BuddyAllocatorAllocationData& AllocationData)
{
    SCOPED_LOCK(AllocatorCS);

    if (FreeOffsets.IsEmpty())
    {
        return;
    }

    const uint32 MaxOrder = static_cast<uint32>(FreeOffsets.Size() - 1);
    uint64 CurrentOffset  = AllocationData.Offset;
    uint32 CurrentOrder   = AllocationData.Order;

    while (CurrentOrder < MaxOrder)
    {
        const uint64 BlockSize   = GetOrderBlockSize(CurrentOrder);
        const uint64 BuddyOffset = CurrentOffset ^ BlockSize;
        TArray<uint64>& FreeList = FreeOffsets[CurrentOrder];

        int32 BuddyIndex = -1;
        for (int32 Index = 0; Index < FreeList.Size(); ++Index)
        {
            if (FreeList[Index] == BuddyOffset)
            {
                BuddyIndex = Index;
                break;
            }
        }

        if (BuddyIndex < 0)
        {
            break;
        }

        FreeList.RemoveAtSwap(BuddyIndex);
        CurrentOffset = Math::Min(CurrentOffset, BuddyOffset);
        ++CurrentOrder;
    }

    FreeOffsets[CurrentOrder].Add(CurrentOffset);
}

FD3D12MultiBuddyAllocator::FD3D12MultiBuddyAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , Allocators()
    , AllocatorsCS()
{
}

FD3D12MultiBuddyAllocator::~FD3D12MultiBuddyAllocator()
{
    Shutdown();
}

bool FD3D12MultiBuddyAllocator::Initialize()
{
    Shutdown();
    return CreateAllocator();
}

void FD3D12MultiBuddyAllocator::Shutdown()
{
    SCOPED_LOCK(AllocatorsCS);

    for (FD3D12BuddyAllocator* Allocator : Allocators)
    {
        delete Allocator;
    }

    Allocators.Clear();
}

bool FD3D12MultiBuddyAllocator::CreateAllocator()
{
    FD3D12BuddyAllocator* Allocator = new FD3D12BuddyAllocator(GetDevice(), PageSizeBytes, MinBlockBytes, HeapType, InitialState, AllocationStrategy);
    if (!Allocator->Initialize())
    {
        delete Allocator;
        return false;
    }

    Allocators.Add(Allocator);
    return true;
}

bool FD3D12MultiBuddyAllocator::TryAllocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(AllocatorsCS);

    for (FD3D12BuddyAllocator* Allocator : Allocators)
    {
        if (Allocator && Allocator->TryAllocate(Size, Alignment, OutStorage))
        {
            return true;
        }
    }

    if (!CreateAllocator())
    {
        return false;
    }

    FD3D12BuddyAllocator* Allocator = Allocators[Allocators.Size() - 1];
    if (!Allocator || !Allocator->TryAllocate(Size, Alignment, OutStorage))
    {
        return false;
    }

    return true;
}

FD3D12PoolAllocatorPage::FD3D12PoolAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , UsedBytes(0)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , BackingHeap(nullptr)
    , BackingResource(nullptr)
    , MappedBaseAddress(nullptr)
    , BaseGpuVirtualAddress(0)
    , FreeRanges()
{
}

FD3D12PoolAllocatorPage::~FD3D12PoolAllocatorPage()
{
}

bool FD3D12PoolAllocatorPage::Initialize()
{
    if (!GetDevice())
    {
        return false;
    }

    BackingHeap           = nullptr;
    BackingResource       = nullptr;
    MappedBaseAddress     = nullptr;
    BaseGpuVirtualAddress = 0;
    UsedBytes             = 0;

    if (AllocationStrategy == EAllocationStrategy::SuballocatedHeap)
    {
        D3D12_HEAP_DESC HeapDesc = {};
        HeapDesc.Properties.Type                 = HeapType;
        HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapDesc.Properties.VisibleNodeMask      = 1;
        HeapDesc.Properties.CreationNodeMask     = 1;
        HeapDesc.SizeInBytes                     = PageSizeBytes;
        HeapDesc.Alignment                       = 0;
        HeapDesc.Flags                           = D3D12_HEAP_FLAG_NONE;

        FD3D12HeapRef NewHeap;
        if (!GetDevice()->CreateHeap(HeapDesc, NewHeap))
        {
            return false;
        }

        BackingHeap = NewHeap;
    }
    else
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = PageSizeBytes;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, NewResource))
        {
            return false;
        }

        BackingResource = NewResource;
    }

    if (BackingResource)
    {
        BaseGpuVirtualAddress = BackingResource->GetGPUVirtualAddress();
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = static_cast<uint8*>(BackingResource->MapRange(0, nullptr));
        }
    }

    FreeRanges.Clear();
    FreeRanges.Add({0, PageSizeBytes});

    return true;
}

bool FD3D12PoolAllocatorPage::TryAllocate(uint64 Size, uint64 InAlignment, uint32 InPageIndex, FD3D12ResourceStorage& OutStorage)
{
    for (int32 Index = 0; Index < FreeRanges.Size(); ++Index)
    {
        const FFreeRange Range = FreeRanges[Index];
        
        const uint64 UsedAlignment = Math::Max<uint64>(InAlignment, Alignment);
        const uint64 AlignedOffset = Math::AlignUp<uint64>(Range.Offset, UsedAlignment);
        const uint64 Padding       = AlignedOffset - Range.Offset;
        const uint64 RequiredSize  = Padding + Size;

        if (Range.Size < RequiredSize)
        {
            continue;
        }

        const uint64 TailSize = Range.Size - RequiredSize;
        if (Padding > 0)
        {
            FreeRanges[Index].Size = Padding;
            if (TailSize > 0)
            {
                FreeRanges.Add({AlignedOffset + Size, TailSize});
            }
        }
        else if (TailSize > 0)
        {
            FreeRanges[Index].Offset = AlignedOffset + Size;
            FreeRanges[Index].Size   = TailSize;
        }
        else
        {
            FreeRanges.RemoveAtSwap(Index);
        }

        UsedBytes += Size;

        OutStorage.Reset();
        OutStorage.SetSize(Size);

        if (BackingResource)
        {
            OutStorage.SetResource(BackingResource);
            OutStorage.SetGpuVirtualAddress(BackingResource->GetGPUVirtualAddress() + AlignedOffset);
        }
        else
        {
            OutStorage.SetGpuVirtualAddress(0);
        }

        OutStorage.SetResourceOffset(AlignedOffset);
        OutStorage.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + AlignedOffset) : nullptr);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex     = InPageIndex;
        AllocationData.Offset        = AlignedOffset;
        AllocationData.Size          = Size;
        AllocationData.bIsStandalone = false;
        AllocationData.bBackedByHeap = (AllocationStrategy == EAllocationStrategy::SuballocatedHeap);
        AllocationData.BackingHeap   = BackingHeap.Get();
        OutStorage.SetPoolAllocationData(AllocationData);

        return true;
    }

    return false;
}

void FD3D12PoolAllocatorPage::RecycleAllocation(uint64 Offset, uint64 Size)
{
    if (Size == 0)
    {
        return;
    }

    FreeRanges.Add({Offset, Size});
    UsedBytes = UsedBytes > Size ? (UsedBytes - Size) : 0;
    CoalesceFreeRanges();
}

void FD3D12PoolAllocatorPage::CoalesceFreeRanges()
{
    if (FreeRanges.IsEmpty())
    {
        return;
    }

    FreeRanges.SortWithPredicate([](const FFreeRange& A, const FFreeRange& B)
    {
        return A.Offset < B.Offset;
    });

    for (int32 Index = 0; Index + 1 < FreeRanges.Size();)
    {
        FFreeRange& Current = FreeRanges[Index];
        FFreeRange& Next    = FreeRanges[Index + 1];

        if (Current.Offset + Current.Size == Next.Offset)
        {
            Current.Size += Next.Size;
            FreeRanges.RemoveAt(Index + 1);
        }
        else
        {
            ++Index;
        }
    }
}

FD3D12PoolAllocator::FD3D12PoolAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxResourceSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , MaxResourceSize(InMaxResourceSize)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , FragmentedBytes(0)
    , DefragRecords()
    , Pages()
    , PagesCS()
{
}

FD3D12PoolAllocator::~FD3D12PoolAllocator()
{
    Shutdown();
}

void FD3D12PoolAllocator::Shutdown()
{
    SCOPED_LOCK(PagesCS);

    for (FD3D12PoolAllocatorPage* Page : Pages)
    {
        delete Page;
    }

    Pages.Clear();
    DefragRecords.Clear();
    FragmentedBytes = 0;
}

FD3D12PoolAllocatorPage* FD3D12PoolAllocator::CreatePage(uint64 MinimumSize, uint32& OutPageIndex)
{
    OutPageIndex = UINT32_MAX;

    if (!GetDevice())
    {
        return nullptr;
    }

    const uint64 RequiredSize   = Math::AlignUp<uint64>(MinimumSize, Alignment);
    const uint64 ActualPageSize = Math::Max(PageSizeBytes, RequiredSize);

    FD3D12PoolAllocatorPage* NewPage = new FD3D12PoolAllocatorPage(GetDevice(), ActualPageSize, Alignment, HeapType, InitialState, AllocationStrategy);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

    OutPageIndex = static_cast<uint32>(Pages.Size());
    Pages.Add(NewPage);

    return NewPage;
}

void FD3D12PoolAllocator::ComputeTLSFIndices(uint64 Size, uint32& OutFL, uint32& OutSL) const
{
    OutFL = 0;
    OutSL = 0;

    if (Size == 0)
    {
        return;
    }

    uint64 Temp       = Size;
    uint32 FirstLevel = 0;

    while (Temp > 1)
    {
        Temp >>= 1;
        ++FirstLevel;
    }

    OutFL = Math::Min<uint32>(FirstLevel, TLSFFirstLevelCount - 1);

    const uint64 RangeBase     = 1ull << OutFL;
    const uint64 OffsetInRange = Size - RangeBase;
    const uint64 SL            = (OffsetInRange * TLSFSecondLevelCount) / RangeBase;

    OutSL = Math::Min<uint32>(static_cast<uint32>(SL), TLSFSecondLevelCount - 1);
}

void FD3D12PoolAllocator::AddDefragRecord(uint32 PageIndex, uint64 Offset, uint64 Size)
{
    FDefragRecord Record = {};
    Record.PageIndex = PageIndex;
    Record.Offset    = Offset;
    Record.Size      = Size;

    DefragRecords.Add(Record);
}

bool FD3D12PoolAllocator::TryAllocate(const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InInitialState, uint64 InAlignment, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice())
    {
        return false;
    }

    uint64 Size = 0;
    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        Size = ResourceDesc.Width;
    }
    else
    {
        const D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &ResourceDesc);
        Size = AllocationInfo.SizeInBytes;
    }

    if (Size == 0)
    {
        return false;
    }

    const uint64 UsedAlignment = InAlignment ? InAlignment : Alignment;
    const uint64 SizeAligned   = Math::AlignUp<uint64>(Size, UsedAlignment);

    if (SizeAligned > MaxResourceSize)
    {
        D3D12_RESOURCE_DESC Desc = ResourceDesc;
        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && Desc.Width == 0)
        {
            Desc.Width              = SizeAligned;
        }

        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InInitialState, ClearValue, NewResource))
        {
            return false;
        }

        void* MappedBaseAddress = nullptr;
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = NewResource->MapRange(0, nullptr);
        }

        OutStorage.InitStandalone(NewResource);
        OutStorage.SetSize(SizeAligned);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(NewResource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(MappedBaseAddress);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex     = UINT32_MAX;
        AllocationData.Offset        = 0;
        AllocationData.Size          = SizeAligned;
        AllocationData.bIsStandalone = true;
        AllocationData.bBackedByHeap = false;
        AllocationData.BackingHeap   = nullptr;

        OutStorage.SetPoolAllocationData(AllocationData);
        OutStorage.SetPoolAllocator(this);

        return true;
    }

    SCOPED_LOCK(PagesCS);

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        FD3D12PoolAllocatorPage* Page = Pages[PageIndex];
        if (Page && Page->TryAllocate(SizeAligned, UsedAlignment, PageIndex, OutStorage))
        {
            OutStorage.SetPoolAllocator(this);
            return true;
        }
    }

    uint32 NewPageIndex = UINT32_MAX;

    FD3D12PoolAllocatorPage* NewPage = CreatePage(SizeAligned, NewPageIndex);
    if (!NewPage)
    {
        return false;
    }

    if (!NewPage->TryAllocate(SizeAligned, UsedAlignment, NewPageIndex, OutStorage))
    {
        return false;
    }

    OutStorage.SetPoolAllocator(this);
    return true;
}

void FD3D12PoolAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12PoolAllocatorAllocationData Data = Storage.GetPoolAllocationData();
    if (Data.bIsStandalone)
    {
        if (Storage.GetResource())
        {
            FD3D12RHI::DeferDeletion(Storage.GetResource());
        }

        return;
    }

    if (Data.bBackedByHeap && Storage.GetResource())
    {
        FD3D12RHI::DeferDeletion(Storage.GetResource());
    }

    FD3D12RHI::DeferDeletion(ED3D12DeferredAllocatorType::Pool, this, Data);
}

void FD3D12PoolAllocator::RecycleAllocation(const FD3D12PoolAllocatorAllocationData& Data)
{
    if (Data.bIsStandalone || Data.PageIndex == UINT32_MAX || Data.Size == 0)
    {
        return;
    }

    SCOPED_LOCK(PagesCS);

    if (Data.PageIndex >= static_cast<uint32>(Pages.Size()))
    {
        return;
    }

    FD3D12PoolAllocatorPage* Page = Pages[Data.PageIndex];
    if (!Page)
    {
        return;
    }

    Page->RecycleAllocation(Data.Offset, Data.Size);

    DefragRecords.Clear();
    FragmentedBytes = 0;

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        const FD3D12PoolAllocatorPage* CurrentPage = Pages[PageIndex];
        if (!CurrentPage || CurrentPage->GetFreeRanges().Size() <= 1)
        {
            continue;
        }

        for (const FD3D12PoolAllocatorPage::FFreeRange& Range : CurrentPage->GetFreeRanges())
        {
            FragmentedBytes += Range.Size;
            AddDefragRecord(PageIndex, Range.Offset, Range.Size);
        }
    }
}

FD3D12BucketAllocator::FD3D12BucketAllocator(FD3D12Device* InDevice, const TArray<uint64>& InBucketSizes, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , Buckets()
    , BucketsCS()
{
    for (uint64 BucketSize : InBucketSizes)
    {
        FBucket& Bucket = Buckets.Emplace();
        Bucket.BlockSize         = Math::Max<uint64>(BucketSize, Alignment);
        Bucket.BackingResource   = nullptr;
        Bucket.MappedBaseAddress = nullptr;
        Bucket.FreeBlocks.Clear();
    }

    Buckets.SortWithPredicate([](const FBucket& A, const FBucket& B)
    {
        return A.BlockSize < B.BlockSize;
    });
}

FD3D12BucketAllocator::~FD3D12BucketAllocator()
{
    SCOPED_LOCK(BucketsCS);
    Buckets.Clear();
}

bool FD3D12BucketAllocator::CreateBucketResource(uint32 BucketIndex, FBucket& Bucket)
{
    if (!GetDevice())
    {
        return false;
    }

    if (Bucket.BackingResource)
    {
        return true;
    }

    const uint64 BlocksPerResource = Math::Max<uint64>(1, PageSizeBytes / Bucket.BlockSize);
    const uint64 ResourceSize      = BlocksPerResource * Bucket.BlockSize;

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = ResourceSize;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    FD3D12ResourceRef Resource;
    if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
    {
        return false;
    }

    Bucket.BackingResource = Resource;
    Bucket.MappedBaseAddress = nullptr;

    if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
    {
        Bucket.MappedBaseAddress = static_cast<uint8*>(Resource->MapRange(0, nullptr));
    }

    for (uint64 BlockIndex = 0; BlockIndex < BlocksPerResource; ++BlockIndex)
    {
        FD3D12BucketAllocatorAllocationData Block = {};
        Block.BucketIndex = BucketIndex;
        Block.SlotIndex   = 0;
        Block.Offset      = BlockIndex * Bucket.BlockSize;
        Block.Size        = Bucket.BlockSize;
        Bucket.FreeBlocks.Add(Block);
    }

    return true;
}

bool FD3D12BucketAllocator::TryAllocate(uint64 Size, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(BucketsCS);

    for (uint32 BucketIndex = 0; BucketIndex < static_cast<uint32>(Buckets.Size()); ++BucketIndex)
    {
        FBucket& Bucket = Buckets[BucketIndex];
        if (Size > Bucket.BlockSize)
        {
            continue;
        }

        if (Bucket.FreeBlocks.IsEmpty() && !CreateBucketResource(BucketIndex, Bucket))
        {
            return false;
        }

        if (Bucket.FreeBlocks.IsEmpty())
        {
            return false;
        }

        const int32 LastIndex = Bucket.FreeBlocks.Size() - 1;
        FD3D12BucketAllocatorAllocationData AllocationData = Bucket.FreeBlocks[LastIndex];
        Bucket.FreeBlocks.Pop();

        OutStorage.Reset();
        OutStorage.SetResource(Bucket.BackingResource);
        OutStorage.SetResourceOffset(AllocationData.Offset);
        OutStorage.SetGpuVirtualAddress(Bucket.BackingResource ? (Bucket.BackingResource->GetGPUVirtualAddress() + AllocationData.Offset) : 0);
        OutStorage.SetMappedBaseAddress(Bucket.MappedBaseAddress ? (Bucket.MappedBaseAddress + AllocationData.Offset) : nullptr);
        OutStorage.SetSize(Bucket.BlockSize);

        AllocationData.BucketIndex = BucketIndex;
        AllocationData.SlotIndex   = 0;
        AllocationData.Size        = Bucket.BlockSize;

        OutStorage.SetBucketAllocationData(AllocationData);
        OutStorage.SetBucketAllocator(this);
        return true;
    }

    return false;
}

void FD3D12BucketAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12BucketAllocatorAllocationData AllocationData = Storage.GetBucketAllocationData();
    if (AllocationData.BucketIndex == UINT32_MAX || AllocationData.Size == 0)
    {
        return;
    }

    FD3D12RHI::DeferDeletion(ED3D12DeferredAllocatorType::Bucket, this, AllocationData);
}

void FD3D12BucketAllocator::RecycleAllocation(const FD3D12BucketAllocatorAllocationData& Data)
{
    if (Data.BucketIndex == UINT32_MAX || Data.Size == 0)
    {
        return;
    }

    SCOPED_LOCK(BucketsCS);

    if (Data.BucketIndex >= static_cast<uint32>(Buckets.Size()))
    {
        return;
    }

    FBucket& Bucket = Buckets[Data.BucketIndex];
    if (!Bucket.BackingResource)
    {
        return;
    }

    FD3D12BucketAllocatorAllocationData AllocationData = Data;
    AllocationData.SlotIndex = 0;
    AllocationData.Size      = Bucket.BlockSize;

    Bucket.FreeBlocks.Add(AllocationData);
}

FD3D12UploadHeapAllocator::FD3D12UploadHeapAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallAllocationThreshold, uint64 InLargeAllocationThreshold)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , DefaultAlignment(InAlignment)
    , SmallAllocationThreshold(InSmallAllocationThreshold)
    , LargeAllocationThreshold(InLargeAllocationThreshold)
    , SmallAllocator(InDevice, InPageSizeBytes, InAlignment, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, EAllocationStrategy::SuballocatedResource)
    , LargeAllocator(InDevice, InPageSizeBytes, InAlignment, InLargeAllocationThreshold, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, EAllocationStrategy::SuballocatedResource)
    , ConstantsAllocator(InDevice, InPageSizeBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, EAllocationStrategy::SuballocatedResource)
{
}

FD3D12UploadHeapAllocator::~FD3D12UploadHeapAllocator()
{
    Shutdown();
}

bool FD3D12UploadHeapAllocator::Initialize()
{
    Shutdown();

    if (!SmallAllocator.Initialize())
    {
        Shutdown();
        return false;
    }

    if (!ConstantsAllocator.Initialize())
    {
        Shutdown();
        return false;
    }

    return true;
}

void FD3D12UploadHeapAllocator::Shutdown()
{
    ConstantsAllocator.Shutdown();
    LargeAllocator.Shutdown();
    SmallAllocator.Shutdown();
}

void* FD3D12UploadHeapAllocator::Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    const uint64 UsedAlignment = Alignment ? Alignment : DefaultAlignment;

    if (Size <= SmallAllocationThreshold)
    {
        if (!SmallAllocator.TryAllocate(Size, UsedAlignment, OutStorage))
        {
            return nullptr;
        }
    }
    else
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = Size;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        if (!LargeAllocator.TryAllocate(Desc, D3D12_RESOURCE_STATE_GENERIC_READ, UsedAlignment, nullptr, OutStorage))
        {
            return nullptr;
        }
    }

    return OutStorage.GetMappedBaseAddress();
}

void* FD3D12UploadHeapAllocator::AllocateConstants(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    const uint64 UsedAlignment = Alignment ? Alignment : D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    if (!ConstantsAllocator.TryAllocate(Size, UsedAlignment, OutStorage))
    {
        return nullptr;
    }

    return OutStorage.GetMappedBaseAddress();
}

FD3D12LinearAllocatorPage::FD3D12LinearAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, uint32 InPageIndex)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , CurrentOffset(0)
    , ActiveAllocations(0)
    , PageIndex(InPageIndex)
    , BackingResourceStorage(InDevice)
{
}

bool FD3D12LinearAllocatorPage::Initialize()
{
    FD3D12Device* CurrentDevice = GetDevice();
    if (!CurrentDevice)
    {
        return false;
    }

    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        FD3D12UploadHeapAllocator* UploadHeapAllocator = CurrentDevice->GetUploadHeapAllocator();
        if (!UploadHeapAllocator)
        {
            return false;
        }

        UploadHeapAllocator->Allocate(PageSizeBytes, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, BackingResourceStorage);
        return BackingResourceStorage.GetResource() != nullptr;
    }

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = PageSizeBytes;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    FD3D12ResourceRef Resource;
    if (!CurrentDevice->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
    {
        return false;
    }

    void* MappedBaseAddress = nullptr;
    if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
    {
        MappedBaseAddress = Resource->MapRange(0, nullptr);
    }

    BackingResourceStorage.InitStandalone(Resource);
    BackingResourceStorage.SetSize(PageSizeBytes);
    BackingResourceStorage.SetResourceOffset(0);
    BackingResourceStorage.SetGpuVirtualAddress(Resource->GetGPUVirtualAddress());
    BackingResourceStorage.SetMappedBaseAddress(MappedBaseAddress);
    return true;
}

bool FD3D12LinearAllocatorPage::TryAllocate(uint64 Size, uint64 Alignment, uint64& OutOffset)
{
    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 AlignedOffset = Math::AlignUp<uint64>(CurrentOffset, UsedAlignment);

    if (AlignedOffset + Size > PageSizeBytes)
    {
        return false;
    }

    OutOffset     = AlignedOffset;
    CurrentOffset = AlignedOffset + Size;

    return true;
}

void FD3D12LinearAllocatorPage::ReleaseAllocation()
{
    // Linear allocations are reclaimed in page batches, not individually.
}

void FD3D12LinearAllocatorPage::Reset()
{
    BackingResourceStorage.Reset();

    CurrentOffset     = 0;
    ActiveAllocations = 0;
}

FD3D12LinearAllocator::FD3D12LinearAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , NextPageIndex(1)
    , Pages()
    , FullPages()
    , PagesCS()
{
}

FD3D12LinearAllocator::~FD3D12LinearAllocator()
{
    SCOPED_LOCK(PagesCS);

    for (FD3D12LinearAllocatorPage* Page : Pages)
    {
        RetirePage(Page);
        delete Page;
    }
    Pages.Clear();

    for (FD3D12LinearAllocatorPage* Page : FullPages)
    {
        RetirePage(Page);
        delete Page;
    }
    FullPages.Clear();
}

FD3D12LinearAllocatorPage* FD3D12LinearAllocator::CreatePage()
{
    FD3D12LinearAllocatorPage* NewPage = new FD3D12LinearAllocatorPage(GetDevice(), PageSizeBytes, HeapType, InitialState, NextPageIndex++);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

    return NewPage;
}

void FD3D12LinearAllocator::RetirePage(FD3D12LinearAllocatorPage* Page)
{
    if (!Page)
    {
        return;
    }

    FD3D12ResourceStorage& BackingResourceStorage = Page->GetBackingResourceStorage();
    if (BackingResourceStorage.GetAllocator() != nullptr)
    {
        BackingResourceStorage.ReleaseResource();
    }
    else if (BackingResourceStorage.GetResource())
    {
        FD3D12RHI::DeferDeletion(BackingResourceStorage.GetResource());
    }

    Page->Reset();
}

void* FD3D12LinearAllocator::Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Size == 0)
    {
        return nullptr;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 SizeAligned   = Math::AlignUp<uint64>(Size, UsedAlignment);

    if (SizeAligned > PageSizeBytes)
    {
        FD3D12ResourceRef Resource;

        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = SizeAligned;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
        {
            return nullptr;
        }

        void* MappedBaseAddress = nullptr;
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = Resource->MapRange(0, nullptr);
        }

        OutStorage.InitStandalone(Resource);
        OutStorage.SetSize(SizeAligned);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(Resource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(MappedBaseAddress);

        FD3D12LinearAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex         = UINT32_MAX;
        AllocationData.PageOffset        = 0;
        AllocationData.AllocationSize    = SizeAligned;
        AllocationData.bDirectAllocation = true;

        OutStorage.SetLinearAllocationData(AllocationData);
        return OutStorage.GetMappedBaseAddress();
    }

    SCOPED_LOCK(PagesCS);

    FD3D12LinearAllocatorPage* SelectedPage = nullptr;
    uint64 AllocationOffset = 0;

    for (int32 Index = 0; Index < Pages.Size();)
    {
        FD3D12LinearAllocatorPage* Page = Pages[Index];
        if (!Page)
        {
            Pages.RemoveAtSwap(Index);
            continue;
        }

        if (Page->TryAllocate(SizeAligned, UsedAlignment, AllocationOffset))
        {
            SelectedPage = Page;
            break;
        }

        if (Page->IsExhausted())
        {
            FullPages.Add(Page);
            Pages.RemoveAtSwap(Index);
            continue;
        }

        ++Index;
    }

    if (!SelectedPage)
    {
        SelectedPage = CreatePage();
        if (!SelectedPage)
        {
            return nullptr;
        }

        Pages.Add(SelectedPage);
        if (!SelectedPage->TryAllocate(SizeAligned, UsedAlignment, AllocationOffset))
        {
            return nullptr;
        }
    }

    const FD3D12ResourceStorage& BackingStorage = SelectedPage->GetBackingResourceStorage();
    const uint64 PageResourceOffset = BackingStorage.GetResourceOffset() + AllocationOffset;
    const D3D12_GPU_VIRTUAL_ADDRESS BaseGpuAddress = BackingStorage.GetGpuVirtualAddress();
    const D3D12_GPU_VIRTUAL_ADDRESS PageGpuAddress = BaseGpuAddress ? (BaseGpuAddress + AllocationOffset) : 0;
    uint8* MappedBase = static_cast<uint8*>(BackingStorage.GetMappedBaseAddress());

    OutStorage.Reset();

    if (FD3D12Resource* PageResource = BackingStorage.GetResource())
    {
        FD3D12ResourceRef ResourceRef = PageResource;
        OutStorage.SetResource(ResourceRef);
    }

    OutStorage.SetResourceOffset(PageResourceOffset);
    OutStorage.SetGpuVirtualAddress(PageGpuAddress);
    OutStorage.SetMappedBaseAddress(MappedBase ? (MappedBase + AllocationOffset) : nullptr);
    OutStorage.SetSize(SizeAligned);

    FD3D12LinearAllocatorAllocationData AllocationData = {};
    AllocationData.PageIndex         = SelectedPage->GetPageIndex();
    AllocationData.PageOffset        = AllocationOffset;
    AllocationData.AllocationSize    = SizeAligned;
    AllocationData.bDirectAllocation = false;
    OutStorage.SetLinearAllocationData(AllocationData);

    if (SelectedPage->IsExhausted())
    {
        for (int32 Index = 0; Index < Pages.Size(); ++Index)
        {
            if (Pages[Index] == SelectedPage)
            {
                FullPages.Add(SelectedPage);
                Pages.RemoveAtSwap(Index);
                break;
            }
        }
    }

    return OutStorage.GetMappedBaseAddress();
}

void FD3D12LinearAllocator::BeginFrame()
{
    SCOPED_LOCK(PagesCS);
    for (int32 Index = FullPages.Size() - 1; Index >= 0; --Index)
    {
        FD3D12LinearAllocatorPage* Page = FullPages[Index];
        if (!Page)
        {
            FullPages.RemoveAtSwap(Index);
            continue;
        }

        FD3D12ResourceStorage& BackingStorage = Page->GetBackingResourceStorage();
        FD3D12Resource* BackingResource = BackingStorage.GetResource();
        if (BackingResource)
        {
            const int32 StableRefCount = BackingStorage.GetAllocator() ? 2 : 1;
            if (BackingResource->GetRefCount() > StableRefCount)
            {
                continue;
            }
        }

        RetirePage(Page);
        if (Page->Initialize())
        {
            Pages.Add(Page);
        }
        else
        {
            delete Page;
        }

        FullPages.RemoveAtSwap(Index);
    }
}

FD3D12DynamicConstantsAllocator::FD3D12DynamicConstantsAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , LinearAllocator(nullptr)
{
}

FD3D12DynamicConstantsAllocator::~FD3D12DynamicConstantsAllocator()
{
    Shutdown();
}

bool FD3D12DynamicConstantsAllocator::Initialize(uint64 InPageSizeBytes, FD3D12UploadHeapAllocator* InUploadHeapAllocator)
{
    UNREFERENCED_VARIABLE(InUploadHeapAllocator);

    Shutdown();
    LinearAllocator = new FD3D12LinearAllocator(GetDevice(), InPageSizeBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    return LinearAllocator != nullptr;
}

void FD3D12DynamicConstantsAllocator::Shutdown()
{
    if (LinearAllocator)
    {
        delete LinearAllocator;
        LinearAllocator = nullptr;
    }
}

void FD3D12DynamicConstantsAllocator::BeginFrame()
{
    if (LinearAllocator)
    {
        LinearAllocator->BeginFrame();
    }
}

void* FD3D12DynamicConstantsAllocator::Allocate(uint64 Size, FD3D12ResourceStorage& OutStorage)
{
    if (!LinearAllocator)
    {
        return nullptr;
    }

    return LinearAllocator->Allocate(Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, OutStorage);
}

void FD3D12DynamicConstantsAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetResource())
    {
        FD3D12RHI::DeferDeletion(Storage.GetResource());
    }
}

FD3D12BufferAllocatorPool::FD3D12BufferAllocatorPool(FD3D12Device* InDevice, D3D12_HEAP_TYPE InHeapType, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12DeviceChild(InDevice)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
    , MultiBuddyAllocator(nullptr)
{
}

FD3D12BufferAllocatorPool::~FD3D12BufferAllocatorPool()
{
    ReleaseAllocator();
}

bool FD3D12BufferAllocatorPool::Initialize()
{
    ReleaseAllocator();

    MultiBuddyAllocator = new FD3D12MultiBuddyAllocator(GetDevice(), PageSizeBytes, MinBlockBytes, HeapType, InitialState, EAllocationStrategy::SuballocatedResource);
    if (!MultiBuddyAllocator || !MultiBuddyAllocator->Initialize())
    {
        ReleaseAllocator();
        return false;
    }

    return true;
}

void FD3D12BufferAllocatorPool::ReleaseAllocator()
{
    if (MultiBuddyAllocator)
    {
        MultiBuddyAllocator->Shutdown();
        delete MultiBuddyAllocator;
        MultiBuddyAllocator = nullptr;
    }
}

bool FD3D12BufferAllocatorPool::Supports(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc) const
{
    return InHeapType == HeapType && ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
}

bool FD3D12BufferAllocatorPool::TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InInitialState, uint64 InAlignment, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || !Supports(InHeapType, ResourceDesc))
    {
        return false;
    }

    const uint64 Size          = ResourceDesc.Width;
    const uint64 UsedAlignment = InAlignment ? InAlignment : 16;

    if (Size > MaxSuballocationSize)
    {
        FD3D12ResourceRef Resource;
        if (!GetDevice()->CreateCommittedResource(ResourceDesc, InHeapType, InInitialState, nullptr, Resource))
        {
            return false;
        }

        void* MappedBaseAddress = nullptr;
        if (InHeapType == D3D12_HEAP_TYPE_UPLOAD || InHeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = Resource->MapRange(0, nullptr);
        }

        OutStorage.InitStandalone(Resource);
        OutStorage.SetSize(Size);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(Resource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(MappedBaseAddress);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex     = UINT32_MAX;
        AllocationData.Size          = Size;
        AllocationData.bIsStandalone = true;
        
        OutStorage.SetPoolAllocationData(AllocationData);
        return true;
    }

    if (!MultiBuddyAllocator || !MultiBuddyAllocator->TryAllocate(Size, UsedAlignment, OutStorage))
    {
        return false;
    }

    return true;
}

void FD3D12BufferAllocatorPool::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetAllocatorType() == ED3D12AllocatorType::BuddyAllocator && Storage.GetAllocator())
    {
        static_cast<FD3D12BuddyAllocator*>(Storage.GetAllocator())->Deallocate(Storage);
        return;
    }

    if (Storage.GetAllocatorType() == ED3D12AllocatorType::PoolAllocator && Storage.GetAllocator())
    {
        static_cast<FD3D12PoolAllocator*>(Storage.GetAllocator())->Deallocate(Storage);
        return;
    }

    if (Storage.GetResource())
    {
        FD3D12RHI::DeferDeletion(Storage.GetResource());
    }
}

FD3D12BufferAllocator::FD3D12BufferAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
{
}

FD3D12BufferAllocator::~FD3D12BufferAllocator()
{
    ReleasePools();
}

bool FD3D12BufferAllocator::Initialize()
{
    ReleasePools();

    const D3D12_HEAP_TYPE HeapTypes[] =
    {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_HEAP_TYPE_READBACK
    };

    const D3D12_RESOURCE_STATES InitialStates[] =
    {
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_STATE_COPY_DEST
    };

    for (uint32 Index = 0; Index < ARRAY_COUNT(HeapTypes); ++Index)
    {
        FD3D12BufferAllocatorPool* Pool = new FD3D12BufferAllocatorPool(GetDevice(), HeapTypes[Index], PageSizeBytes, MinBlockBytes, MaxSuballocationSize, InitialStates[Index]);
        if (!Pool->Initialize())
        {
            delete Pool;
            ReleasePools();
            return false;
        }

        Pools.Add(Pool);
    }

    return true;
}

void FD3D12BufferAllocator::ReleasePools()
{
    SCOPED_LOCK(PoolsCS);
    for (FD3D12BufferAllocatorPool* Pool : Pools)
    {
        delete Pool;
    }

    Pools.Clear();
}

bool FD3D12BufferAllocator::TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(PoolsCS);

    for (FD3D12BufferAllocatorPool* Pool : Pools)
    {
        if (!Pool->Supports(InHeapType, ResourceDesc))
        {
            continue;
        }

        if (Pool->TryAllocate(InHeapType, ResourceDesc, InitialState, Alignment, OutStorage))
        {
            return true;
        }
    }

    FD3D12BufferAllocatorPool* NewPool = new FD3D12BufferAllocatorPool(GetDevice(), InHeapType, PageSizeBytes, MinBlockBytes, MaxSuballocationSize, InitialState);
    if (!NewPool->Initialize())
    {
        delete NewPool;
        return false;
    }

    Pools.Add(NewPool);
    return NewPool->TryAllocate(InHeapType, ResourceDesc, InitialState, Alignment, OutStorage);
}

void FD3D12BufferAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetAllocatorType() == ED3D12AllocatorType::BufferAllocatorPool && Storage.GetAllocator())
    {
        static_cast<FD3D12BufferAllocatorPool*>(Storage.GetAllocator())->Deallocate(Storage);
        return;
    }

    if (Storage.GetAllocatorType() == ED3D12AllocatorType::BuddyAllocator && Storage.GetAllocator())
    {
        static_cast<FD3D12BuddyAllocator*>(Storage.GetAllocator())->Deallocate(Storage);
        return;
    }

    if (Storage.GetAllocatorType() == ED3D12AllocatorType::PoolAllocator && Storage.GetAllocator())
    {
        static_cast<FD3D12PoolAllocator*>(Storage.GetAllocator())->Deallocate(Storage);
        return;
    }

    if (Storage.GetResource())
    {
        FD3D12RHI::DeferDeletion(Storage.GetResource());
    }
}

FD3D12TextureAllocator::FD3D12TextureAllocator(FD3D12Device* InDevice, uint64 InDefaultPageSizeBytes, uint64 InCommittedThreshold)
    : FD3D12DeviceChild(InDevice)
    , CommittedThreshold(InCommittedThreshold)
    , DefaultPageSizeBytes(InDefaultPageSizeBytes)
{
    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
    {
        Pools[Index] = nullptr;
    }
}

FD3D12TextureAllocator::~FD3D12TextureAllocator()
{
    SCOPED_LOCK(PoolsCS);
    ReleasePools();
}

bool FD3D12TextureAllocator::Initialize()
{
    SCOPED_LOCK(PoolsCS);
    ReleasePools();

    auto CreatePool = [this](ETexturePoolClass PoolClass, uint64 PoolAlignment) -> bool
    {
        const uint32 PoolIndex = static_cast<uint32>(PoolClass);
        if (PoolIndex >= TexturePoolClassCount)
        {
            return false;
        }

        FD3D12PoolAllocator* Pool = new FD3D12PoolAllocator(GetDevice(), DefaultPageSizeBytes, PoolAlignment, CommittedThreshold, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, EAllocationStrategy::SuballocatedHeap);
        if (!Pool)
        {
            return false;
        }

        Pools[PoolIndex] = Pool;
        return true;
    };

    if (!CreatePool(ETexturePoolClass::Small4K, D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT) ||
        !CreatePool(ETexturePoolClass::ReadOnly, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) ||
        !CreatePool(ETexturePoolClass::RenderTargetDepthStencil, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) ||
        !CreatePool(ETexturePoolClass::UAVOnly, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT))
    {
        ReleasePools();
        return false;
    }

    return true;
}

void FD3D12TextureAllocator::ReleasePools()
{
    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
    {
        if (Pools[Index])
        {
            delete Pools[Index];
            Pools[Index] = nullptr;
        }
    }
}

FD3D12TextureAllocator::ETexturePoolClass FD3D12TextureAllocator::ClassifyTexture(const D3D12_RESOURCE_DESC& Desc, uint64 Size, uint64 Alignment) const
{
    if (Size <= D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT && Alignment <= D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
    {
        return ETexturePoolClass::Small4K;
    }

    const bool bIsRTDS = (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    const bool bIsUAV  = (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;

    if (bIsRTDS)
    {
        return ETexturePoolClass::RenderTargetDepthStencil;
    }

    if (bIsUAV)
    {
        return ETexturePoolClass::UAVOnly;
    }

    return ETexturePoolClass::ReadOnly;
}

bool FD3D12TextureAllocator::TryAllocate(const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice())
    {
        return false;
    }

    const D3D12_RESOURCE_DESC Desc = ResourceDesc;
    const D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &Desc);

    if (AllocationInfo.SizeInBytes >= CommittedThreshold)
    {
        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, D3D12_HEAP_TYPE_DEFAULT, InitialState, ClearValue, NewResource))
        {
            return false;
        }

        OutStorage.InitStandalone(NewResource);
        OutStorage.SetSize(AllocationInfo.SizeInBytes);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(NewResource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(nullptr);
        OutStorage.SetTextureAllocator(this);
        return true;
    }

    SCOPED_LOCK(PoolsCS);

    const ETexturePoolClass PoolClass = ClassifyTexture(Desc, AllocationInfo.SizeInBytes, AllocationInfo.Alignment);

    const uint32 PoolIndex = static_cast<uint32>(PoolClass);
    if (PoolIndex >= TexturePoolClassCount || !Pools[PoolIndex])
    {
        return false;
    }

    D3D12_RESOURCE_DESC BlockDesc = {};
    BlockDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    BlockDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    BlockDesc.Format             = DXGI_FORMAT_UNKNOWN;
    BlockDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    BlockDesc.Width              = AllocationInfo.SizeInBytes;
    BlockDesc.Height             = 1;
    BlockDesc.DepthOrArraySize   = 1;
    BlockDesc.MipLevels          = 1;
    BlockDesc.Alignment          = 0;
    BlockDesc.SampleDesc.Count   = 1;
    BlockDesc.SampleDesc.Quality = 0;

    FD3D12ResourceStorage BlockResourceStorage(GetDevice());
    if (!Pools[PoolIndex]->TryAllocate(BlockDesc, InitialState, AllocationInfo.Alignment, nullptr, BlockResourceStorage))
    {
        return false;
    }

    FD3D12Heap* Heap = BlockResourceStorage.GetPoolAllocationData().BackingHeap;
    if (!Heap)
    {
        BlockResourceStorage.ReleaseResource();
        return false;
    }

    FD3D12ResourceRef NewResource;
    if (!GetDevice()->CreatePlacedResource(Heap, BlockResourceStorage.GetPoolAllocationData().Offset, Desc, InitialState, ClearValue, NewResource))
    {
        BlockResourceStorage.ReleaseResource();
        return false;
    }

    OutStorage.Swap(BlockResourceStorage);
    OutStorage.SetResource(NewResource);
    OutStorage.SetResourceOffset(0);
    OutStorage.SetGpuVirtualAddress(NewResource->GetGPUVirtualAddress());
    OutStorage.SetMappedBaseAddress(nullptr);
    OutStorage.SetSize(AllocationInfo.SizeInBytes);
    return true;
}

void FD3D12TextureAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetResource())
    {
        FD3D12RHI::DeferDeletion(Storage.GetResource());
    }
}
