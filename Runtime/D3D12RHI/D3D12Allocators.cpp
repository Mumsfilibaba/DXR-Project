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
    return CreateBackingAllocation();
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

bool FD3D12BuddyAllocator::AllocateFromAllocator(uint64 Size, uint64 Alignment, uint64& OutOffset, uint32& OutOrder)
{
    const uint64 RequestedSize = Math::Max(Size, Alignment);
    const uint32 TargetOrder   = GetOrderForSize(RequestedSize);

    for (uint32 CurrentOrder = TargetOrder; CurrentOrder < static_cast<uint32>(FreeOffsets.Size()); ++CurrentOrder)
    {
        TArray<uint64>& CurrentList = FreeOffsets[CurrentOrder];
        if (CurrentList.IsEmpty())
        {
            continue;
        }

        const int32 LastIndex = CurrentList.Size() - 1;
        uint64 BlockOffset = CurrentList[LastIndex];
        CurrentList.Pop();

        while (CurrentOrder > TargetOrder)
        {
            --CurrentOrder;
            const uint64 SplitBlockSize = GetOrderBlockSize(CurrentOrder);
            FreeOffsets[CurrentOrder].Add(BlockOffset + SplitBlockSize);
        }

        OutOffset = BlockOffset;
        OutOrder  = TargetOrder;
        return true;
    }

    return false;
}

void FD3D12BuddyAllocator::FreeToAllocator(uint64 Offset, uint32 Order)
{
    if (FreeOffsets.IsEmpty())
    {
        return;
    }

    const uint32 MaxOrder = static_cast<uint32>(FreeOffsets.Size() - 1);
    uint64 CurrentOffset  = Offset;
    uint32 CurrentOrder   = Order;

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

bool FD3D12BuddyAllocator::CreateBackingAllocation()
{
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

    FD3D12ResourceRef NewResource;
    if (AllocationStrategy == EAllocationStrategy::SuballocatedResource)
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
        if (!GetDevice()->CreatePlacedResource(NewHeap.Get(), 0, Desc, InitialState, nullptr, NewResource))
        {
            return false;
        }
    }

    BackingHeap           = NewHeap;
    BackingResource       = NewResource;
    MappedBaseAddress     = nullptr;
    BaseGpuVirtualAddress = 0;

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

bool FD3D12BuddyAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Request.Size == 0 || FreeOffsets.IsEmpty())
    {
        return false;
    }

    const uint64 Alignment      = Request.Alignment ? Request.Alignment : MinBlockBytes;
    const uint64 AllocationSize = Math::AlignUp<uint64>(Math::Max(Request.Size, Alignment), MinBlockBytes);

    SCOPED_LOCK(AllocatorCS);

    uint64 Offset = 0;
    uint32 Order  = 0;

    if (!AllocateFromAllocator(AllocationSize, Alignment, Offset, Order))
    {
        return false;
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
    AllocationData.PageIndex     = 0;
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
    if (AllocationData.PageIndex == UINT32_MAX)
    {
        return;
    }

    FD3D12RHI::DeferDeletion(ED3D12DeferredAllocatorType::Buddy, this, AllocationData);
}

