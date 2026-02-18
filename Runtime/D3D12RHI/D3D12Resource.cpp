#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

FD3D12Resource::FD3D12Resource(FD3D12Device* InDevice, ID3D12Resource* InResource, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Resource(InResource)
    , HeapType(InHeapType)
    , ResourceState(InInitialState)
    , Desc(InResource ? InResource->GetDesc() : D3D12_RESOURCE_DESC{})
    , Address(0)
    , NumSubresources(0)
    , bShouldDeferredRelease(true)
{
    if (Resource)
    {
        if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
        {
            Address = Resource->GetGPUVirtualAddress();
        }

        const uint32 ArraySize = Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? Desc.DepthOrArraySize : 1u;
        NumSubresources = D3D12CalculateSubresourceCount(Desc.MipLevels, ArraySize, 1);
    }
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

void FD3D12Resource::SetDebugName(const FString& Name)
{
    if (Resource)
    {
        HRESULT Result = Resource->SetPrivateData(WKPDID_D3DDebugObjectName, Name.Size(), *Name);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set resource name");
        }

        // Calling SetName as well since NVIDIA Nsight does not recognize the name otherwise
        FStringWide WideName = CharToWide(Name);
        Result = Resource->SetName(*WideName);
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to set resource name");
        }
    }
}

FString FD3D12Resource::GetDebugName() const
{
    if (Resource)
    {
        UINT NameLength = 0;
        HRESULT Result = Resource->GetPrivateData(WKPDID_D3DDebugObjectName, &NameLength, nullptr);
        if (Result == DXGI_ERROR_NOT_FOUND)
        {
            // We have not called SetPrivateData on this resource, so just return an empty string
            return "";
        }

        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to get size of resource name");
            return "";
        }

        FString NewName;
        NewName.Resize(NameLength);

        Result = Resource->GetPrivateData(WKPDID_D3DDebugObjectName, &NameLength, NewName.Data());
        if (FAILED(Result))
        {
            D3D12_ERROR("Failed to get resource name");
            return "";
        }

        return NewName;
    }

    return "";
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
        ResidencyHandle = ResidencyManager->RegisterPageable(Resource.Get(), AllocationInfo.SizeInBytes, false);
        ResidencyManager->TouchPageable(ResidencyHandle);
    }
}

void FD3D12Resource::EndResidencyTracking()
{
    if (ResidencyHandle.IsValid())
    {
        if (FD3D12ResidencyManager* ResidencyManager = GetDevice()->GetResidencyManager())
        {
            ResidencyManager->UnregisterPageable(ResidencyHandle);
        }

        ResidencyHandle = {};
    }
}

void FD3D12Resource::DeferredRelease()
{
    FD3D12RHI::DeferDeletion(this);
}

FD3D12ResourceStorage::FD3D12ResourceStorage(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , ResourceOffset(0)
    , GpuVirtualAddress(0)
    , MappedBaseAddress(nullptr)
    , Size(0)
    , ResidencyHandle()
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

    FD3D12Resource* TempResource = Resource;
    Resource       = Other.Resource;
    Other.Resource = TempResource;

    const uint64 TempResourceOffset = ResourceOffset;
    ResourceOffset       = Other.ResourceOffset;
    Other.ResourceOffset = TempResourceOffset;

    const D3D12_GPU_VIRTUAL_ADDRESS TempGpuVirtualAddress = GpuVirtualAddress;
    GpuVirtualAddress       = Other.GpuVirtualAddress;
    Other.GpuVirtualAddress = TempGpuVirtualAddress;

    void* const TempMappedBaseAddress = MappedBaseAddress;
    MappedBaseAddress       = Other.MappedBaseAddress;
    Other.MappedBaseAddress = TempMappedBaseAddress;

    const uint64 TempSize = Size;
    Size       = Other.Size;
    Other.Size = TempSize;

    const FD3D12ResidencyHandle TempResidencyHandle = ResidencyHandle;
    ResidencyHandle       = Other.ResidencyHandle;
    Other.ResidencyHandle = TempResidencyHandle;

    const ED3D12AllocatorType TempAllocatorType = AllocatorType;
    AllocatorType       = Other.AllocatorType;
    Other.AllocatorType = TempAllocatorType;

    const EResourceStorageType TempStorageType = StorageType;
    StorageType       = Other.StorageType;
    Other.StorageType = TempStorageType;

    uint8 TempAllocationData[sizeof(AllocationData)];
    FMemory::Memcpy(TempAllocationData, &AllocationData, sizeof(AllocationData));
    FMemory::Memcpy(&AllocationData, &Other.AllocationData, sizeof(AllocationData));
    FMemory::Memcpy(&Other.AllocationData, TempAllocationData, sizeof(AllocationData));

    void* const TempAllocatorPointer = AllocatorPointers.AsVoid;
    AllocatorPointers.AsVoid       = Other.AllocatorPointers.AsVoid;
    Other.AllocatorPointers.AsVoid = TempAllocatorPointer;
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
    ResidencyHandle   = {};
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

FD3D12BaseResource::FD3D12BaseResource(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , ResourceStorage(InDevice)
{
}

FD3D12BaseResource::~FD3D12BaseResource()
{
    ResourceStorage.ReleaseResource();
}

void FD3D12BaseResource::AddListener(ID3D12ResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.AddUnique(Listener);
}

void FD3D12BaseResource::RemoveListener(ID3D12ResourceRelocationListener* Listener)
{
    if (!Listener)
    {
        return;
    }

    TScopedLock Lock(ListenersCS);
    Listeners.Remove(Listener);
}

void FD3D12BaseResource::NotifyRelocation()
{
    // Notify all listeners that the resource was reallocated (underlying resource/allocation changed)
    TScopedLock Lock(ListenersCS);

    for (ID3D12ResourceRelocationListener* Listener : Listeners)
    {
        if (Listener)
        {
            Listener->OnRelocation(this);
        }
    }
}
