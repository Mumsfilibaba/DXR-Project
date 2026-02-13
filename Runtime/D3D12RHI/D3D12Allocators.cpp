#include "Core/Math/Math.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12DeletionQueue.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

namespace
{
static uint64 AlignUpValue(uint64 Value, uint64 Alignment)
{
    if (Alignment <= 1)
    {
        return Value;
    }

    const uint64 Mask = Alignment - 1;
    return (Value + Mask) & ~Mask;
}

static D3D12_RESOURCE_DESC CreateBufferDesc(uint64 Size, D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE)
{
    D3D12_RESOURCE_DESC Desc = {};
    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = Flags;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = Size;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.Alignment          = 0;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;
    return Desc;
}

static FD3D12ResidencyHandle RegisterResidency(FD3D12Device* Device, ID3D12Pageable* Pageable, uint64 SizeBytes)
{
    if (!Device || !Pageable)
    {
        return {};
    }

    if (FD3D12ResidencyManager* ResidencyManager = Device->GetResidencyManager())
    {
        FD3D12ResidencyHandle Handle = ResidencyManager->RegisterPageable(Pageable, SizeBytes, false);
        ResidencyManager->TouchPageable(Handle);
        return Handle;
    }

    return {};
}
} // namespace

FD3D12BuddyAllocator::FD3D12BuddyAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , MinBlockBytes(256)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , InitialState(D3D12_RESOURCE_STATE_COMMON)
    , bBackWithHeap(false)
    , NextPageIndex(1)
{
}

FD3D12BuddyAllocator::~FD3D12BuddyAllocator()
{
    Shutdown();
}

bool FD3D12BuddyAllocator::Initialize(uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, bool bInBackWithHeap)
{
    PageSizeBytes = InPageSizeBytes;
    MinBlockBytes = Math::Max<uint64>(InMinBlockBytes, 256);
    HeapType = InHeapType;
    InitialState = InInitialState;
    bBackWithHeap = bInBackWithHeap;
    NextPageIndex = 1;
    Pages.Clear();
    return true;
}

void FD3D12BuddyAllocator::Shutdown()
{
    SCOPED_LOCK(PagesCS);
    Pages.Clear(true);
}

uint32 FD3D12BuddyAllocator::GetOrderForSize(uint64 Size) const
{
    uint32 Order = 0;
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

bool FD3D12BuddyAllocator::AllocateFromPage(FPage& Page, uint64 Size, uint64 Alignment, uint64& OutOffset, uint32& OutOrder)
{
    const uint64 RequestedSize = Math::Max(Size, Alignment);
    const uint32 TargetOrder = GetOrderForSize(RequestedSize);
    for (uint32 CurrentOrder = TargetOrder; CurrentOrder < static_cast<uint32>(Page.FreeOffsets.Size()); ++CurrentOrder)
    {
        TArray<uint64>& CurrentList = Page.FreeOffsets[CurrentOrder];
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
            Page.FreeOffsets[CurrentOrder].Add(BlockOffset + SplitBlockSize);
        }

        OutOffset = BlockOffset;
        OutOrder = TargetOrder;
        return true;
    }

    return false;
}

void FD3D12BuddyAllocator::FreeToPage(FPage& Page, uint64 Offset, uint32 Order)
{
    const uint32 MaxOrder = static_cast<uint32>(Page.FreeOffsets.Size() - 1);
    uint64 CurrentOffset = Offset;
    uint32 CurrentOrder = Order;

    while (CurrentOrder < MaxOrder)
    {
        const uint64 BlockSize = GetOrderBlockSize(CurrentOrder);
        const uint64 BuddyOffset = CurrentOffset ^ BlockSize;
        TArray<uint64>& FreeList = Page.FreeOffsets[CurrentOrder];

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

    Page.FreeOffsets[CurrentOrder].Add(CurrentOffset);
}

bool FD3D12BuddyAllocator::CreatePage(FPage& OutPage, uint64 RequestedSize)
{
    if (!GetDevice())
    {
        return false;
    }

    uint64 TargetSize = Math::Max(PageSizeBytes, AlignUpValue(RequestedSize, MinBlockBytes));
    uint64 PageSize = MinBlockBytes;
    while (PageSize < TargetSize)
    {
        PageSize <<= 1;
    }

    D3D12_HEAP_DESC HeapDesc = {};
    HeapDesc.Properties.Type                 = HeapType;
    HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapDesc.Properties.VisibleNodeMask      = 1;
    HeapDesc.Properties.CreationNodeMask     = 1;
    HeapDesc.SizeInBytes                     = PageSize;
    HeapDesc.Alignment                       = 0;
    HeapDesc.Flags                           = D3D12_HEAP_FLAG_NONE;

    FD3D12HeapRef NewHeap;
    if (!GetDevice()->CreateHeap(HeapDesc, NewHeap))
    {
        return false;
    }

    FD3D12ResourceRef NewResource;
    if (!bBackWithHeap)
    {
        const D3D12_RESOURCE_DESC Desc = CreateBufferDesc(PageSize, D3D12_RESOURCE_FLAG_NONE);
        if (!GetDevice()->CreatePlacedResource(NewHeap.Get(), 0, Desc, InitialState, nullptr, NewResource))
        {
            return false;
        }
    }

    OutPage.Heap = NewHeap;
    OutPage.Resource = NewResource;
    OutPage.Size = PageSize;
    OutPage.PageIndex = NextPageIndex++;
    OutPage.MappedBaseAddress = nullptr;
    OutPage.BaseGpuVirtualAddress = 0;

    if (NewResource)
    {
        OutPage.BaseGpuVirtualAddress = NewResource->GetGPUVirtualAddress();
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            OutPage.MappedBaseAddress = static_cast<uint8*>(NewResource->MapRange(0, nullptr));
        }
    }

    OutPage.ResidencyHandle = NewHeap ? NewHeap->GetResidencyHandle() : FD3D12ResidencyHandle{};

    const uint32 MaxOrder = GetOrderForSize(PageSize);
    OutPage.FreeOffsets.Resize(MaxOrder + 1);
    OutPage.FreeOffsets[MaxOrder].Add(0);
    return true;
}

