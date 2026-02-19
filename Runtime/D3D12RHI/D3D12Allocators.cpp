#include "Core/Math/Math.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12DeletionQueue.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"

static D3D12_RESOURCE_DESC ApplyTightAlignmentFlag(const D3D12_RESOURCE_DESC& ResourceDesc)
{
    D3D12_RESOURCE_DESC Result = ResourceDesc;

    if (GD3D12SupportTightAlignment)
    {
        // Tight-alignment resources must use Alignment=0 for Create/GetResourceAllocationInfo.
        Result.Flags    |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
        Result.Alignment = 0;
    }

    return Result;
}

FD3D12BuddyAllocator::FD3D12BuddyAllocator(FD3D12Device* InDevice, uint64 InBackingStorageSize, uint64 InMinBlockBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags)
    : FD3D12DeviceChild(InDevice)
    , BackingStorageSize(InBackingStorageSize)
    , MinBlockBytes(Math::Max<uint64>(InMinBlockBytes, 256ull))
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , ResourceFlags(GD3D12SupportTightAlignment ? (InResourceFlags | D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) : InResourceFlags)
    , BackingHeap(nullptr)
    , BackingResource(nullptr)
    , FreeOffsets()
    , MappedBaseAddress(nullptr)
    , AllocatorCS()
{
}

FD3D12BuddyAllocator::~FD3D12BuddyAllocator()
{
    Destroy();
}

bool FD3D12BuddyAllocator::Initialize()
{
    SCOPED_LOCK(AllocatorCS);

    Destroy();

    uint64 BackingSize = MinBlockBytes;
    while (BackingSize < BackingStorageSize)
    {
        BackingSize <<= 1;
    }

    BackingStorageSize = BackingSize;
    BackingHeap        = nullptr;
    BackingResource    = nullptr;
    MappedBaseAddress  = nullptr;
    
    if (AllocationStrategy == EAllocationStrategy::SuballocatedHeap)
    {
        D3D12_HEAP_DESC HeapDesc = {};
        HeapDesc.Properties.Type                 = HeapType;
        HeapDesc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapDesc.Properties.VisibleNodeMask      = 1;
        HeapDesc.Properties.CreationNodeMask     = 1;
        HeapDesc.SizeInBytes                     = BackingStorageSize;
        HeapDesc.Alignment                       = 0;
        HeapDesc.Flags                           = D3D12_HEAP_FLAG_NONE;

        FD3D12HeapRef NewHeap;
        if (!GetDevice()->CreateHeap(HeapDesc, NewHeap))
        {
            return false;
        }

        NewHeap->StartResidencyTracking();
        BackingHeap = NewHeap;
    }
    else
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = ResourceFlags;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = BackingStorageSize;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        Desc = ApplyTightAlignmentFlag(Desc);

        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, NewResource))
        {
            return false;
        }

        BackingResource = NewResource;
    }

    if (BackingResource)
    {
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = static_cast<uint8*>(BackingResource->MapRange(0, nullptr));
        }
    }
    
    const uint32 MaxOrder = GetOrderForSize(BackingStorageSize);
    FreeOffsets.Resize(MaxOrder + 1);
    FreeOffsets[MaxOrder].Add(0);

    return true;
}

void FD3D12BuddyAllocator::Destroy()
{
    if (BackingHeap)
    {
        BackingHeap->EndResidencyTracking();
        BackingHeap = nullptr;
    }

    BackingResource   = nullptr;
    MappedBaseAddress = nullptr;

    FreeOffsets.Clear();
}

bool FD3D12BuddyAllocator::Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags) const
{
    if (InHeapType != HeapType || InAllocationStrategy != AllocationStrategy)
    {
        return false;
    }

    if (AllocationStrategy == EAllocationStrategy::SuballocatedResource)
    {
        return InInitialState == InitialState && InResourceFlags == ResourceFlags;
    }

    return true;
}

uint32 FD3D12BuddyAllocator::GetOrderForSize(uint64 SizeInBytes) const
{
    uint32 Order     = 0;
    uint64 BlockSize = MinBlockBytes;
    
    while (BlockSize < SizeInBytes)
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

bool FD3D12BuddyAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    if (SizeInBytes == 0 || FreeOffsets.IsEmpty())
    {
        return false;
    }

    const uint64 UsedAlignment  = Alignment ? Alignment : MinBlockBytes;
    const uint64 AllocationSize = Math::AlignUp<uint64>(Math::Max(SizeInBytes, UsedAlignment), MinBlockBytes);

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
        OutStorage.SetResource(BackingResource.Get());
        OutStorage.SetResourceOffset(Offset);
        OutStorage.SetGpuVirtualAddress(BackingResource->GetGPUVirtualAddress() + Offset);
        OutStorage.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + Offset) : nullptr);
        OutStorage.SetStorageType(EResourceStorageType::SuballocatedResource);
    }
    else
    {
        OutStorage.SetResourceOffset(Offset);
        OutStorage.SetGpuVirtualAddress(0);
        OutStorage.SetMappedBaseAddress(nullptr);
        OutStorage.SetStorageType(EResourceStorageType::SuballocatedHeap);
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

    uint64 CurrentOffset = AllocationData.Offset;
    uint32 CurrentOrder  = AllocationData.Order;

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

bool FD3D12BuddyAllocator::IsEmpty() const
{
    SCOPED_LOCK(AllocatorCS);

    if (FreeOffsets.IsEmpty())
    {
        return true;
    }

    const uint32 MaxOrder = static_cast<uint32>(FreeOffsets.Size() - 1);
    return FreeOffsets[MaxOrder].Size() == 1 && FreeOffsets[MaxOrder][0] == 0;
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
    Destroy();
}

bool FD3D12MultiBuddyAllocator::Initialize()
{
    Destroy();
    return true;
}

void FD3D12MultiBuddyAllocator::Destroy()
{
    SCOPED_LOCK(AllocatorsCS);

    for (FD3D12BuddyAllocator* Allocator : Allocators)
    {
        delete Allocator;
    }

    Allocators.Clear();
}

bool FD3D12MultiBuddyAllocator::Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags) const
{
    if (InHeapType != HeapType || InAllocationStrategy != AllocationStrategy)
    {
        return false;
    }

    if (AllocationStrategy == EAllocationStrategy::SuballocatedResource && InInitialState != InitialState)
    {
        return false;
    }

    SCOPED_LOCK(AllocatorsCS);

    for (const FD3D12BuddyAllocator* Allocator : Allocators)
    {
        if (Allocator && Allocator->Supports(InHeapType, InInitialState, InAllocationStrategy, InResourceFlags))
        {
            return true;
        }
    }

    return true;
}

