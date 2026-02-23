#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

FD3D12Resource::FD3D12Resource(FD3D12Device* InDevice, ID3D12Resource* InResource, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, FD3D12Heap* InHeap)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Resource(InResource)
    , HeapType(InHeapType)
    , Desc(InResource ? InResource->GetDesc() : D3D12_RESOURCE_DESC{})
    , Address(0)
    , Heap(MakeSharedRef<FD3D12Heap>(InHeap))
    , NumSubresources(0)
    , bShouldDeferredRelease(true)
    , bHasClearValue(false)
{
    TrackedState.SetResourceState(InInitialState);

    if (Resource)
    {
        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            Address = Resource->GetGPUVirtualAddress();
        }

        const uint32 ArraySize = Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? Desc.DepthOrArraySize : 1u;
        NumSubresources = D3D12CalculateSubresourceCount(Desc.MipLevels, ArraySize, 1);
    }

    if (HeapType == D3D12_HEAP_TYPE_DEFAULT)
    {
        InitializeStateTracking(InInitialState);
    }
}

void FD3D12Resource::InitializeStateTracking(D3D12_RESOURCE_STATES InitialState)
{
    TrackedState.Initialize(Math::Max(NumSubresources, 1u));
    TrackedState.SetResourceState(InitialState);
}

void* FD3D12Resource::MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range)
{
    void* MappedData = nullptr;

    HRESULT Result = Resource->Map(SubresourceIndex, Range, &MappedData);
    if (FAILED(Result))
    {
        LOG_ERROR("[FD3D12Resource::Map] Failed");
        return nullptr;
    }
    else
    {
        return MappedData;
    }
}

void FD3D12Resource::UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range)
{
    Resource->Unmap(SubresourceIndex, Range);
}

void FD3D12Resource::SetDebugName(const FString& InDebugName)
{
    if (Resource)
    {
        HRESULT Result = Resource->SetPrivateData(WKPDID_D3DDebugObjectName, InDebugName.Size(), *InDebugName);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set resource name");
        }

        // Calling SetName as well since NVIDIA Nsight does not recognize the name otherwise
        FStringWide WideName = CharToWide(InDebugName);
        
        Result = Resource->SetName(*WideName);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set resource name");
        }
    }
}

void FD3D12Resource::GetDebugName(FString& OutDebugName) const
{
    OutDebugName.Clear();

    if (Resource)
    {
        UINT NameLength = 0;

        HRESULT Result = Resource->GetPrivateData(WKPDID_D3DDebugObjectName, &NameLength, nullptr);
        if (Result == DXGI_ERROR_NOT_FOUND)
        {
            return;
        }

        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to get size of resource name");
            return;
        }

        OutDebugName.Resize(NameLength);

        Result = Resource->GetPrivateData(WKPDID_D3DDebugObjectName, &NameLength, OutDebugName.Data());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to get resource name");
            OutDebugName.Clear();
        }
    }
}

FD3D12Resource::~FD3D12Resource()
{
    EndResidencyTracking();
}

void FD3D12Resource::StartResidencyTracking()
{
    if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
    {
        const D3D12_RESOURCE_ALLOCATION_INFO AllocationInfo = GetDevice()->GetD3D12Device()->GetResourceAllocationInfo(0, 1, &Desc);
        ResidencyHandle.Initialize(Resource.Get(), AllocationInfo.SizeInBytes);
        ResidencyManager->BeginTrackingObject(&ResidencyHandle);
    }
}

void FD3D12Resource::EndResidencyTracking()
{
    if (ResidencyHandle.IsInitialized())
    {
        if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
        {
            ResidencyManager->EndTrackingObject(&ResidencyHandle);
        }
    }
}

void FD3D12Resource::DeferredRelease()
{
    FD3D12RHI::DeferDeletion(this);
}