bool FD3D12BuddyAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Request.Size == 0)
    {
        return false;
    }

    const uint64 Alignment = Request.Alignment ? Request.Alignment : MinBlockBytes;
    const uint64 AllocationSize = AlignUpValue(Math::Max(Request.Size, Alignment), MinBlockBytes);

    SCOPED_LOCK(PagesCS);

    uint64 Offset = 0;
    uint32 Order = 0;
    FPage* SelectedPage = nullptr;
    for (FPage& Page : Pages)
    {
        if (AllocateFromPage(Page, AllocationSize, Alignment, Offset, Order))
        {
            SelectedPage = &Page;
            break;
        }
    }

    if (!SelectedPage)
    {
        FPage NewPage;
        if (!CreatePage(NewPage, AllocationSize))
        {
            return false;
        }

        Pages.Add(NewPage);
        SelectedPage = &Pages.LastElement();
        if (!AllocateFromPage(*SelectedPage, AllocationSize, Alignment, Offset, Order))
        {
            return false;
        }
    }

    const uint64 BlockSize = GetOrderBlockSize(Order);

    OutStorage.Reset();
    OutStorage.SetSize(BlockSize);
    OutStorage.SetResidencyHandle(SelectedPage->ResidencyHandle);

    if (SelectedPage->Resource)
    {
        OutStorage.SetResource(SelectedPage->Resource);
        OutStorage.SetSubrangeInfo(
            Offset,
            SelectedPage->BaseGpuVirtualAddress + Offset,
            SelectedPage->MappedBaseAddress ? (SelectedPage->MappedBaseAddress + Offset) : nullptr);
    }

    FD3D12BuddyAllocatorAllocationData AllocationData = {};
    AllocationData.PageIndex = SelectedPage->PageIndex;
    AllocationData.Order = Order;
    AllocationData.Offset = Offset;
    AllocationData.bBackedByHeap = bBackWithHeap;
    AllocationData.BackingHeap = SelectedPage->Heap.Get();
    OutStorage.SetBuddyAllocationData(AllocationData);
    OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::BuddyAllocator);
    return true;
}

void FD3D12BuddyAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (!Storage.IsValid())
    {
        return;
    }

    if (FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(ED3D12DeferredAllocatorType::Buddy, this, Storage);
        return;
    }

    ReturnBlockToAllocator(Storage);
}

void FD3D12BuddyAllocator::ReturnBlockToAllocator(const FD3D12ResourceStorage& Storage)
{
    const FD3D12BuddyAllocatorAllocationData Data = Storage.GetBuddyAllocationData();
    if (Data.PageIndex == UINT32_MAX)
    {
        return;
    }

    SCOPED_LOCK(PagesCS);
    for (FPage& Page : Pages)
    {
        if (Page.PageIndex == Data.PageIndex)
        {
            FreeToPage(Page, Data.Offset, Data.Order);
            return;
        }
    }
}

FD3D12MultiBuddyAllocator::FD3D12MultiBuddyAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , MinBlockBytes(0)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , InitialState(D3D12_RESOURCE_STATE_COMMON)
    , bBackWithHeap(false)
{
}

FD3D12MultiBuddyAllocator::~FD3D12MultiBuddyAllocator()
{
    Shutdown();
}

bool FD3D12MultiBuddyAllocator::Initialize(uint64 InPageSizeBytes, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, bool bInBackWithHeap)
{
    PageSizeBytes = InPageSizeBytes;
    MinBlockBytes = InMinBlockBytes;
    HeapType = InHeapType;
    InitialState = InInitialState;
    bBackWithHeap = bInBackWithHeap;
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
    FD3D12BuddyAllocator* Allocator = new FD3D12BuddyAllocator(GetDevice());
    if (!Allocator->Initialize(PageSizeBytes, MinBlockBytes, HeapType, InitialState, bBackWithHeap))
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
            Data.PageIndex = BuddyData.PageIndex;
            Data.Order = BuddyData.Order;
            Data.Offset = BuddyData.Offset;
            Data.bBackedByHeap = BuddyData.bBackedByHeap;
            Data.BackingHeap = BuddyData.BackingHeap;
            OutStorage.SetMultiBuddyAllocationData(Data);
            OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::MultiBuddyAllocator);
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
    Data.PageIndex = BuddyData.PageIndex;
    Data.Order = BuddyData.Order;
    Data.Offset = BuddyData.Offset;
    Data.bBackedByHeap = BuddyData.bBackedByHeap;
    Data.BackingHeap = BuddyData.BackingHeap;
    OutStorage.SetMultiBuddyAllocationData(Data);
    OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::MultiBuddyAllocator);
    return true;
}

void FD3D12MultiBuddyAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12MultiBuddyAllocatorAllocationData MultiData = Storage.GetMultiBuddyAllocationData();
    if (MultiData.BuddyAllocatorIndex == UINT32_MAX)
    {
        return;
    }

    SCOPED_LOCK(AllocatorsCS);
    if (MultiData.BuddyAllocatorIndex >= static_cast<uint32>(Allocators.Size()))
    {
        return;
    }

    FD3D12BuddyAllocator* Allocator = Allocators[MultiData.BuddyAllocatorIndex];

    FD3D12ResourceStorage DeferredResourceStorage;
    DeferredResourceStorage.CopyFrom(Storage);
    FD3D12BuddyAllocatorAllocationData BuddyData = {};
    BuddyData.PageIndex = MultiData.PageIndex;
    BuddyData.Order = MultiData.Order;
    BuddyData.Offset = MultiData.Offset;
    BuddyData.bBackedByHeap = MultiData.bBackedByHeap;
    BuddyData.BackingHeap = MultiData.BackingHeap;
    DeferredResourceStorage.SetBuddyAllocationData(BuddyData);
    DeferredResourceStorage.SetOwner(nullptr, ED3D12ResourceStorageOwnerType::None);

    if (FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(ED3D12DeferredAllocatorType::Buddy, Allocator, DeferredResourceStorage);
    }
    else
    {
        Allocator->ReturnBlockToAllocator(DeferredResourceStorage);
    }
}