bool FD3D12MultiBuddyAllocator::CreateAllocator(D3D12_RESOURCE_FLAGS InResourceFlags)
{
    FD3D12BuddyAllocator* Allocator = new FD3D12BuddyAllocator(GetDevice(), PageSizeBytes, MinBlockBytes, HeapType, InitialState, AllocationStrategy, InResourceFlags);
    if (!Allocator->Initialize())
    {
        delete Allocator;
        return false;
    }

    Allocators.Add(Allocator);
    return true;
}

void FD3D12MultiBuddyAllocator::CleanUp()
{
    SCOPED_LOCK(AllocatorsCS);

    for (int32 Index = Allocators.Size() - 1; Index >= 0; --Index)
    {
        FD3D12BuddyAllocator* Allocator = Allocators[Index];
        if (Allocator && Allocator->IsEmpty())
        {
            delete Allocator;
            Allocators.RemoveAtSwap(Index);
        }
    }
}

bool FD3D12MultiBuddyAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage, D3D12_RESOURCE_FLAGS InResourceFlags)
{
    SCOPED_LOCK(AllocatorsCS);

    for (FD3D12BuddyAllocator* Allocator : Allocators)
    {
        if (!Allocator)
        {
            continue;
        }

        if (!Allocator->Supports(HeapType, InitialState, AllocationStrategy, InResourceFlags))
        {
            continue;
        }

        if (Allocator->TryAllocate(SizeInBytes, Alignment, OutStorage))
        {
            return true;
        }
    }

    if (!CreateAllocator(InResourceFlags))
    {
        return false;
    }

    FD3D12BuddyAllocator* Allocator = Allocators[Allocators.Size() - 1];
    if (!Allocator || !Allocator->TryAllocate(SizeInBytes, Alignment, OutStorage))
    {
        return false;
    }

    return true;
}

FD3D12PoolAllocatorPage::FD3D12PoolAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , UsedBytes(0)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , ResourceFlags(InResourceFlags)
    , BackingHeap(nullptr)
    , BackingResource(nullptr)
    , MappedBaseAddress(nullptr)
    , BaseGpuVirtualAddress(0)
    , FreeRanges()
{
}

FD3D12PoolAllocatorPage::~FD3D12PoolAllocatorPage()
{
    if (BackingHeap)
    {
        BackingHeap->EndResidencyTracking();
    }
}

bool FD3D12PoolAllocatorPage::Initialize()
{
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

        NewHeap->StartResidencyTracking();
        BackingHeap = NewHeap;
    }
    else
    {
        D3D12_RESOURCE_DESC Desc = {};
        Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Flags              = ResourceFlags;
        Desc.Format             = DXGI_FORMAT_UNKNOWN;
        Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Width              = PageSizeBytes;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        Desc = ApplyTightAlignmentFlag(Desc);

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

bool FD3D12PoolAllocatorPage::TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FD3D12ResourceStorage& OutStorage)
{
    for (int32 Index = 0; Index < FreeRanges.Size(); ++Index)
    {
        const FFreeRange Range = FreeRanges[Index];
        
        const uint64 UsedAlignment = Math::Max<uint64>(InAlignment, Alignment);
        const uint64 AlignedOffset = Math::AlignUp<uint64>(Range.Offset, UsedAlignment);
        const uint64 Padding       = AlignedOffset - Range.Offset;
        const uint64 RequiredSize  = Padding + SizeInBytes;

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
                FreeRanges.Add({AlignedOffset + SizeInBytes, TailSize});
            }
        }
        else if (TailSize > 0)
        {
            FreeRanges[Index].Offset = AlignedOffset + SizeInBytes;
            FreeRanges[Index].Size   = TailSize;
        }
        else
        {
            FreeRanges.RemoveAtSwap(Index);
        }

        UsedBytes += SizeInBytes;

        OutStorage.Reset();
        OutStorage.SetSize(SizeInBytes);

        if (BackingResource)
        {
            OutStorage.SetResource(BackingResource.Get());
            OutStorage.SetGpuVirtualAddress(BackingResource->GetGPUVirtualAddress() + AlignedOffset);
            OutStorage.SetStorageType(EResourceStorageType::SuballocatedResource);
        }
        else
        {
            OutStorage.SetGpuVirtualAddress(0);
            OutStorage.SetStorageType(EResourceStorageType::SuballocatedHeap);
        }

        OutStorage.SetResourceOffset(AlignedOffset);
        OutStorage.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + AlignedOffset) : nullptr);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex = InPageIndex;
        AllocationData.Offset    = AlignedOffset;
        AllocationData.Size      = SizeInBytes;

        OutStorage.SetPoolAllocationData(AllocationData);

        FLiveAllocation LiveAlloc = {};
        LiveAlloc.Offset = AlignedOffset;
        LiveAlloc.Size   = SizeInBytes;
        LiveAllocations.Add(LiveAlloc);

        return true;
    }

    return false;
}