void FD3D12BuddyAllocator::RecycleAllocation(const FD3D12BuddyAllocatorAllocationData& AllocationData)
{
    if (AllocationData.PageIndex == UINT32_MAX)
    {
        return;
    }

    SCOPED_LOCK(AllocatorCS);
    FreeToAllocator(AllocationData.Offset, AllocationData.Order);
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

bool FD3D12MultiBuddyAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(AllocatorsCS);

    for (uint32 Index = 0; Index < static_cast<uint32>(Allocators.Size()); ++Index)
    {
        if (Allocators[Index]->TryAllocate(Request, OutStorage))
        {
            const FD3D12BuddyAllocatorAllocationData BuddyData = OutStorage.GetBuddyAllocationData();

            FD3D12MultiBuddyAllocatorAllocationData Data = {};
            Data.BuddyAllocatorIndex = Index;
            Data.PageIndex           = BuddyData.PageIndex;
            Data.Order               = BuddyData.Order;
            Data.Offset              = BuddyData.Offset;
            Data.bBackedByHeap       = BuddyData.bBackedByHeap;
            Data.BackingHeap         = BuddyData.BackingHeap;

            OutStorage.SetMultiBuddyAllocationData(Data);
            OutStorage.SetMultiBuddyAllocator(this);
            return true;
        }
    }

    if (!CreateAllocator())
    {
        return false;
    }

    const uint32 Index = static_cast<uint32>(Allocators.Size() - 1);
    if (!Allocators[Index]->TryAllocate(Request, OutStorage))
    {
        return false;
    }

    const FD3D12BuddyAllocatorAllocationData BuddyData = OutStorage.GetBuddyAllocationData();
    FD3D12MultiBuddyAllocatorAllocationData Data = {};
    Data.BuddyAllocatorIndex = Index;
    Data.PageIndex           = BuddyData.PageIndex;
    Data.Order               = BuddyData.Order;
    Data.Offset              = BuddyData.Offset;
    Data.bBackedByHeap       = BuddyData.bBackedByHeap;
    Data.BackingHeap         = BuddyData.BackingHeap;

    OutStorage.SetMultiBuddyAllocationData(Data);
    OutStorage.SetMultiBuddyAllocator(this);
    return true;
}

void FD3D12MultiBuddyAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12MultiBuddyAllocatorAllocationData AllocationData = Storage.GetMultiBuddyAllocationData();
    if (AllocationData.BuddyAllocatorIndex == UINT32_MAX)
    {
        return;
    }

    FD3D12RHI::DeferDeletion(ED3D12DeferredAllocatorType::MultiBuddy, this, AllocationData);
}

void FD3D12MultiBuddyAllocator::RecycleAllocation(const FD3D12MultiBuddyAllocatorAllocationData& AllocationData)
{
    if (AllocationData.BuddyAllocatorIndex == UINT32_MAX)
    {
        return;
    }

    SCOPED_LOCK(AllocatorsCS);
    if (AllocationData.BuddyAllocatorIndex >= static_cast<uint32>(Allocators.Size()))
    {
        return;
    }

    FD3D12BuddyAllocator* Allocator = Allocators[AllocationData.BuddyAllocatorIndex];
    if (!Allocator)
    {
        return;
    }

    FD3D12BuddyAllocatorAllocationData BuddyData = {};
    BuddyData.PageIndex     = AllocationData.PageIndex;
    BuddyData.Order         = AllocationData.Order;
    BuddyData.Offset        = AllocationData.Offset;
    BuddyData.bBackedByHeap = AllocationData.bBackedByHeap;
    BuddyData.BackingHeap   = AllocationData.BackingHeap;
    Allocator->RecycleAllocation(BuddyData);
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

    FD3D12ResourceRef NewResource;
    if (AllocationStrategy == EAllocationStrategy::SuballocatedResource)
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
        if (!GetDevice()->CreatePlacedResource(NewHeap.Get(), 0, Desc, InitialState, nullptr, NewResource))
        {
            return false;
        }
    }

    BackingHeap           = NewHeap;
    BackingResource       = NewResource;
    MappedBaseAddress     = nullptr;
    BaseGpuVirtualAddress = 0;
    UsedBytes             = 0;

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

bool FD3D12PoolAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Request.Size == 0)
    {
        return false;
    }

    const uint64 UsedAlignment = Request.Alignment ? Request.Alignment : Alignment;
    const uint64 SizeAligned   = Math::AlignUp<uint64>(Request.Size, UsedAlignment);

    if (SizeAligned > MaxResourceSize && Request.bAllowCommittedFallback)
    {
        D3D12_RESOURCE_DESC Desc = {};
        if (Request.bHasResourceDesc)
        {
            Desc = Request.ResourceDesc;
        }
        else
        {
            Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            Desc.Flags              = Request.ResourceFlags;
            Desc.Format             = DXGI_FORMAT_UNKNOWN;
            Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            Desc.Width              = SizeAligned;
            Desc.Height             = 1;
            Desc.DepthOrArraySize   = 1;
            Desc.MipLevels          = 1;
            Desc.Alignment          = 0;
            Desc.SampleDesc.Count   = 1;
            Desc.SampleDesc.Quality = 0;
        }

        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, Request.InitialState, Request.ClearValue, NewResource))
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