FD3D12PoolAllocator::FD3D12PoolAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , Alignment(256)
    , MaxResourceSize(0)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , InitialState(D3D12_RESOURCE_STATE_COMMON)
    , bBackWithHeap(false)
    , NextPageIndex(1)
    , FragmentedBytes(0)
{
}

FD3D12PoolAllocator::~FD3D12PoolAllocator()
{
    Shutdown();
}

bool FD3D12PoolAllocator::Initialize(uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxResourceSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, bool bInBackWithHeap)
{
    PageSizeBytes = InPageSizeBytes;
    Alignment = Math::Max<uint64>(InAlignment, 16);
    MaxResourceSize = InMaxResourceSize;
    HeapType = InHeapType;
    InitialState = InInitialState;
    bBackWithHeap = bInBackWithHeap;
    NextPageIndex = 1;
    FragmentedBytes = 0;
    DefragRecords.Clear();
    Pages.Clear();
    return true;
}

void FD3D12PoolAllocator::Shutdown()
{
    SCOPED_LOCK(PagesCS);
    Pages.Clear(true);
    DefragRecords.Clear();
    FragmentedBytes = 0;
}

bool FD3D12PoolAllocator::CreatePage(uint64 MinimumSize, FPage& OutPage)
{
    if (!GetDevice())
    {
        return false;
    }

    const uint64 PageSize = Math::Max(PageSizeBytes, AlignUpValue(MinimumSize, Alignment));
    D3D12_HEAP_DESC HeapDesc = {};
    HeapDesc.Properties.Type                 = HeapType;
    HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapDesc.Properties.VisibleNodeMask      = 1;
    HeapDesc.Properties.CreationNodeMask     = 1;
    HeapDesc.SizeInBytes                     = PageSize;
    HeapDesc.Alignment                       = 0;
    HeapDesc.Flags                           = D3D12_HEAP_FLAG_NONE;

    FD3D12HeapRef NewHeap;
    if (!GetDevice()->CreateHeap(HeapDesc, NewHeap))
    {
        return false;
    }

    FD3D12ResourceRef NewResource;
    if (!bBackWithHeap)
    {
        const D3D12_RESOURCE_DESC Desc = CreateBufferDesc(PageSize);
        if (!GetDevice()->CreatePlacedResource(NewHeap.Get(), 0, Desc, InitialState, nullptr, NewResource))
        {
            return false;
        }
    }

    OutPage.Heap = NewHeap;
    OutPage.Resource = NewResource;
    OutPage.MappedBaseAddress = nullptr;
    OutPage.BaseGpuVirtualAddress = 0;
    OutPage.Size = PageSize;
    OutPage.UsedBytes = 0;
    OutPage.PageIndex = NextPageIndex++;
    OutPage.FreeRanges.Clear();
    OutPage.FreeRanges.Add({0, PageSize});

    if (NewResource)
    {
        OutPage.BaseGpuVirtualAddress = NewResource->GetGPUVirtualAddress();
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            OutPage.MappedBaseAddress = static_cast<uint8*>(NewResource->MapRange(0, nullptr));
        }
    }

    OutPage.ResidencyHandle = NewHeap ? NewHeap->GetResidencyHandle() : FD3D12ResidencyHandle{};

    return true;
}

bool FD3D12PoolAllocator::AllocateFromPage(FPage& Page, uint64 Size, uint64 InAlignment, uint64& OutOffset)
{
    for (int32 Index = 0; Index < Page.FreeRanges.Size(); ++Index)
    {
        const FFreeRange Range = Page.FreeRanges[Index];
        const uint64 AlignedOffset = AlignUpValue(Range.Offset, InAlignment);
        const uint64 Padding = AlignedOffset - Range.Offset;
        const uint64 RequiredSize = Padding + Size;

        if (Range.Size < RequiredSize)
        {
            continue;
        }

        OutOffset = AlignedOffset;

        const uint64 TailSize = Range.Size - RequiredSize;
        if (Padding > 0)
        {
            Page.FreeRanges[Index].Size = Padding;
            if (TailSize > 0)
            {
                Page.FreeRanges.Add({AlignedOffset + Size, TailSize});
            }
        }
        else if (TailSize > 0)
        {
            Page.FreeRanges[Index].Offset = AlignedOffset + Size;
            Page.FreeRanges[Index].Size = TailSize;
        }
        else
        {
            Page.FreeRanges.RemoveAtSwap(Index);
        }

        Page.UsedBytes += Size;
        return true;
    }

    return false;
}

void FD3D12PoolAllocator::CoalesceFreeRanges(TArray<FFreeRange>& FreeRanges)
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
        FFreeRange& Next = FreeRanges[Index + 1];
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