bool FD3D12PoolAllocatorPage::TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, FD3D12PoolAllocatorAllocationData& OutData)
{
    for (int32 Index = 0; Index < FreeRanges.Size(); ++Index)
    {
        const FFreeRange Range = FreeRanges[Index];

        const uint64 UsedAlignment = Math::Max<uint64>(InAlignment, Alignment);
        const uint64 AlignedOffset = Math::AlignUp<uint64>(Range.Offset, UsedAlignment);
        const uint64 Padding       = AlignedOffset - Range.Offset;
        const uint64 RequiredSize  = Padding + SizeInBytes;

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
                FreeRanges.Add({AlignedOffset + SizeInBytes, TailSize});
            }
        }
        else if (TailSize > 0)
        {
            FreeRanges[Index].Offset = AlignedOffset + SizeInBytes;
            FreeRanges[Index].Size   = TailSize;
        }
        else
        {
            FreeRanges.RemoveAtSwap(Index);
        }

        UsedBytes += SizeInBytes;

        OutData.Offset = AlignedOffset;
        OutData.Size   = SizeInBytes;
        return true;
    }

    return false;
}

void FD3D12PoolAllocatorPage::RecycleAllocation(uint64 Offset, uint64 SizeInBytes)
{
    if (SizeInBytes == 0)
    {
        return;
    }

    UnregisterOwner(Offset);

    FreeRanges.Add({Offset, SizeInBytes});
    UsedBytes = UsedBytes > SizeInBytes ? (UsedBytes - SizeInBytes) : 0;
    CoalesceFreeRanges();
}

void FD3D12PoolAllocatorPage::RegisterOwner(uint64 Offset, FD3D12BaseResource* Owner)
{
    for (FLiveAllocation& Alloc : LiveAllocations)
    {
        if (Alloc.Offset == Offset)
        {
            Alloc.Owner = Owner;
            return;
        }
    }

    FLiveAllocation NewAlloc = {};
    NewAlloc.Offset = Offset;
    NewAlloc.Owner  = Owner;
    LiveAllocations.Add(NewAlloc);
}

void FD3D12PoolAllocatorPage::UnregisterOwner(uint64 Offset)
{
    for (int32 Index = 0; Index < LiveAllocations.Size(); ++Index)
    {
        if (LiveAllocations[Index].Offset == Offset)
        {
            LiveAllocations.RemoveAtSwap(Index);
            return;
        }
    }
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

FD3D12PoolAllocator::FD3D12PoolAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxResourceSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, D3D12_RESOURCE_FLAGS InResourceFlags)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , MaxResourceSize(InMaxResourceSize)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , ResourceFlags(GD3D12SupportTightAlignment ? (InResourceFlags | D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) : InResourceFlags)
    , FragmentedBytes(0)
    , Pages()
    , PagesCS()
{
}

FD3D12PoolAllocator::~FD3D12PoolAllocator()
{
    Destroy();
}

bool FD3D12PoolAllocator::Initialize()
{
    Destroy();
    return true;
}

void FD3D12PoolAllocator::Destroy()
{
    SCOPED_LOCK(PagesCS);

    for (FD3D12PoolAllocatorPage* Page : Pages)
    {
        delete Page;
    }

    Pages.Clear();
    FragmentedBytes = 0;
}

void FD3D12PoolAllocator::CleanUp()
{
    SCOPED_LOCK(PagesCS);

    for (int32 Index = Pages.Size() - 1; Index >= 0; --Index)
    {
        FD3D12PoolAllocatorPage* Page = Pages[Index];
        if (Page && Page->IsEmpty())
        {
            delete Page;
            Pages[Index] = nullptr;
        }
    }

    while (!Pages.IsEmpty() && Pages[Pages.Size() - 1] == nullptr)
    {
        Pages.Pop();
    }

}

bool FD3D12PoolAllocator::Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_FLAGS InResourceFlags) const
{
    if (InHeapType != HeapType || InAllocationStrategy != AllocationStrategy)
    {
        return false;
    }

    if (ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_UNKNOWN)
    {
        return false;
    }

    if (AllocationStrategy == EAllocationStrategy::SuballocatedResource)
    {
        if (ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            return false;
        }

        if (InInitialState != InitialState)
        {
            return false;
        }

        if (InResourceFlags != ResourceFlags)
        {
            return false;
        }
    }

    return true;
}

FD3D12PoolAllocatorPage* FD3D12PoolAllocator::CreatePage(uint64 MinimumSize, uint32& OutPageIndex)
{
    OutPageIndex = UINT32_MAX;

    const uint64 RequiredSize   = Math::AlignUp<uint64>(MinimumSize, Alignment);
    const uint64 ActualPageSize = Math::Max(PageSizeBytes, RequiredSize);

    FD3D12PoolAllocatorPage* NewPage = new FD3D12PoolAllocatorPage(GetDevice(), ActualPageSize, Alignment, HeapType, InitialState, AllocationStrategy, ResourceFlags);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

    OutPageIndex = static_cast<uint32>(Pages.Size());
    Pages.Add(NewPage);

    return NewPage;
}

void FD3D12PoolAllocator::ComputeTLSFIndices(uint64 SizeInBytes, uint32& OutFL, uint32& OutSL) const
{
    OutFL = 0;
    OutSL = 0;

    if (SizeInBytes == 0)
    {
        return;
    }

    uint64 Temp       = SizeInBytes;
    uint32 FirstLevel = 0;

    while (Temp > 1)
    {
        Temp >>= 1;
        ++FirstLevel;
    }

    OutFL = Math::Min<uint32>(FirstLevel, TLSFFirstLevelCount - 1);

    const uint64 RangeBase     = 1ull << OutFL;
    const uint64 OffsetInRange = SizeInBytes - RangeBase;
    const uint64 SL            = (OffsetInRange * TLSFSecondLevelCount) / RangeBase;

    OutSL = Math::Min<uint32>(static_cast<uint32>(SL), TLSFSecondLevelCount - 1);
}

