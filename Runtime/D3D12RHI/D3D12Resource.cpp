#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"

FD3D12Resource::FD3D12Resource(FD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Resource(InNativeResource)
    , HeapType(D3D12_HEAP_TYPE_DEFAULT)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InNativeResource ? InNativeResource->GetDesc() : D3D12_RESOURCE_DESC{})
    , Address(0)
    , NumSubresources(0)
{
}

FD3D12Resource::FD3D12Resource(FD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType)
    : FD3D12RefCounted()
    , FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
    , HeapType(InHeapType)
    , ResourceState(D3D12_RESOURCE_STATE_COMMON)
    , Desc(InDesc)
    , Address(0)
    , NumSubresources(0)
{
}

void FD3D12Resource::SetResource(const TComPtr<ID3D12Resource>& InNativeResource)
{
    Resource = InNativeResource;
    Desc = Resource ? Resource->GetDesc() : D3D12_RESOURCE_DESC{};
    Address = 0;
    NumSubresources = 0;
}

void FD3D12Resource::InitializeFromNative(D3D12_RESOURCE_STATES InitialState)
{
    if (!Resource)
    {
        return;
    }

    if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        Address = Resource->GetGPUVirtualAddress();
    }
    else
    {
        Address = 0;
    }

    ResourceState = InitialState;

    const uint32 ArraySize = Desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? Desc.DepthOrArraySize : 1u;
    NumSubresources = D3D12CalculateSubresourceCount(Desc.MipLevels, ArraySize, 1);
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
    ReleaseResource();
}

void FD3D12Resource::ReleaseResource()
{
    if (bDeferDeletion && FD3D12RHI::Get())
    {
        FD3D12RHI::Get()->DeferDeletion(this);
        return;
    }
    
    // Immediate deletion
    Resource.Reset();
}

FD3D12ResourceStorage::FD3D12ResourceStorage()
    : Resource(nullptr)
    , ResourceOffset(0)
    , GpuVirtualAddress(0)
    , MappedBaseAddress(nullptr)
    , Size(0)
    , ResidencyHandle()
    , OwnerType(ED3D12ResourceStorageOwnerType::None)
{
    FMemory::Memzero(&AllocationData, sizeof(AllocationData));
    ClearOwner();
}

FD3D12ResourceStorage::FD3D12ResourceStorage(FD3D12ResourceStorage&& Other) noexcept
    : Resource(nullptr)
    , ResourceOffset(0)
    , GpuVirtualAddress(0)
    , MappedBaseAddress(nullptr)
    , Size(0)
    , ResidencyHandle()
    , OwnerType(ED3D12ResourceStorageOwnerType::None)
{
    FMemory::Memzero(&AllocationData, sizeof(AllocationData));
    ClearOwner();
    MoveFrom(Move(Other));
}

FD3D12ResourceStorage& FD3D12ResourceStorage::operator=(FD3D12ResourceStorage&& Other) noexcept
{
    if (this != &Other)
    {
        ReleaseResource();
        MoveFrom(Move(Other));
    }
    return *this;
}

FD3D12ResourceStorage::~FD3D12ResourceStorage()
{
    ReleaseResource();
}

void FD3D12ResourceStorage::CopyFrom(const FD3D12ResourceStorage& Other)
{
    if (this == &Other)
    {
        return;
    }

    ReleaseResource();

    Resource = Other.Resource;
    ResourceOffset = Other.ResourceOffset;
    GpuVirtualAddress = Other.GpuVirtualAddress;
    MappedBaseAddress = Other.MappedBaseAddress;
    Size = Other.Size;
    ResidencyHandle = Other.ResidencyHandle;
    OwnerType = Other.OwnerType;
    OwnerPointers.AsVoid = Other.OwnerPointers.AsVoid;
    FMemory::Memcpy(&AllocationData, &Other.AllocationData, sizeof(AllocationData));
}

void FD3D12ResourceStorage::Reset()
{
    Resource = nullptr;
    ResourceOffset = 0;
    GpuVirtualAddress = 0;
    MappedBaseAddress = nullptr;
    Size = 0;
    ResidencyHandle = {};
    FMemory::Memzero(&AllocationData, sizeof(AllocationData));
    ClearOwner();
}

void FD3D12ResourceStorage::ReleaseResource()
{
    if (OwnerPointers.AsVoid == nullptr)
    {
        return;
    }

    switch (OwnerType)
    {
    case ED3D12ResourceStorageOwnerType::LinearAllocator:
        OwnerPointers.LinearAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::DynamicConstantsAllocator:
        OwnerPointers.DynamicConstantsAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::UploadHeapAllocator:
        OwnerPointers.UploadHeapAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::BufferAllocatorPool:
        OwnerPointers.BufferAllocatorPool->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::BufferAllocator:
        OwnerPointers.BufferAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::TextureAllocator:
        OwnerPointers.TextureAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::BuddyAllocator:
        OwnerPointers.BuddyAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::MultiBuddyAllocator:
        OwnerPointers.MultiBuddyAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::BucketAllocator:
        OwnerPointers.BucketAllocator->Deallocate(*this);
        break;
    case ED3D12ResourceStorageOwnerType::PoolAllocator:
        OwnerPointers.PoolAllocator->Deallocate(*this);
        break;
    default:
        break;
    }

    Reset();
}