FD3D12ResourceStorage::FD3D12ResourceStorage(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , Owner(nullptr)
    , ResourceOffset(0)
    , GpuVirtualAddress(0)
    , MappedBaseAddress(nullptr)
    , Size(0)
    , AllocatorType(ED3D12AllocatorType::None)
    , StorageType(EResourceStorageType::Unknown)
{
    FMemory::Memzero(&AllocationData, sizeof(AllocationData));
}

FD3D12ResourceStorage::~FD3D12ResourceStorage()
{
    ReleaseResource();
}

void FD3D12ResourceStorage::Swap(FD3D12ResourceStorage& Other)
{
    if (this == &Other)
    {
        return;
    }

    CHECK(GetDevice() == Other.GetDevice());

    ::Swap(Resource, Other.Resource);
    ::Swap(ResourceOffset, Other.ResourceOffset);
    ::Swap(GpuVirtualAddress, Other.GpuVirtualAddress);
    ::Swap(MappedBaseAddress, Other.MappedBaseAddress);
    ::Swap(Size, Other.Size);
    ::Swap(AllocatorType, Other.AllocatorType);
    ::Swap(StorageType, Other.StorageType);
    ::Swap(AllocatorPointers.AsVoid, Other.AllocatorPointers.AsVoid);
    
    FMemory::Memswap(&AllocationData, &Other.AllocationData, sizeof(AllocationData));

    UpdateOwnership();
    Other.UpdateOwnership();
}

void FD3D12ResourceStorage::UpdateOwnership()
{
    if (AllocatorType == ED3D12AllocatorType::PoolAllocator && AllocatorPointers.PoolAllocator)
    {
        AllocatorPointers.PoolAllocator->TransferOwnership(AllocationData.Pool, this);
    }
}

void FD3D12ResourceStorage::Reset()
{
    FMemory::Memzero(&AllocationData, sizeof(AllocationData));
    ResetAllocator();

    if (Resource)
    {
        Resource->Release();
        Resource = nullptr;
    }

    ResourceOffset    = 0;
    GpuVirtualAddress = 0;
    MappedBaseAddress = nullptr;
    Size              = 0;
    StorageType       = EResourceStorageType::Unknown;
}

void FD3D12ResourceStorage::ReleaseResource()
{
    if (StorageType == EResourceStorageType::Unknown)
    {
        Reset();
        return;
    }

    if (StorageType == EResourceStorageType::Standalone)
    {
        if (Resource && Resource->ShouldDeferredRelease())
        {
            Resource->DeferredRelease();
        }

        Reset();
        return;
    }

    if (AllocatorPointers.AsVoid != nullptr)
    {
        switch (AllocatorType)
        {
        case ED3D12AllocatorType::BuddyAllocator:
            AllocatorPointers.BuddyAllocator->Deallocate(*this);
            break;
        case ED3D12AllocatorType::BucketAllocator:
            AllocatorPointers.BucketAllocator->Deallocate(*this);
            break;
        case ED3D12AllocatorType::PoolAllocator:
            AllocatorPointers.PoolAllocator->Deallocate(*this);
            break;
        default:
            break;
        }
    }

    // For SuballocatedHeap, we own the placed resource
    if (StorageType == EResourceStorageType::SuballocatedHeap && Resource && Resource->ShouldDeferredRelease())
    {
        Resource->DeferredRelease();
    }

    Reset();
}

void FD3D12ResourceStorage::SetResource(FD3D12Resource* InResource)
{
    if (InResource)
    {
        InResource->AddRef();
    }

    if (Resource)
    {
        Resource->Release();
    }

    Resource = InResource;
}

void FD3D12ResourceStorage::InitStandalone(FD3D12Resource* InResource)
{
    Reset();

    if (InResource)
    {
        InResource->AddRef();
    }

    Resource          = InResource;
    ResourceOffset    = 0;
    GpuVirtualAddress = InResource ? InResource->GetGPUVirtualAddress() : 0;
    MappedBaseAddress = nullptr;
    StorageType       = EResourceStorageType::Standalone;
}

void FD3D12ResourceStorage::ResetAllocator()
{
    AllocatorPointers.AsVoid = nullptr;
    AllocatorType            = ED3D12AllocatorType::None;
}

FD3D12GenericResource::FD3D12GenericResource(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , ResourceStorage(InDevice)
{
    ResourceStorage.SetOwner(this);
}

FD3D12GenericResource::~FD3D12GenericResource()
{
    GetDevice()->CancelPendingDefragMoves(this);

    ResourceRelocated(nullptr);

    ResourceStorage.ReleaseResource();
}

void FD3D12GenericResource::AddResourceRelocatedListener(ID3D12ResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.AddUnique(Listener);
}

void FD3D12GenericResource::RemoveResourceRelocatedListener(ID3D12ResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.Remove(Listener);
}

void FD3D12GenericResource::ResourceRelocated(FD3D12ResourceStorage* NewResourceStorage)
{
    TScopedLock Lock(ListenersCS);

    for (ID3D12ResourceRelocationListener* Listener : Listeners)
    {
        if (Listener)
        {
            Listener->OnResourceRelocated(this, NewResourceStorage);
        }
    }

    if (!NewResourceStorage)
    {
        Listeners.Clear();
    }
}