void FD3D12PoolAllocator::RebuildFragmentationData()
{
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
        }
    }
}

void FD3D12PoolAllocator::RegisterAllocationOwner(const FD3D12PoolAllocatorAllocationData& Data, FD3D12BaseResource* Owner)
{
    if (Data.PageIndex == UINT32_MAX || !Owner)
    {
        return;
    }

    SCOPED_LOCK(PagesCS);

    if (Data.PageIndex < static_cast<uint32>(Pages.Size()) && Pages[Data.PageIndex])
    {
        Pages[Data.PageIndex]->RegisterOwner(Data.Offset, Owner);
    }
}

bool FD3D12PoolAllocator::GetDefragCandidate(FDefragCandidate& OutCandidate) const
{
    SCOPED_LOCK(PagesCS);

    if (AllocationStrategy != EAllocationStrategy::SuballocatedHeap)
    {
        return false;
    }

    uint32 BestPageIndex     = UINT32_MAX;
    uint64 LowestUtilization = UINT64_MAX;

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        const FD3D12PoolAllocatorPage* Page = Pages[PageIndex];
        if (!Page || Page->IsEmpty() || Page->GetFreeRanges().Size() <= 1)
        {
            continue;
        }

        const uint64 Utilization = Page->GetUsedBytes();
        if (Utilization < LowestUtilization)
        {
            LowestUtilization = Utilization;
            BestPageIndex     = PageIndex;
        }
    }

    if (BestPageIndex == UINT32_MAX)
    {
        return false;
    }

    const FD3D12PoolAllocatorPage* SourcePage = Pages[BestPageIndex];
    for (const auto& LiveAlloc : SourcePage->GetLiveAllocations())
    {
        if (LiveAlloc.Owner)
        {
            OutCandidate.Owner     = LiveAlloc.Owner;
            OutCandidate.PageIndex = BestPageIndex;
            OutCandidate.Offset    = LiveAlloc.Offset;
            OutCandidate.Size      = LiveAlloc.Size;
            return true;
        }
    }

    return false;
}

bool FD3D12PoolAllocator::TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, uint32 ExcludePageIndex, FD3D12PoolAllocatorAllocationData& OutData)
{
    SCOPED_LOCK(PagesCS);

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        if (PageIndex == ExcludePageIndex)
        {
            continue;
        }

        FD3D12PoolAllocatorPage* Page = Pages[PageIndex];
        if (!Page)
        {
            continue;
        }

        if (Page->TryAllocateForDefrag(SizeInBytes, InAlignment, OutData))
        {
            OutData.PageIndex = PageIndex;
            return true;
        }
    }

    return false;
}