bool FD3D12BucketAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(BucketsCS);
    for (uint32 BucketIndex = 0; BucketIndex < static_cast<uint32>(Buckets.Size()); ++BucketIndex)
    {
        FBucket& Bucket = Buckets[BucketIndex];
        if (Request.Size > Bucket.BlockSize)
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
    , SmallAllocator(nullptr)
    , LargeAllocator(nullptr)
    , ConstantsAllocator(nullptr)
{
}

FD3D12UploadHeapAllocator::~FD3D12UploadHeapAllocator()
{
    Shutdown();
}

bool FD3D12UploadHeapAllocator::Initialize()
{
    Shutdown();

    SmallAllocator = new FD3D12MultiBuddyAllocator(GetDevice(), PageSizeBytes, DefaultAlignment, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, EAllocationStrategy::SuballocatedResource);
    if (!SmallAllocator || !SmallAllocator->Initialize())
    {
        Shutdown();
        return false;
    }

    LargeAllocator = new FD3D12PoolAllocator( GetDevice(), PageSizeBytes, DefaultAlignment, LargeAllocationThreshold, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, EAllocationStrategy::SuballocatedResource);
    if (!LargeAllocator)
    {
        Shutdown();
        return false;
    }

    ConstantsAllocator = new FD3D12MultiBuddyAllocator(GetDevice(), PageSizeBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, EAllocationStrategy::SuballocatedResource);
    if (!ConstantsAllocator || !ConstantsAllocator->Initialize())
    {
        Shutdown();
        return false;
    }

    return true;
}

void FD3D12UploadHeapAllocator::Shutdown()
{
    if (ConstantsAllocator)
    {
        ConstantsAllocator->Shutdown();
        delete ConstantsAllocator;
        ConstantsAllocator = nullptr;
    }

    if (LargeAllocator)
    {
        LargeAllocator->Shutdown();
        delete LargeAllocator;
        LargeAllocator = nullptr;
    }

    if (SmallAllocator)
    {
        SmallAllocator->Shutdown();
        delete SmallAllocator;
        SmallAllocator = nullptr;
    }
}

void* FD3D12UploadHeapAllocator::Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    FD3D12ResourceAllocationRequest Request = {};
    Request.ResourceType            = ED3D12ResourceType::Buffer;
    Request.HeapType                = D3D12_HEAP_TYPE_UPLOAD;
    Request.InitialState            = D3D12_RESOURCE_STATE_GENERIC_READ;
    Request.ResourceFlags           = D3D12_RESOURCE_FLAG_NONE;
    Request.Size                    = Size;
    Request.Alignment               = Alignment ? Alignment : DefaultAlignment;
    Request.bAllowCommittedFallback = true;

    if (Size <= SmallAllocationThreshold)
    {
        if (!SmallAllocator || !SmallAllocator->TryAllocate(Request, OutStorage))
        {
            return nullptr;
        }
    }
    else
    {
        if (!LargeAllocator || !LargeAllocator->TryAllocate(Request, OutStorage))
        {
            return nullptr;
        }
    }

    return OutStorage.GetMappedBaseAddress();
}

void* FD3D12UploadHeapAllocator::AllocateConstants(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    FD3D12ResourceAllocationRequest Request = {};
    Request.ResourceType            = ED3D12ResourceType::Buffer;
    Request.HeapType                = D3D12_HEAP_TYPE_UPLOAD;
    Request.InitialState            = D3D12_RESOURCE_STATE_GENERIC_READ;
    Request.ResourceFlags           = D3D12_RESOURCE_FLAG_NONE;
    Request.Size                    = Size;
    Request.Alignment               = Alignment ? Alignment : D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    Request.bAllowCommittedFallback = true;

    if (!ConstantsAllocator || !ConstantsAllocator->TryAllocate(Request, OutStorage))
    {
        return nullptr;
    }

    return OutStorage.GetMappedBaseAddress();
}