void FD3D12PoolAllocator::ComputeTLSFIndices(uint64 Size, uint32& OutFL, uint32& OutSL) const
{
    OutFL = 0;
    OutSL = 0;
    if (Size == 0)
    {
        return;
    }

    uint32 FirstLevel = 0;
    uint64 Temp = Size;
    while (Temp > 1)
    {
        Temp >>= 1;
        ++FirstLevel;
    }

    OutFL = Math::Min<uint32>(FirstLevel, TLSFFirstLevelCount - 1);
    const uint64 RangeBase = 1ull << OutFL;
    const uint64 OffsetInRange = Size - RangeBase;
    const uint64 SL = (OffsetInRange * TLSFSecondLevelCount) / RangeBase;
    OutSL = Math::Min<uint32>(static_cast<uint32>(SL), TLSFSecondLevelCount - 1);
}

void FD3D12PoolAllocator::AddDefragRecord(uint32 PageIndex, uint64 Offset, uint64 Size)
{
    FDefragRecord Record = {};
    Record.PageIndex = PageIndex;
    Record.Offset = Offset;
    Record.Size = Size;
    DefragRecords.Add(Record);
}

bool FD3D12PoolAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Request.Size == 0)
    {
        return false;
    }

    const uint64 UsedAlignment = Request.Alignment ? Request.Alignment : Alignment;
    const uint64 SizeAligned = AlignUpValue(Request.Size, UsedAlignment);

    if (SizeAligned > MaxResourceSize && Request.bAllowCommittedFallback)
    {
        D3D12_RESOURCE_DESC Desc = Request.bHasResourceDesc ? Request.ResourceDesc : CreateBufferDesc(SizeAligned, Request.ResourceFlags);
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

        OutStorage.InitializeAsStandalone(NewResource, ED3D12ResourceKind::Unknown, HeapType, ED3D12HeapUsage::Any, ED3D12ResourceLifetime::Default, SizeAligned, UsedAlignment);
        OutStorage.SetSubrangeInfo(0, NewResource->GetGPUVirtualAddress(), MappedBaseAddress);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex = UINT32_MAX;
        AllocationData.Offset = 0;
        AllocationData.Size = SizeAligned;
        AllocationData.bCommittedAllocation = true;
        AllocationData.bBackedByHeap = false;
        AllocationData.BackingHeap = nullptr;
        OutStorage.SetPoolAllocationData(AllocationData);
        OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::PoolAllocator);

        if (HeapType == D3D12_HEAP_TYPE_DEFAULT)
        {
            OutStorage.SetResidencyHandle(RegisterResidency(GetDevice(), NewResource->GetD3D12Resource(), SizeAligned));
        }

        return true;
    }

    SCOPED_LOCK(PagesCS);

    uint64 Offset = 0;
    FPage* SelectedPage = nullptr;
    for (FPage& Page : Pages)
    {
        if (AllocateFromPage(Page, SizeAligned, UsedAlignment, Offset))
        {
            SelectedPage = &Page;
            break;
        }
    }

    if (!SelectedPage)
    {
        FPage NewPage;
        if (!CreatePage(SizeAligned, NewPage))
        {
            return false;
        }

        Pages.Add(NewPage);
        SelectedPage = &Pages.LastElement();
        if (!AllocateFromPage(*SelectedPage, SizeAligned, UsedAlignment, Offset))
        {
            return false;
        }
    }

    OutStorage.Reset();
    OutStorage.SetSize(SizeAligned);
    OutStorage.SetResidencyHandle(SelectedPage->ResidencyHandle);

    if (SelectedPage->Resource)
    {
        OutStorage.SetResource(SelectedPage->Resource);
        OutStorage.SetSubrangeInfo(
            Offset,
            SelectedPage->BaseGpuVirtualAddress + Offset,
            SelectedPage->MappedBaseAddress ? (SelectedPage->MappedBaseAddress + Offset) : nullptr);
    }

    FD3D12PoolAllocatorAllocationData AllocationData = {};
    AllocationData.PageIndex = SelectedPage->PageIndex;
    AllocationData.Offset = Offset;
    AllocationData.Size = SizeAligned;
    AllocationData.bCommittedAllocation = false;
    AllocationData.bBackedByHeap = bBackWithHeap;
    AllocationData.BackingHeap = SelectedPage->Heap.Get();
    OutStorage.SetPoolAllocationData(AllocationData);
    OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::PoolAllocator);
    return true;
}

void FD3D12PoolAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12PoolAllocatorAllocationData Data = Storage.GetPoolAllocationData();
    if (Data.bCommittedAllocation)
    {
        if (Storage.GetResource() && FD3D12RHI::Get())
        {
            FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
        }
        return;
    }

    if (Data.bBackedByHeap && Storage.GetResource() && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
    }

    if (FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(ED3D12DeferredAllocatorType::Pool, this, Storage);
    }
    else
    {
        ReturnBlockToAllocator(Storage);
    }
}

void FD3D12PoolAllocator::ReturnBlockToAllocator(const FD3D12ResourceStorage& Storage)
{
    const FD3D12PoolAllocatorAllocationData Data = Storage.GetPoolAllocationData();
    if (Data.bCommittedAllocation || Data.PageIndex == UINT32_MAX || Data.Size == 0)
    {
        return;
    }

    SCOPED_LOCK(PagesCS);
    for (FPage& Page : Pages)
    {
        if (Page.PageIndex != Data.PageIndex)
        {
            continue;
        }

        Page.FreeRanges.Add({Data.Offset, Data.Size});
        Page.UsedBytes = Page.UsedBytes > Data.Size ? (Page.UsedBytes - Data.Size) : 0;
        CoalesceFreeRanges(Page.FreeRanges);
        break;
    }

    DefragRecords.Clear();
    FragmentedBytes = 0;
    for (const FPage& Page : Pages)
    {
        if (Page.FreeRanges.Size() <= 1)
        {
            continue;
        }

        for (const FFreeRange& Range : Page.FreeRanges)
        {
            FragmentedBytes += Range.Size;
            AddDefragRecord(Page.PageIndex, Range.Offset, Range.Size);
        }
    }
}