bool FD3D12PoolAllocator::TryAllocate(const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InInitialState, uint64 InAlignment, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceStorage& OutStorage)
{
    D3D12_RESOURCE_DESC AllocationDesc = ApplyTightAlignmentFlag(ResourceDesc);

    if (!Supports(HeapType, InInitialState, AllocationStrategy, AllocationDesc, AllocationDesc.Flags))
    {
        return false;
    }

    uint64 SizeInBytes = 0;
    if (AllocationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        SizeInBytes = AllocationDesc.Width;
    }
    else
    {
        const D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &AllocationDesc);
        SizeInBytes = AllocationInfo.SizeInBytes;
    }

    if (SizeInBytes == 0)
    {
        return false;
    }

    const uint64 UsedAlignment = InAlignment ? InAlignment : Alignment;
    const uint64 SizeAligned   = Math::AlignUp<uint64>(SizeInBytes, UsedAlignment);

    if (SizeAligned > MaxResourceSize)
    {
        D3D12_RESOURCE_DESC Desc = AllocationDesc;
        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && Desc.Width == 0)
        {
            Desc.Width = SizeAligned;
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

        OutStorage.InitStandalone(NewResource.Get());
        OutStorage.SetSize(SizeAligned);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(NewResource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(MappedBaseAddress);

        FD3D12PoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex     = UINT32_MAX;
        AllocationData.Offset        = 0;
        AllocationData.Size          = SizeAligned;

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
    const EResourceStorageType StorageType = Storage.GetStorageType();
    if (StorageType != EResourceStorageType::SuballocatedHeap && StorageType != EResourceStorageType::SuballocatedResource)
    {
        return;
    }

    const FD3D12PoolAllocatorAllocationData Data = Storage.GetPoolAllocationData();
    if (Data.PageIndex == UINT32_MAX || Data.Size == 0)
    {
        return;
    }

    FD3D12RHI::DeferDeletion(ED3D12DeferredAllocatorType::Pool, this, Data);
}

void FD3D12PoolAllocator::RecycleAllocation(const FD3D12PoolAllocatorAllocationData& Data)
{
    if (Data.PageIndex == UINT32_MAX || Data.Size == 0)
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
    RebuildFragmentationData();
}

FD3D12Heap* FD3D12PoolAllocator::GetBackingHeap(uint32 PageIndex)
{
    SCOPED_LOCK(PagesCS);

    if (PageIndex >= static_cast<uint32>(Pages.Size()))
    {
        return nullptr;
    }

    FD3D12PoolAllocatorPage* Page = Pages[PageIndex];
    return Page ? Page->GetBackingHeap() : nullptr;
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

    Desc = ApplyTightAlignmentFlag(Desc);

    FD3D12ResourceRef Resource;
    if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
    {
        return false;
    }

    Bucket.BackingResource   = Resource;
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

bool FD3D12BucketAllocator::TryAllocate(uint64 SizeInBytes, FD3D12ResourceStorage& OutStorage)
{
    SCOPED_LOCK(BucketsCS);

    for (uint32 BucketIndex = 0; BucketIndex < static_cast<uint32>(Buckets.Size()); ++BucketIndex)
    {
        FBucket& Bucket = Buckets[BucketIndex];
        if (SizeInBytes > Bucket.BlockSize)
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
        OutStorage.SetResource(Bucket.BackingResource.Get());
        OutStorage.SetResourceOffset(AllocationData.Offset);
        OutStorage.SetGpuVirtualAddress(Bucket.BackingResource ? (Bucket.BackingResource->GetGPUVirtualAddress() + AllocationData.Offset) : 0);
        OutStorage.SetMappedBaseAddress(Bucket.MappedBaseAddress ? (Bucket.MappedBaseAddress + AllocationData.Offset) : nullptr);
        OutStorage.SetSize(Bucket.BlockSize);
        OutStorage.SetStorageType(EResourceStorageType::SuballocatedResource);

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
    Destroy();
}

bool FD3D12UploadHeapAllocator::Initialize()
{
    Destroy();

    if (!SmallAllocator.Initialize())
    {
        Destroy();
        return false;
    }

    if (!LargeAllocator.Initialize())
    {
        Destroy();
        return false;
    }

    if (!ConstantsAllocator.Initialize())
    {
        Destroy();
        return false;
    }

    return true;
}

void FD3D12UploadHeapAllocator::Destroy()
{
    ConstantsAllocator.Destroy();
    LargeAllocator.Destroy();
    SmallAllocator.Destroy();
}

void FD3D12UploadHeapAllocator::CleanUp()
{
    SmallAllocator.CleanUp();
    LargeAllocator.CleanUp();
    ConstantsAllocator.CleanUp();
}

void* FD3D12UploadHeapAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    const uint64 UsedAlignment = Alignment ? Alignment : DefaultAlignment;

    if (SizeInBytes <= SmallAllocationThreshold)
    {
        if (!SmallAllocator.TryAllocate(SizeInBytes, UsedAlignment, OutStorage))
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
        Desc.Width              = SizeInBytes;
        Desc.Height             = 1;
        Desc.DepthOrArraySize   = 1;
        Desc.MipLevels          = 1;
        Desc.Alignment          = 0;
        Desc.SampleDesc.Count   = 1;
        Desc.SampleDesc.Quality = 0;

        Desc = ApplyTightAlignmentFlag(Desc);

        if (!LargeAllocator.TryAllocate(Desc, D3D12_RESOURCE_STATE_GENERIC_READ, UsedAlignment, nullptr, OutStorage))
        {
            return nullptr;
        }
    }

    return OutStorage.GetMappedBaseAddress();
}

void* FD3D12UploadHeapAllocator::AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    const uint64 UsedAlignment = Alignment ? Alignment : D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

    if (!ConstantsAllocator.TryAllocate(SizeInBytes, UsedAlignment, OutStorage))
    {
        return nullptr;
    }

    return OutStorage.GetMappedBaseAddress();
}

FD3D12LinearAllocatorPage::FD3D12LinearAllocatorPage(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , CurrentOffset(0)
    , BackingResourceStorage(InDevice)
{
}

bool FD3D12LinearAllocatorPage::Initialize()
{
    FD3D12Device* CurrentDevice = GetDevice();

    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
    {
        FD3D12UploadHeapAllocator* UploadHeapAllocator = CurrentDevice->GetUploadHeapAllocator();
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

    Desc = ApplyTightAlignmentFlag(Desc);

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

    BackingResourceStorage.InitStandalone(Resource.Get());
    BackingResourceStorage.SetSize(PageSizeBytes);
    BackingResourceStorage.SetResourceOffset(0);
    BackingResourceStorage.SetGpuVirtualAddress(Resource->GetGPUVirtualAddress());
    BackingResourceStorage.SetMappedBaseAddress(MappedBaseAddress);
    return true;
}

bool FD3D12LinearAllocatorPage::TryAllocate(uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset)
{
    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 AlignedOffset = Math::AlignUp<uint64>(CurrentOffset, UsedAlignment);

    if (AlignedOffset + SizeInBytes > PageSizeBytes)
    {
        return false;
    }

    OutOffset     = AlignedOffset;
    CurrentOffset = AlignedOffset + SizeInBytes;
    return true;
}

void FD3D12LinearAllocatorPage::Reset()
{
    BackingResourceStorage.ReleaseResource();
    CurrentOffset = 0;
}

FD3D12LinearAllocator::FD3D12LinearAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12DeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
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
    FD3D12LinearAllocatorPage* NewPage = new FD3D12LinearAllocatorPage(GetDevice(), PageSizeBytes, HeapType, InitialState);
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

    Page->Reset();
}

void* FD3D12LinearAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FD3D12ResourceStorage& OutStorage)
{
    if (SizeInBytes == 0)
    {
        return nullptr;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 SizeAligned   = Math::AlignUp<uint64>(SizeInBytes, UsedAlignment);

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

        Desc = ApplyTightAlignmentFlag(Desc);

        if (!GetDevice()->CreateCommittedResource(Desc, HeapType, InitialState, nullptr, Resource))
        {
            return nullptr;
        }

        void* MappedBaseAddress = nullptr;
        if (HeapType == D3D12_HEAP_TYPE_UPLOAD || HeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = Resource->MapRange(0, nullptr);
        }

        OutStorage.InitStandalone(Resource.Get());
        OutStorage.SetSize(SizeAligned);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(Resource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(MappedBaseAddress);

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

    const FD3D12ResourceStorage&    BackingStorage     = SelectedPage->GetBackingResourceStorage();
    const uint64                    PageResourceOffset = BackingStorage.GetResourceOffset() + AllocationOffset;
    const D3D12_GPU_VIRTUAL_ADDRESS BaseGpuAddress     = BackingStorage.GetGpuVirtualAddress();
    const D3D12_GPU_VIRTUAL_ADDRESS PageGpuAddress     = BaseGpuAddress ? (BaseGpuAddress + AllocationOffset) : 0;
    
    uint8* MappedBase = static_cast<uint8*>(BackingStorage.GetMappedBaseAddress());
    OutStorage.Reset();

    if (FD3D12Resource* PageResource = BackingStorage.GetResource())
    {
        OutStorage.SetResource(PageResource);
    }

    OutStorage.SetResourceOffset(PageResourceOffset);
    OutStorage.SetGpuVirtualAddress(PageGpuAddress);
    OutStorage.SetMappedBaseAddress(MappedBase ? (MappedBase + AllocationOffset) : nullptr);
    OutStorage.SetSize(SizeAligned);
    OutStorage.SetStorageType(EResourceStorageType::SuballocatedResource);

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

void FD3D12LinearAllocator::CleanUp()
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

FD3D12DynamicConstantsAllocator::FD3D12DynamicConstantsAllocator(FD3D12Device* InDevice, uint64 InPageSizeBytes)
    : FD3D12DeviceChild(InDevice)
    , LinearAllocator(InDevice, InPageSizeBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ)
{
}

void FD3D12DynamicConstantsAllocator::CleanUp()
{
    LinearAllocator.CleanUp();
}

void* FD3D12DynamicConstantsAllocator::Allocate(uint64 SizeInBytes, FD3D12ResourceStorage& OutStorage)
{
    return LinearAllocator.Allocate(SizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, OutStorage);
}

FD3D12BufferAllocatorPool::FD3D12BufferAllocatorPool(FD3D12Device* InDevice, D3D12_HEAP_TYPE InHeapType, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy)
    : FD3D12DeviceChild(InDevice)
    , HeapType(InHeapType)
    , InitialState(InInitialState)
    , AllocationStrategy(InAllocationStrategy)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
    , MultiBuddyAllocator(InDevice, InPageSizeBytes, InMinBlockBytes, InHeapType, InInitialState, InAllocationStrategy)
{
}

FD3D12BufferAllocatorPool::~FD3D12BufferAllocatorPool()
{
    Destroy();
}

bool FD3D12BufferAllocatorPool::Initialize()
{
    Destroy();

    if (!MultiBuddyAllocator.Initialize())
    {
        return false;
    }

    return true;
}

void FD3D12BufferAllocatorPool::CleanUp()
{
    MultiBuddyAllocator.CleanUp();
}

void FD3D12BufferAllocatorPool::Destroy()
{
    MultiBuddyAllocator.Destroy();
}

D3D12_RESOURCE_STATES FD3D12BufferAllocatorPool::GetInitialResourceStateForHeapType(D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES RequestedInitialState)
{
    switch (HeapType)
    {
    case D3D12_HEAP_TYPE_UPLOAD:
        if (RequestedInitialState != D3D12_RESOURCE_STATE_GENERIC_READ)
        {
            D3D12_WARNING("Invalid initial state (0x%llX) for upload heap. Forcing D3D12_RESOURCE_STATE_GENERIC_READ.", static_cast<unsigned long long>(RequestedInitialState));
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        }
        break;
    case D3D12_HEAP_TYPE_READBACK:
        if (RequestedInitialState != D3D12_RESOURCE_STATE_COPY_DEST)
        {
            D3D12_WARNING("Invalid initial state (0x%llX) for readback heap. Forcing D3D12_RESOURCE_STATE_COPY_DEST.", static_cast<unsigned long long>(RequestedInitialState));
            return D3D12_RESOURCE_STATE_COPY_DEST;
        }
        break;
    default:
        break;
    }

    return RequestedInitialState;
}

bool FD3D12BufferAllocatorPool::Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, EAllocationStrategy InAllocationStrategy, const D3D12_RESOURCE_DESC& ResourceDesc) const
{
    if (ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        return false;
    }

    return MultiBuddyAllocator.Supports(InHeapType, InInitialState, InAllocationStrategy, ResourceDesc.Flags);
}

bool FD3D12BufferAllocatorPool::TryAllocate(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_RESOURCE_STATES InInitialState, uint64 InAlignment, FD3D12ResourceStorage& OutStorage)
{
    D3D12_RESOURCE_DESC AllocationDesc = ApplyTightAlignmentFlag(ResourceDesc);

    const D3D12_RESOURCE_STATES EffectiveInitialState = GetInitialResourceStateForHeapType(InHeapType, InInitialState);
    if (!Supports(InHeapType, EffectiveInitialState, AllocationStrategy, AllocationDesc))
    {
        return false;
    }

    const uint64 SizeInBytes   = AllocationDesc.Width;
    const uint64 UsedAlignment = InAlignment ? InAlignment : 16;

    if (SizeInBytes > MaxSuballocationSize)
    {
        FD3D12ResourceRef Resource;
        if (!GetDevice()->CreateCommittedResource(AllocationDesc, InHeapType, EffectiveInitialState, nullptr, Resource))
        {
            return false;
        }

        void* MappedBaseAddress = nullptr;
        if (InHeapType == D3D12_HEAP_TYPE_UPLOAD || InHeapType == D3D12_HEAP_TYPE_READBACK)
        {
            MappedBaseAddress = Resource->MapRange(0, nullptr);
        }

        OutStorage.InitStandalone(Resource.Get());
        OutStorage.SetSize(SizeInBytes);
        OutStorage.SetResourceOffset(0);
        OutStorage.SetGpuVirtualAddress(Resource->GetGPUVirtualAddress());
        OutStorage.SetMappedBaseAddress(MappedBaseAddress);
        return true;
    }

    if (!MultiBuddyAllocator.TryAllocate(SizeInBytes, UsedAlignment, OutStorage, AllocationDesc.Flags))
    {
        return false;
    }

    if (OutStorage.GetStorageType() == EResourceStorageType::SuballocatedHeap)
    {
        const FD3D12BuddyAllocatorAllocationData& BuddyData = OutStorage.GetBuddyAllocationData();
        CHECK(BuddyData.BackingHeap != nullptr);

        FD3D12ResourceRef PlacedResource;
        if (!GetDevice()->CreatePlacedResource(BuddyData.BackingHeap, BuddyData.Offset, AllocationDesc, EffectiveInitialState, nullptr, PlacedResource))
        {
            return false;
        }

        OutStorage.SetResource(PlacedResource.Get());
        OutStorage.SetGpuVirtualAddress(PlacedResource->GetGPUVirtualAddress());
    }

    return true;
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
    Destroy();
}

EAllocationStrategy FD3D12BufferAllocator::GetAllocationStrategy(D3D12_HEAP_TYPE HeapType)
{
    return (HeapType == D3D12_HEAP_TYPE_DEFAULT) ? EAllocationStrategy::SuballocatedHeap : EAllocationStrategy::SuballocatedResource;
}

bool FD3D12BufferAllocator::Initialize()
{
    Destroy();

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
        const EAllocationStrategy Strategy = GetAllocationStrategy(HeapTypes[Index]);
        FD3D12BufferAllocatorPool* Pool = new FD3D12BufferAllocatorPool(GetDevice(), HeapTypes[Index], PageSizeBytes, MinBlockBytes, MaxSuballocationSize, InitialStates[Index], Strategy);
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

void FD3D12BufferAllocator::Destroy()
{
    ReleasePools();
}

void FD3D12BufferAllocator::CleanUp()
{
    SCOPED_LOCK(PoolsCS);

    for (FD3D12BufferAllocatorPool* Pool : Pools)
    {
        if (Pool)
        {
            Pool->CleanUp();
        }
    }
}

bool FD3D12BufferAllocator::Supports(D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, const D3D12_RESOURCE_DESC& ResourceDesc) const
{
    if (ResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        return false;
    }

    switch (InHeapType)
    {
    case D3D12_HEAP_TYPE_DEFAULT:
        return true;
    case D3D12_HEAP_TYPE_UPLOAD:
        return InInitialState == D3D12_RESOURCE_STATE_GENERIC_READ;
    case D3D12_HEAP_TYPE_READBACK:
        return InInitialState == D3D12_RESOURCE_STATE_COPY_DEST;
    default:
        return false;
    }
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
    D3D12_RESOURCE_DESC AllocationDesc = ApplyTightAlignmentFlag(ResourceDesc);

    const D3D12_RESOURCE_STATES EffectiveInitialState = FD3D12BufferAllocatorPool::GetInitialResourceStateForHeapType(InHeapType, InitialState);
    if (!Supports(InHeapType, EffectiveInitialState, AllocationDesc))
    {
        return false;
    }

    const EAllocationStrategy Strategy = GetAllocationStrategy(InHeapType);

    SCOPED_LOCK(PoolsCS);

    for (FD3D12BufferAllocatorPool* Pool : Pools)
    {
        if (!Pool->Supports(InHeapType, EffectiveInitialState, Strategy, AllocationDesc))
        {
            continue;
        }

        if (Pool->TryAllocate(InHeapType, AllocationDesc, EffectiveInitialState, Alignment, OutStorage))
        {
            return true;
        }
    }

    FD3D12BufferAllocatorPool* NewPool = new FD3D12BufferAllocatorPool(GetDevice(), InHeapType, PageSizeBytes, MinBlockBytes, MaxSuballocationSize, EffectiveInitialState, Strategy);
    if (!NewPool->Initialize())
    {
        delete NewPool;
        return false;
    }

    Pools.Add(NewPool);
    return NewPool->TryAllocate(InHeapType, AllocationDesc, EffectiveInitialState, Alignment, OutStorage);
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
    Destroy();
}

bool FD3D12TextureAllocator::Initialize()
{
    Destroy();

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

        if (!Pool->Initialize())
        {
            delete Pool;
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

void FD3D12TextureAllocator::Destroy()
{
    SCOPED_LOCK(PoolsCS);
    ReleasePools();
}

void FD3D12TextureAllocator::CleanUp()
{
    SCOPED_LOCK(PoolsCS);

    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
    {
        if (Pools[Index])
        {
            Pools[Index]->CleanUp();
        }
    }
}

void FD3D12TextureAllocator::RegisterAllocationOwner(FD3D12PoolAllocator* Allocator, const FD3D12PoolAllocatorAllocationData& Data, FD3D12BaseResource* Owner)
{
    if (Allocator)
    {
        Allocator->RegisterAllocationOwner(Data, Owner);
    }
}

bool FD3D12TextureAllocator::GetDefragCandidate(FD3D12PoolAllocator::FDefragCandidate& OutCandidate, FD3D12PoolAllocator*& OutAllocator)
{
    SCOPED_LOCK(PoolsCS);

    uint64 MostFragmented = 0;
    FD3D12PoolAllocator* BestPool = nullptr;

    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
    {
        if (Pools[Index] && Pools[Index]->GetFragmentedBytes() > MostFragmented)
        {
            MostFragmented = Pools[Index]->GetFragmentedBytes();
            BestPool       = Pools[Index];
        }
    }

    if (BestPool && BestPool->GetDefragCandidate(OutCandidate))
    {
        OutAllocator = BestPool;
        return true;
    }

    return false;
}

bool FD3D12TextureAllocator::Supports(D3D12_HEAP_TYPE InHeapType, const D3D12_RESOURCE_DESC& ResourceDesc) const
{
    if (InHeapType != D3D12_HEAP_TYPE_DEFAULT)
    {
        return false;
    }

    return ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D ||
           ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
           ResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D;
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

bool FD3D12TextureAllocator::CanUseSmallResourcePlacementAlignment(const D3D12_RESOURCE_DESC& Desc) const
{
    if (Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
        return false;
    }

    if (Desc.Layout != D3D12_TEXTURE_LAYOUT_UNKNOWN)
    {
        return false;
    }

    if ((Desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0)
    {
        return false;
    }

    if (Desc.SampleDesc.Count > 1 || Desc.SampleDesc.Quality != 0)
    {
        return false;
    }

    if (Desc.DepthOrArraySize != 1)
    {
        return false;
    }

    if (Desc.Width == 0 || Desc.Height == 0 || Desc.Width > 0xFFFFFFFFull)
    {
        return false;
    }

    uint32 SizeX = static_cast<uint32>(Desc.Width);
    uint32 SizeY = Desc.Height;

    uint32 BitsPerPixel = GetBitsPerPixel(Desc.Format);
    if (BitsPerPixel == 0)
    {
        return false;
    }

    if (IsFormatCompressed(Desc.Format))
    {
        SizeX         = Math::DivideByMultiple(SizeX, 4u);
        SizeY         = Math::DivideByMultiple(SizeY, 4u);
        BitsPerPixel *= 16u;
    }

    uint32 TileSizeX = 0;
    uint32 TileSizeY = 0;

    switch (BitsPerPixel)
    {
    case 8:
        TileSizeX = 64;
        TileSizeY = 64;
        break;
    case 16:
        TileSizeX = 64;
        TileSizeY = 32;
        break;
    case 32:
        TileSizeX = 32;
        TileSizeY = 32;
        break;
    case 64:
        TileSizeX = 32;
        TileSizeY = 16;
        break;
    case 128:
        TileSizeX = 16;
        TileSizeY = 16;
        break;
    default:
        return false;
    }

    const uint32 TileCountX = Math::DivideByMultiple(SizeX, TileSizeX);
    const uint32 TileCountY = Math::DivideByMultiple(SizeY, TileSizeY);
    const uint64 TileCount  = static_cast<uint64>(TileCountX) * static_cast<uint64>(TileCountY);

    return TileCount <= 16ull;
}

FD3D12TextureAllocator::ETexturePoolClass FD3D12TextureAllocator::ClassifyTexture(const D3D12_RESOURCE_DESC& Desc, uint64 Alignment) const
{
    if (Alignment == D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
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
    if (!Supports(D3D12_HEAP_TYPE_DEFAULT, ResourceDesc))
    {
        return false;
    }

    D3D12_RESOURCE_DESC AllocationDesc = ApplyTightAlignmentFlag(ResourceDesc);

    const bool bUseTightAlignment = (AllocationDesc.Flags & D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT) != 0;
    if (!bUseTightAlignment && CanUseSmallResourcePlacementAlignment(AllocationDesc))
    {
        AllocationDesc.Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
    }
    else
    {
        AllocationDesc.Alignment = 0;
    }

    D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &AllocationDesc);
    if (AllocationDesc.Alignment == D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT && AllocationInfo.Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
    {
        AllocationDesc.Alignment = 0;
        AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &AllocationDesc);
    }

    if (AllocationInfo.SizeInBytes >= CommittedThreshold)
    {
        FD3D12ResourceRef NewResource;
        if (!GetDevice()->CreateCommittedResource(AllocationDesc, D3D12_HEAP_TYPE_DEFAULT, InitialState, ClearValue, NewResource))
        {
            return false;
        }

        OutStorage.InitStandalone(NewResource.Get());
        OutStorage.SetSize(AllocationInfo.SizeInBytes);
        return true;
    }

    SCOPED_LOCK(PoolsCS);

    const uint32 PoolIndex = static_cast<uint32>(ClassifyTexture(AllocationDesc, AllocationInfo.Alignment));
    if (PoolIndex >= TexturePoolClassCount || !Pools[PoolIndex])
    {
        return false;
    }

    FD3D12ResourceStorage PoolResourceStorage(GetDevice());
    if (!Pools[PoolIndex]->TryAllocate(AllocationDesc, InitialState, AllocationInfo.Alignment, ClearValue, PoolResourceStorage))
    {
        return false;
    }

    const EResourceStorageType PoolStorageType = PoolResourceStorage.GetStorageType();
    if (PoolStorageType == EResourceStorageType::Standalone)
    {
        OutStorage.Swap(PoolResourceStorage);
        OutStorage.SetSize(AllocationInfo.SizeInBytes);
        return true;
    }
    
    const FD3D12PoolAllocatorAllocationData PoolAllocationData = PoolResourceStorage.GetPoolAllocationData();
    if (PoolStorageType != EResourceStorageType::SuballocatedHeap || PoolAllocationData.PageIndex == UINT32_MAX)
    {
        PoolResourceStorage.ReleaseResource();
        return false;
    }

    FD3D12Heap* Heap = Pools[PoolIndex]->GetBackingHeap(PoolAllocationData.PageIndex);
    if (!Heap)
    {
        PoolResourceStorage.ReleaseResource();
        return false;
    }

    FD3D12ResourceRef NewResource;
    if (!GetDevice()->CreatePlacedResource(Heap, PoolAllocationData.Offset, AllocationDesc, InitialState, ClearValue, NewResource))
    {
        PoolResourceStorage.ReleaseResource();
        return false;
    }

    OutStorage.Swap(PoolResourceStorage);
    OutStorage.SetResource(NewResource.Get());
    OutStorage.SetResourceOffset(0);
    OutStorage.SetGpuVirtualAddress(0);
    OutStorage.SetMappedBaseAddress(nullptr);
    OutStorage.SetStorageType(EResourceStorageType::SuballocatedHeap);
    OutStorage.SetSize(AllocationInfo.SizeInBytes);
    return true;
}