void FD3D12UploadHeapAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetAllocatorType() == ED3D12AllocatorType::PoolAllocator)
    {
        if (!LargeAllocator)
        {
            return;
        }

        LargeAllocator->Deallocate(Storage);
        return;
    }

    if (Storage.GetAllocatorType() == ED3D12AllocatorType::MultiBuddyAllocator)
    {
        if (!SmallAllocator)
        {
            return;
        }

        SmallAllocator->Deallocate(Storage);
        return;
    }

    if (Storage.GetResource())
    {
        FD3D12RHI::DeferDeletion(Storage.GetResource());
    }
}

FD3D12LinearAllocator::FD3D12LinearAllocatorPage::FD3D12LinearAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, uint32 InPageIndex)
    : Device(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , CurrentOffset(0)
    , ActiveAllocations(0)
    , PageIndex(InPageIndex)
    , BackingResourceStorage(InDevice)
{
}

bool FD3D12LinearAllocator::FD3D12LinearAllocatorPage::Initialize()
{
    if (!Device)
    {
        return false;
    }

    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        FD3D12UploadHeapAllocator* UploadHeapAllocator = Device->GetUploadHeapAllocator();
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
    if (!Device->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
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

bool FD3D12LinearAllocator::FD3D12LinearAllocatorPage::TryAllocate(uint64 Size, uint64 Alignment, uint64& OutOffset)
{
    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 AlignedOffset = Math::AlignUp<uint64>(CurrentOffset, UsedAlignment);
    if (AlignedOffset + Size > PageSizeBytes)
    {
        return false;
    }

    OutOffset = AlignedOffset;
    CurrentOffset = AlignedOffset + Size;
    ++ActiveAllocations;
    return true;
}

void FD3D12LinearAllocator::FD3D12LinearAllocatorPage::ReleaseAllocation()
{
    if (ActiveAllocations > 0)
    {
        --ActiveAllocations;
    }
}

void FD3D12LinearAllocator::FD3D12LinearAllocatorPage::Reset()
{
    BackingResourceStorage.Reset();
    CurrentOffset = 0;
    ActiveAllocations = 0;
}

FD3D12LinearAllocator::FD3D12LinearAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , NextPageIndex(1)
    , Pages()
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
}

FD3D12LinearAllocator::FD3D12LinearAllocatorPage* FD3D12LinearAllocator::CreatePage()
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
    const uint64 SizeAligned = Math::AlignUp<uint64>(Size, UsedAlignment);

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
        OutStorage.SetLinearAllocator(this);

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

    for (FD3D12LinearAllocatorPage* Page : Pages)
    {
        if (!Page)
        {
            continue;
        }

        if (Page->TryAllocate(SizeAligned, UsedAlignment, AllocationOffset))
        {
            SelectedPage = Page;
            break;
        }
    }

    if (!SelectedPage)
    {
        SelectedPage = CreatePage();
        if (!SelectedPage)
        {
            return nullptr;
        }

        if (!SelectedPage->TryAllocate(SizeAligned, UsedAlignment, AllocationOffset))
        {
            delete SelectedPage;
            return nullptr;
        }

        Pages.Add(SelectedPage);
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
    OutStorage.SetLinearAllocator(this);

    FD3D12LinearAllocatorAllocationData AllocationData = {};
    AllocationData.PageIndex         = SelectedPage->GetPageIndex();
    AllocationData.PageOffset        = AllocationOffset;
    AllocationData.AllocationSize    = SizeAligned;
    AllocationData.bDirectAllocation = false;
    OutStorage.SetLinearAllocationData(AllocationData);

    return OutStorage.GetMappedBaseAddress();
}

void FD3D12LinearAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12LinearAllocatorAllocationData Data = Storage.GetLinearAllocationData();

    if (Data.bDirectAllocation)
    {
        if (Storage.GetResource())
        {
            FD3D12RHI::DeferDeletion(Storage.GetResource());
        }

        return;
    }

    SCOPED_LOCK(PagesCS);

    for (int32 Index = 0; Index < Pages.Size(); ++Index)
    {
        FD3D12LinearAllocatorPage* Page = Pages[Index];
        if (!Page || Page->GetPageIndex() != Data.PageIndex)
        {
            continue;
        }

        Page->ReleaseAllocation();
        if (Page->GetActiveAllocations() == 0)
        {
            RetirePage(Page);
            delete Page;
            Pages.RemoveAtSwap(Index);
        }

        return;
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
    if (LinearAllocator)
    {
        LinearAllocator->Deallocate(Storage);
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

bool FD3D12BufferAllocatorPool::TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, EBufferFlags BufferUsage, D3D12_RESOURCE_STATES InInitialState, uint64 InAlignment, FD3D12ResourceStorage& OutStorage)
{
    UNREFERENCED_VARIABLE(BufferUsage);
    if (!GetDevice() || !Supports(InHeapType, ResourceDesc))
    {
        return false;
    }

    const uint64 Size      = ResourceDesc.Width;
    const uint64 Alignment = InAlignment ? InAlignment : 16;

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

    FD3D12ResourceAllocationRequest Request = {};
    Request.ResourceType     = ED3D12ResourceType::Buffer;
    Request.HeapType         = InHeapType;
    Request.InitialState     = InInitialState;
    Request.ResourceFlags    = ResourceDesc.Flags;
    Request.Size             = Size;
    Request.Alignment        = Alignment;
    Request.bHasResourceDesc = true;
    Request.ResourceDesc     = ResourceDesc;

    if (!MultiBuddyAllocator || !MultiBuddyAllocator->TryAllocate(Request, OutStorage))
    {
        return false;
    }

    return true;
}

bool FD3D12BufferAllocatorPool::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    D3D12_RESOURCE_DESC Desc = {};
    if (Request.bHasResourceDesc)
    {
        Desc = Request.ResourceDesc;
    }
    else
    {
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = Request.ResourceFlags;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = Request.Size;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;
    }

    return TryAllocate(Request.HeapType, Desc, EBufferFlags::None, Request.InitialState, Request.Alignment, OutStorage);
}

void FD3D12BufferAllocatorPool::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetAllocatorType() == ED3D12AllocatorType::MultiBuddyAllocator && Storage.GetAllocator())
    {
        static_cast<FD3D12MultiBuddyAllocator*>(Storage.GetAllocator())->Deallocate(Storage);
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

bool FD3D12BufferAllocator::TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, EBufferFlags BufferUsage, D3D12_RESOURCE_STATES InitialState, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(PoolsCS);

    for (FD3D12BufferAllocatorPool* Pool : Pools)
    {
        if (!Pool->Supports(InHeapType, ResourceDesc))
        {
            continue;
        }

        if (Pool->TryAllocate(InHeapType, ResourceDesc, BufferUsage, InitialState, Alignment, OutStorage))
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
    return NewPool->TryAllocate(InHeapType, ResourceDesc, BufferUsage, InitialState, Alignment, OutStorage);
}

bool FD3D12BufferAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    D3D12_RESOURCE_DESC Desc = {};
    if (Request.bHasResourceDesc)
    {
        Desc = Request.ResourceDesc;
    }
    else
    {
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = Request.ResourceFlags;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = Request.Size;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;
    }

    return TryAllocate(Request.HeapType, Desc, EBufferFlags::None, Request.InitialState, Request.Alignment, OutStorage);
}

void FD3D12BufferAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetAllocatorType() == ED3D12AllocatorType::BufferAllocatorPool && Storage.GetAllocator())
    {
        static_cast<FD3D12BufferAllocatorPool*>(Storage.GetAllocator())->Deallocate(Storage);
        return;
    }

    if (Storage.GetAllocatorType() == ED3D12AllocatorType::MultiBuddyAllocator && Storage.GetAllocator())
    {
        static_cast<FD3D12MultiBuddyAllocator*>(Storage.GetAllocator())->Deallocate(Storage);
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

FD3D12TextureAllocator::FD3D12TextureAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , CommittedThreshold(128ull * 1024ull * 1024ull)
    , DefaultPageSizeBytes(256ull * 1024ull * 1024ull)
{
}

FD3D12TextureAllocator::~FD3D12TextureAllocator()
{
    Shutdown();
}

bool FD3D12TextureAllocator::Initialize(uint64 InDefaultPageSizeBytes, uint64 InCommittedThreshold)
{
    DefaultPageSizeBytes = InDefaultPageSizeBytes;
    CommittedThreshold   = InCommittedThreshold;
    Pools.Clear();

    auto CreatePool = [this](ETexturePoolClass PoolClass, uint64 PoolAlignment) -> bool
    {
        FPool& Entry = Pools.Emplace();
        Entry.PoolClass = PoolClass;

        Entry.Pool = new FD3D12PoolAllocator(GetDevice(), DefaultPageSizeBytes, PoolAlignment, CommittedThreshold, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, EAllocationStrategy::SuballocatedHeap);
        if (!Entry.Pool)
        {
            delete Entry.Pool;
            Entry.Pool = nullptr;
            Pools.Pop();
            return false;
        }

        return true;
    };

    if (!CreatePool(ETexturePoolClass::Small4K, 4ull * 1024ull) ||
        !CreatePool(ETexturePoolClass::ReadOnly, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) ||
        !CreatePool(ETexturePoolClass::RenderTargetDepthStencil, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT) ||
        !CreatePool(ETexturePoolClass::UAVOnly, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT))
    {
        Shutdown();
        return false;
    }

    return true;
}

void FD3D12TextureAllocator::Shutdown()
{
    for (FPool& Pool : Pools)
    {
        if (Pool.Pool)
        {
            Pool.Pool->Shutdown();
            delete Pool.Pool;
            Pool.Pool = nullptr;
        }
    }

    Pools.Clear();
}

FD3D12TextureAllocator::ETexturePoolClass FD3D12TextureAllocator::ClassifyTexture(const D3D12_RESOURCE_DESC& Desc, uint64 Size, uint64 Alignment) const
{
    if (Size <= (4ull * 1024ull) && Alignment <= (4ull * 1024ull))
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

bool FD3D12TextureAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || !Request.bHasResourceDesc)
    {
        return false;
    }

    const D3D12_RESOURCE_DESC Desc = Request.ResourceDesc;
    const D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &Desc);

    if (AllocationInfo.SizeInBytes >= CommittedThreshold && Request.bAllowCommittedFallback)
    {
        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, D3D12_HEAP_TYPE_DEFAULT, Request.InitialState, Request.ClearValue, NewResource))
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

    int32 PoolIndex = -1;
    for (int32 Index = 0; Index < Pools.Size(); ++Index)
    {
        if (Pools[Index].PoolClass == PoolClass)
        {
            PoolIndex = Index;
            break;
        }
    }

    if (PoolIndex < 0)
    {
        return false;
    }

    FD3D12ResourceAllocationRequest BlockRequest = {};
    BlockRequest.ResourceType            = ED3D12ResourceType::Texture;
    BlockRequest.HeapType                = D3D12_HEAP_TYPE_DEFAULT;
    BlockRequest.InitialState            = Request.InitialState;
    BlockRequest.Size                    = AllocationInfo.SizeInBytes;
    BlockRequest.Alignment               = AllocationInfo.Alignment;
    BlockRequest.bAllowCommittedFallback = false;

    FD3D12ResourceStorage BlockResourceStorage(GetDevice());
    if (!Pools[PoolIndex].Pool || !Pools[PoolIndex].Pool->TryAllocate(BlockRequest, BlockResourceStorage))
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
    if (!GetDevice()->CreatePlacedResource(Heap, BlockResourceStorage.GetPoolAllocationData().Offset, Desc, Request.InitialState, Request.ClearValue, NewResource))
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