FD3D12BucketAllocator::FD3D12BucketAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , Alignment(256)
    , HeapType(D3D12_HEAP_TYPE_UPLOAD)
    , InitialState(D3D12_RESOURCE_STATE_GENERIC_READ)
{
}

FD3D12BucketAllocator::~FD3D12BucketAllocator()
{
    Shutdown();
}

bool FD3D12BucketAllocator::Initialize(const TArray<uint64>& InBucketSizes, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
{
    PageSizeBytes = InPageSizeBytes;
    Alignment = InAlignment;
    HeapType = InHeapType;
    InitialState = InInitialState;
    Buckets.Clear();

    for (uint64 BucketSize : InBucketSizes)
    {
        FBucket& Bucket = Buckets.Emplace();
        Bucket.BlockSize = BucketSize;
        Bucket.Pool = new FD3D12PoolAllocator(GetDevice());
        if (!Bucket.Pool->Initialize(PageSizeBytes, Alignment, BucketSize, HeapType, InitialState, false))
        {
            delete Bucket.Pool;
            Bucket.Pool = nullptr;
            Buckets.Pop();
            Shutdown();
            return false;
        }
    }

    Buckets.SortWithPredicate([](const FBucket& A, const FBucket& B)
    {
        return A.BlockSize < B.BlockSize;
    });

    return true;
}

void FD3D12BucketAllocator::Shutdown()
{
    for (FBucket& Bucket : Buckets)
    {
        if (Bucket.Pool)
        {
            Bucket.Pool->Shutdown();
            delete Bucket.Pool;
            Bucket.Pool = nullptr;
        }
    }
    Buckets.Clear();
}

bool FD3D12BucketAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    for (FBucket& Bucket : Buckets)
    {
        if (Request.Size <= Bucket.BlockSize)
        {
            if (!Bucket.Pool)
            {
                return false;
            }
            return Bucket.Pool->TryAllocate(Request, OutStorage);
        }
    }

    return false;
}

void FD3D12BucketAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const uint64 AllocationSize = Storage.GetSize();
    for (FBucket& Bucket : Buckets)
    {
        if (AllocationSize <= Bucket.BlockSize)
        {
            if (Bucket.Pool)
            {
                Bucket.Pool->Deallocate(Storage);
            }
            return;
        }
    }
}

void FD3D12BucketAllocator::ReturnBlockToAllocator(const FD3D12ResourceStorage& Storage)
{
    const uint64 AllocationSize = Storage.GetSize();
    for (FBucket& Bucket : Buckets)
    {
        if (AllocationSize <= Bucket.BlockSize)
        {
            if (Bucket.Pool)
            {
                Bucket.Pool->ReturnBlockToAllocator(Storage);
            }
            return;
        }
    }
}

FD3D12UploadHeapAllocator::FD3D12UploadHeapAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , DefaultAlignment(256)
    , SmallAllocationThreshold(64ull * 1024ull)
    , LargeAllocationThreshold(2ull * 1024ull * 1024ull)
    , SmallAllocator(InDevice)
    , LargeAllocator(InDevice)
    , ConstantsAllocator(InDevice)
{
}

FD3D12UploadHeapAllocator::~FD3D12UploadHeapAllocator()
{
    Shutdown();
}

bool FD3D12UploadHeapAllocator::Initialize(uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallAllocationThreshold, uint64 InLargeAllocationThreshold)
{
    PageSizeBytes = InPageSizeBytes;
    DefaultAlignment = InAlignment;
    SmallAllocationThreshold = InSmallAllocationThreshold;
    LargeAllocationThreshold = InLargeAllocationThreshold;

    if (!SmallAllocator.Initialize(PageSizeBytes, DefaultAlignment, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, false))
    {
        return false;
    }

    if (!LargeAllocator.Initialize(PageSizeBytes, DefaultAlignment, LargeAllocationThreshold, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, false))
    {
        return false;
    }

    return ConstantsAllocator.Initialize(PageSizeBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, false);
}

void FD3D12UploadHeapAllocator::Shutdown()
{
    ConstantsAllocator.Shutdown();
    LargeAllocator.Shutdown();
    SmallAllocator.Shutdown();
}

void* FD3D12UploadHeapAllocator::Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    FD3D12ResourceAllocationRequest Request = {};
    Request.ResourceType = ED3D12ResourceType::Buffer;
    Request.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    Request.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    Request.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    Request.Size = Size;
    Request.Alignment = Alignment ? Alignment : DefaultAlignment;
    Request.bAllowCommittedFallback = true;

    if (Size <= SmallAllocationThreshold)
    {
        if (!SmallAllocator.TryAllocate(Request, OutStorage))
        {
            return nullptr;
        }
    }
    else
    {
        if (!LargeAllocator.TryAllocate(Request, OutStorage))
        {
            return nullptr;
        }
    }

    return OutStorage.GetMappedBaseAddress();
}

void* FD3D12UploadHeapAllocator::AllocateConstants(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    FD3D12ResourceAllocationRequest Request = {};
    Request.ResourceType = ED3D12ResourceType::Buffer;
    Request.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    Request.InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    Request.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
    Request.Size = Size;
    Request.Alignment = Alignment ? Alignment : D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    Request.bAllowCommittedFallback = true;

    if (!ConstantsAllocator.TryAllocate(Request, OutStorage))
    {
        return nullptr;
    }

    return OutStorage.GetMappedBaseAddress();
}

bool FD3D12UploadHeapAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    return Allocate(Request.Size, Request.Alignment, OutStorage) != nullptr;
}