void FD3D12ResourceStorage::InitializeAsStandalone(
    const FD3D12ResourceRef& InResource,
    ED3D12ResourceKind InResourceKind,
    D3D12_HEAP_TYPE InHeapType,
    ED3D12HeapUsage InHeapUsage,
    ED3D12ResourceLifetime InLifetime,
    uint64 InSize,
    uint64 InAlignment)
{
    UNREFERENCED_VARIABLE(InResourceKind);
    UNREFERENCED_VARIABLE(InHeapType);
    UNREFERENCED_VARIABLE(InHeapUsage);
    UNREFERENCED_VARIABLE(InLifetime);
    UNREFERENCED_VARIABLE(InAlignment);

    Reset();
    Resource = InResource;
    Size = InSize;
    ResourceOffset = 0;
    GpuVirtualAddress = InResource ? InResource->GetGPUVirtualAddress() : 0;
    MappedBaseAddress = nullptr;
}

void FD3D12ResourceStorage::InitializeAsPlaced(
    const FD3D12ResourceRef& InResource,
    const FD3D12HeapRef& InHeap,
    uint64 InHeapOffset,
    ED3D12ResourceKind InResourceKind,
    D3D12_HEAP_TYPE InHeapType,
    ED3D12HeapUsage InHeapUsage,
    ED3D12ResourceLifetime InLifetime,
    uint64 InSize,
    uint64 InAlignment)
{
    UNREFERENCED_VARIABLE(InHeapOffset);
    UNREFERENCED_VARIABLE(InResourceKind);
    UNREFERENCED_VARIABLE(InHeapType);
    UNREFERENCED_VARIABLE(InHeapUsage);
    UNREFERENCED_VARIABLE(InLifetime);
    UNREFERENCED_VARIABLE(InAlignment);

    Reset();
    Resource = InResource;
    SetHeap(InHeap);
    Size = InSize;
    ResourceOffset = 0;
    GpuVirtualAddress = InResource ? InResource->GetGPUVirtualAddress() : 0;
    MappedBaseAddress = nullptr;
}

void FD3D12ResourceStorage::InitializeAsSuballocated(
    const FD3D12ResourceRef& InResource,
    uint64 InResourceOffset,
    D3D12_GPU_VIRTUAL_ADDRESS InGpuVirtualAddress,
    void* InMappedPtr,
    ED3D12ResourceKind InResourceKind,
    D3D12_HEAP_TYPE InHeapType,
    ED3D12HeapUsage InHeapUsage,
    ED3D12ResourceLifetime InLifetime,
    uint64 InSize,
    uint64 InAlignment,
    void* InOwner,
    ED3D12ResourceStorageOwnerType InOwnerType,
    uint64 InRetireFenceValue)
{
    UNREFERENCED_VARIABLE(InResourceKind);
    UNREFERENCED_VARIABLE(InHeapType);
    UNREFERENCED_VARIABLE(InHeapUsage);
    UNREFERENCED_VARIABLE(InLifetime);
    UNREFERENCED_VARIABLE(InAlignment);
    UNREFERENCED_VARIABLE(InRetireFenceValue);

    Reset();
    Resource = InResource;
    ResourceOffset = InResourceOffset;
    GpuVirtualAddress = InGpuVirtualAddress;
    MappedBaseAddress = InMappedPtr;
    Size = InSize;
    SetOwner(InOwner, InOwnerType);
}

void FD3D12ResourceStorage::MoveFrom(FD3D12ResourceStorage&& Other) noexcept
{
    Resource = Move(Other.Resource);
    ResourceOffset = Other.ResourceOffset;
    GpuVirtualAddress = Other.GpuVirtualAddress;
    MappedBaseAddress = Other.MappedBaseAddress;
    Size = Other.Size;
    ResidencyHandle = Other.ResidencyHandle;
    OwnerType = Other.OwnerType;
    OwnerPointers.AsVoid = Other.OwnerPointers.AsVoid;
    FMemory::Memcpy(&AllocationData, &Other.AllocationData, sizeof(AllocationData));

    Other.Reset();
}

void FD3D12ResourceStorage::ClearOwner()
{
    OwnerPointers.AsVoid = nullptr;
    OwnerType = ED3D12ResourceStorageOwnerType::None;
}

FD3D12BaseResource::FD3D12BaseResource(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , ResourceStorage()
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