void FD3D12UploadHeapAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::PoolAllocator)
    {
        FD3D12ResourceStorage CopiedResourceStorage;
        CopiedResourceStorage.CopyFrom(Storage);
        CopiedResourceStorage.SetOwner(&LargeAllocator, ED3D12ResourceStorageOwnerType::PoolAllocator);
        LargeAllocator.Deallocate(CopiedResourceStorage);
        CopiedResourceStorage.SetOwner(nullptr, ED3D12ResourceStorageOwnerType::None);
        return;
    }

    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::MultiBuddyAllocator)
    {
        FD3D12ResourceStorage CopiedResourceStorage;
        CopiedResourceStorage.CopyFrom(Storage);
        CopiedResourceStorage.SetOwner(&SmallAllocator, ED3D12ResourceStorageOwnerType::MultiBuddyAllocator);
        SmallAllocator.Deallocate(CopiedResourceStorage);
        CopiedResourceStorage.SetOwner(nullptr, ED3D12ResourceStorageOwnerType::None);
        return;
    }

    if (Storage.GetResource() && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
    }
}

FD3D12LinearAllocator::FD3D12LinearAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , HeapType(D3D12_HEAP_TYPE_UPLOAD)
    , InitialState(D3D12_RESOURCE_STATE_GENERIC_READ)
    , NextPageIndex(1)
    , UploadHeapAllocator(nullptr)
{
}

FD3D12LinearAllocator::~FD3D12LinearAllocator()
{
    Shutdown();
}

bool FD3D12LinearAllocator::Initialize(uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, FD3D12UploadHeapAllocator* InUploadHeapAllocator)
{
    PageSizeBytes = InPageSizeBytes;
    HeapType = InHeapType;
    InitialState = InInitialState;
    UploadHeapAllocator = InUploadHeapAllocator;
    NextPageIndex = 1;
    Pages.Clear();
    return true;
}

void FD3D12LinearAllocator::Shutdown()
{
    SCOPED_LOCK(PagesCS);
    for (FPage& Page : Pages)
    {
        RetirePage(Page);
    }
    Pages.Clear();
}

bool FD3D12LinearAllocator::CreatePage(FPage& OutPage)
{
    OutPage = {};
    OutPage.PageIndex = NextPageIndex++;

    if (HeapType == D3D12_HEAP_TYPE_UPLOAD && UploadHeapAllocator)
    {
        if (UploadHeapAllocator->Allocate(PageSizeBytes, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, OutPage.BackingResourceStorage) == nullptr)
        {
            return false;
        }
        return true;
    }

    if (!GetDevice())
    {
        return false;
    }

    FD3D12ResourceRef Resource;
    const D3D12_RESOURCE_DESC Desc = CreateBufferDesc(PageSizeBytes);
    if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
    {
        return false;
    }

    void* MappedBaseAddress = nullptr;
    if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
    {
        MappedBaseAddress = Resource->MapRange(0, nullptr);
    }

    OutPage.BackingResourceStorage.InitializeAsStandalone(Resource, ED3D12ResourceKind::Buffer, HeapType, ED3D12HeapUsage::BufferOnly, ED3D12ResourceLifetime::Transient, PageSizeBytes, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    OutPage.BackingResourceStorage.SetSubrangeInfo(0, Resource->GetGPUVirtualAddress(), MappedBaseAddress);
    return true;
}

void FD3D12LinearAllocator::RetirePage(FPage& Page)
{
    if (Page.BackingResourceStorage.GetOwner() != nullptr)
    {
        Page.BackingResourceStorage.ReleaseResource();
    }
    else if (Page.BackingResourceStorage.GetResource() && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(Page.BackingResourceStorage.GetResource());
    }

    Page.BackingResourceStorage = FD3D12ResourceStorage{};
    Page.CurrentOffset = 0;
    Page.ActiveAllocations = 0;
}

void* FD3D12LinearAllocator::Allocate(uint64 Size, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    if (!GetDevice() || Size == 0)
    {
        return nullptr;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16);
    const uint64 SizeAligned = AlignUpValue(Size, UsedAlignment);

    if (SizeAligned > PageSizeBytes)
    {
        FD3D12ResourceRef Resource;
        const D3D12_RESOURCE_DESC Desc = CreateBufferDesc(SizeAligned);
        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
        {
            return nullptr;
        }

        void* MappedBaseAddress = nullptr;
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = Resource->MapRange(0, nullptr);
        }

        OutStorage.InitializeAsStandalone(Resource, ED3D12ResourceKind::Buffer, HeapType, ED3D12HeapUsage::BufferOnly, ED3D12ResourceLifetime::Transient, SizeAligned, UsedAlignment);
        OutStorage.SetSubrangeInfo(0, Resource->GetGPUVirtualAddress(), MappedBaseAddress);
        OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::LinearAllocator);

        FD3D12LinearAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex = UINT32_MAX;
        AllocationData.PageOffset = 0;
        AllocationData.AllocationSize = SizeAligned;
        AllocationData.bDirectAllocation = true;
        OutStorage.SetLinearAllocationData(AllocationData);
        return OutStorage.GetMappedBaseAddress();
    }

    SCOPED_LOCK(PagesCS);

    FPage* SelectedPage = nullptr;
    uint64 AllocationOffset = 0;
    for (FPage& Page : Pages)
    {
        const uint64 AlignedOffset = AlignUpValue(Page.CurrentOffset, UsedAlignment);
        if (AlignedOffset + SizeAligned <= PageSizeBytes)
        {
            SelectedPage = &Page;
            AllocationOffset = AlignedOffset;
            break;
        }
    }

    if (!SelectedPage)
    {
        FPage NewPage;
        if (!CreatePage(NewPage))
        {
            return nullptr;
        }

        Pages.Add(Move(NewPage));
        SelectedPage = &Pages.LastElement();
        AllocationOffset = 0;
    }

    const uint64 PageResourceOffset = SelectedPage->BackingResourceStorage.GetResourceOffset() + AllocationOffset;
    const D3D12_GPU_VIRTUAL_ADDRESS PageGpuAddress = SelectedPage->BackingResourceStorage.GetGpuVirtualAddress() + AllocationOffset;
    uint8* MappedBase = static_cast<uint8*>(SelectedPage->BackingResourceStorage.GetMappedBaseAddress());

    OutStorage.Reset();
    if (FD3D12Resource* PageResource = SelectedPage->BackingResourceStorage.GetResource())
    {
        FD3D12ResourceRef ResourceRef = PageResource;
        OutStorage.SetResource(ResourceRef);
    }
    OutStorage.SetSubrangeInfo(PageResourceOffset, PageGpuAddress, MappedBase ? (MappedBase + AllocationOffset) : nullptr);
    OutStorage.SetSize(SizeAligned);
    OutStorage.SetResidencyHandle(SelectedPage->BackingResourceStorage.GetResidencyHandle());
    OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::LinearAllocator);

    FD3D12LinearAllocatorAllocationData AllocationData = {};
    AllocationData.PageIndex = SelectedPage->PageIndex;
    AllocationData.PageOffset = AllocationOffset;
    AllocationData.AllocationSize = SizeAligned;
    AllocationData.bDirectAllocation = false;
    OutStorage.SetLinearAllocationData(AllocationData);

    SelectedPage->CurrentOffset = AllocationOffset + SizeAligned;
    SelectedPage->ActiveAllocations += 1;
    return OutStorage.GetMappedBaseAddress();
}

bool FD3D12LinearAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    return Allocate(Request.Size, Request.Alignment, OutStorage) != nullptr || (OutStorage.IsValid() && OutStorage.GetMappedBaseAddress() == nullptr);
}

void FD3D12LinearAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    const FD3D12LinearAllocatorAllocationData Data = Storage.GetLinearAllocationData();

    if (Data.bDirectAllocation)
    {
        if (Storage.GetResource() && FD3D12RHI::Get())
        {
            FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
        }
        return;
    }

    SCOPED_LOCK(PagesCS);
    for (int32 Index = 0; Index < Pages.Size(); ++Index)
    {
        FPage& Page = Pages[Index];
        if (Page.PageIndex != Data.PageIndex)
        {
            continue;
        }

        if (Page.ActiveAllocations > 0)
        {
            --Page.ActiveAllocations;
        }

        if (Page.ActiveAllocations == 0)
        {
            RetirePage(Page);
            Pages.RemoveAtSwap(Index);
        }
        return;
    }
}

FD3D12DynamicConstantsAllocator::FD3D12DynamicConstantsAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , LinearAllocator(InDevice)
{
}

FD3D12DynamicConstantsAllocator::~FD3D12DynamicConstantsAllocator()
{
    Shutdown();
}

bool FD3D12DynamicConstantsAllocator::Initialize(uint64 InPageSizeBytes, FD3D12UploadHeapAllocator* InUploadHeapAllocator)
{
    return LinearAllocator.Initialize(InPageSizeBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, InUploadHeapAllocator);
}

void FD3D12DynamicConstantsAllocator::Shutdown()
{
    LinearAllocator.Shutdown();
}

void* FD3D12DynamicConstantsAllocator::Allocate(uint64 Size, FD3D12ResourceStorage& OutStorage)
{
    return LinearAllocator.Allocate(Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, OutStorage);
}

bool FD3D12DynamicConstantsAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    return Allocate(Request.Size, OutStorage) != nullptr || OutStorage.IsValid();
}

void FD3D12DynamicConstantsAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    LinearAllocator.Deallocate(Storage);
}

FD3D12BufferAllocatorPool::FD3D12BufferAllocatorPool(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , InitialState(D3D12_RESOURCE_STATE_COMMON)
    , MaxSuballocationSize(0)
    , MultiBuddyAllocator(InDevice)
{
}

FD3D12BufferAllocatorPool::~FD3D12BufferAllocatorPool()
{
    Shutdown();
}

bool FD3D12BufferAllocatorPool::Initialize(D3D12_HEAP_TYPE InHeapType, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, D3D12_RESOURCE_STATES InInitialState)
{
    HeapType = InHeapType;
    InitialState = InInitialState;
    MaxSuballocationSize = InMaxSuballocationSize;
    return MultiBuddyAllocator.Initialize(InPageSizeBytes, InMinBlockBytes, HeapType, InitialState, false);
}

void FD3D12BufferAllocatorPool::Shutdown()
{
    MultiBuddyAllocator.Shutdown();
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

    const uint64 Size = ResourceDesc.Width;
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

        OutStorage.InitializeAsStandalone(Resource, ED3D12ResourceKind::Buffer, InHeapType, ED3D12HeapUsage::BufferOnly, ED3D12ResourceLifetime::Default, Size, Alignment);
        OutStorage.SetSubrangeInfo(0, Resource->GetGPUVirtualAddress(), MappedBaseAddress);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex = UINT32_MAX;
        AllocationData.Size = Size;
        AllocationData.bCommittedAllocation = true;
        OutStorage.SetPoolAllocationData(AllocationData);

        return true;
    }

    FD3D12ResourceAllocationRequest Request = {};
    Request.ResourceType = ED3D12ResourceType::Buffer;
    Request.HeapType = InHeapType;
    Request.InitialState = InInitialState;
    Request.ResourceFlags = ResourceDesc.Flags;
    Request.Size = Size;
    Request.Alignment = Alignment;
    Request.bHasResourceDesc = true;
    Request.ResourceDesc = ResourceDesc;

    if (!MultiBuddyAllocator.TryAllocate(Request, OutStorage))
    {
        return false;
    }

    return true;
}

bool FD3D12BufferAllocatorPool::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    const D3D12_RESOURCE_DESC Desc = Request.bHasResourceDesc ? Request.ResourceDesc : CreateBufferDesc(Request.Size, Request.ResourceFlags);
    return TryAllocate(Request.HeapType, Desc, EBufferFlags::None, Request.InitialState, Request.Alignment, OutStorage);
}

void FD3D12BufferAllocatorPool::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::MultiBuddyAllocator && Storage.GetOwner())
    {
        static_cast<FD3D12MultiBuddyAllocator*>(Storage.GetOwner())->Deallocate(Storage);
        return;
    }

    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::PoolAllocator && Storage.GetOwner())
    {
        static_cast<FD3D12PoolAllocator*>(Storage.GetOwner())->Deallocate(Storage);
        return;
    }

    if (Storage.GetResource() && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
    }
}

FD3D12BufferAllocator::FD3D12BufferAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(0)
    , MinBlockBytes(0)
    , MaxSuballocationSize(0)
{
}

FD3D12BufferAllocator::~FD3D12BufferAllocator()
{
    Shutdown();
}

bool FD3D12BufferAllocator::Initialize(uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize)
{
    PageSizeBytes = InPageSizeBytes;
    MinBlockBytes = InMinBlockBytes;
    MaxSuballocationSize = InMaxSuballocationSize;

    Shutdown();

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
        FD3D12BufferAllocatorPool* Pool = new FD3D12BufferAllocatorPool(GetDevice());
        if (!Pool->Initialize(HeapTypes[Index], PageSizeBytes, MinBlockBytes, MaxSuballocationSize, InitialStates[Index]))
        {
            delete Pool;
            Shutdown();
            return false;
        }

        Pools.Add(Pool);
    }

    return true;
}

void FD3D12BufferAllocator::Shutdown()
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

    FD3D12BufferAllocatorPool* NewPool = new FD3D12BufferAllocatorPool(GetDevice());
    if (!NewPool->Initialize(InHeapType, PageSizeBytes, MinBlockBytes, MaxSuballocationSize, InitialState))
    {
        delete NewPool;
        return false;
    }

    Pools.Add(NewPool);
    return NewPool->TryAllocate(InHeapType, ResourceDesc, BufferUsage, InitialState, Alignment, OutStorage);
}

bool FD3D12BufferAllocator::TryAllocate(const FD3D12ResourceAllocationRequest& Request, FD3D12ResourceStorage& OutStorage)
{
    D3D12_RESOURCE_DESC Desc = Request.bHasResourceDesc ? Request.ResourceDesc : CreateBufferDesc(Request.Size, Request.ResourceFlags);
    return TryAllocate(Request.HeapType, Desc, EBufferFlags::None, Request.InitialState, Request.Alignment, OutStorage);
}

void FD3D12BufferAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::BufferAllocatorPool && Storage.GetOwner())
    {
        static_cast<FD3D12BufferAllocatorPool*>(Storage.GetOwner())->Deallocate(Storage);
        return;
    }

    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::MultiBuddyAllocator && Storage.GetOwner())
    {
        static_cast<FD3D12MultiBuddyAllocator*>(Storage.GetOwner())->Deallocate(Storage);
        return;
    }

    if (Storage.GetOwnerType() == ED3D12ResourceStorageOwnerType::PoolAllocator && Storage.GetOwner())
    {
        static_cast<FD3D12PoolAllocator*>(Storage.GetOwner())->Deallocate(Storage);
        return;
    }

    if (Storage.GetResource() && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
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
    CommittedThreshold = InCommittedThreshold;
    Pools.Clear();
    auto CreatePool = [this](ETexturePoolClass PoolClass, uint64 PoolAlignment) -> bool
    {
        FPool& Entry = Pools.Emplace();
        Entry.PoolClass = PoolClass;
        Entry.Pool = new FD3D12PoolAllocator(GetDevice());
        if (!Entry.Pool->Initialize(DefaultPageSizeBytes, PoolAlignment, CommittedThreshold, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COMMON, true))
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
    const bool bIsUAV = (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0;
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

        OutStorage.InitializeAsStandalone(NewResource, ED3D12ResourceKind::Texture, D3D12_HEAP_TYPE_DEFAULT, ED3D12HeapUsage::TextureOnly, ED3D12ResourceLifetime::Default, AllocationInfo.SizeInBytes, AllocationInfo.Alignment);
        OutStorage.SetSubrangeInfo(0, NewResource->GetGPUVirtualAddress(), nullptr);
        OutStorage.SetOwner(this, ED3D12ResourceStorageOwnerType::TextureAllocator);
        OutStorage.SetResidencyHandle(RegisterResidency(GetDevice(), NewResource->GetD3D12Resource(), AllocationInfo.SizeInBytes));
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
    BlockRequest.ResourceType = ED3D12ResourceType::Texture;
    BlockRequest.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    BlockRequest.InitialState = Request.InitialState;
    BlockRequest.Size = AllocationInfo.SizeInBytes;
    BlockRequest.Alignment = AllocationInfo.Alignment;
    BlockRequest.bAllowCommittedFallback = false;

    FD3D12ResourceStorage BlockResourceStorage;
    if (!Pools[PoolIndex].Pool || !Pools[PoolIndex].Pool->TryAllocate(BlockRequest, BlockResourceStorage))
    {
        return false;
    }

    FD3D12Heap* Heap = BlockResourceStorage.GetHeap();
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

    OutStorage = Move(BlockResourceStorage);
    OutStorage.SetResource(NewResource);
    OutStorage.SetSubrangeInfo(0, NewResource->GetGPUVirtualAddress(), nullptr);
    OutStorage.SetSize(AllocationInfo.SizeInBytes);
    return true;
}

void FD3D12TextureAllocator::Deallocate(const FD3D12ResourceStorage& Storage)
{
    if (Storage.GetResource() && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(Storage.GetResource());
    }
}
