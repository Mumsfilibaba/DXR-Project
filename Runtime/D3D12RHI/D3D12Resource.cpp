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
    if (bDeferDeletion)
    {
        FD3D12RHI::DeferDeletion(this);
        return;
    }
    
    // Immediate deletion
    Resource.Reset();
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
{
    FMemory::Memzero(&AllocationData, sizeof(AllocationData));
    ClearAllocator();
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

    FD3D12ResourceRef TempResource = Resource;
    Resource = Other.Resource;
    Other.Resource = TempResource;

    const uint64 TempResourceOffset = ResourceOffset;
    ResourceOffset = Other.ResourceOffset;
    Other.ResourceOffset = TempResourceOffset;

    const D3D12_GPU_VIRTUAL_ADDRESS TempGpuVirtualAddress = GpuVirtualAddress;
    GpuVirtualAddress = Other.GpuVirtualAddress;
    Other.GpuVirtualAddress = TempGpuVirtualAddress;

    void* const TempMappedBaseAddress = MappedBaseAddress;
    MappedBaseAddress = Other.MappedBaseAddress;
    Other.MappedBaseAddress = TempMappedBaseAddress;

    const uint64 TempSize = Size;
    Size = Other.Size;
    Other.Size = TempSize;

    const FD3D12ResidencyHandle TempResidencyHandle = ResidencyHandle;
    ResidencyHandle = Other.ResidencyHandle;
    Other.ResidencyHandle = TempResidencyHandle;

    const ED3D12AllocatorType TempAllocatorType = AllocatorType;
    AllocatorType = Other.AllocatorType;
    Other.AllocatorType = TempAllocatorType;

    uint8 TempAllocationData[sizeof(AllocationData)];
    FMemory::Memcpy(TempAllocationData, &AllocationData, sizeof(AllocationData));
    FMemory::Memcpy(&AllocationData, &Other.AllocationData, sizeof(AllocationData));
    FMemory::Memcpy(&Other.AllocationData, TempAllocationData, sizeof(AllocationData));

    void* const TempAllocatorPointer = AllocatorPointers.AsVoid;
    AllocatorPointers.AsVoid = Other.AllocatorPointers.AsVoid;
    Other.AllocatorPointers.AsVoid = TempAllocatorPointer;
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
    ClearAllocator();
}

void FD3D12ResourceStorage::ReleaseResource()
{
    if (AllocatorPointers.AsVoid == nullptr)
    {
        return;
    }

    switch (AllocatorType)
    {
    case ED3D12AllocatorType::LinearAllocator:
        AllocatorPointers.LinearAllocator->Deallocate(*this);
        break;
    case ED3D12AllocatorType::DynamicConstantsAllocator:
        AllocatorPointers.DynamicConstantsAllocator->Deallocate(*this);
        break;
    case ED3D12AllocatorType::UploadHeapAllocator:
        AllocatorPointers.UploadHeapAllocator->Deallocate(*this);
        break;
    case ED3D12AllocatorType::BufferAllocatorPool:
        AllocatorPointers.BufferAllocatorPool->Deallocate(*this);
        break;
    case ED3D12AllocatorType::BufferAllocator:
        AllocatorPointers.BufferAllocator->Deallocate(*this);
        break;
    case ED3D12AllocatorType::TextureAllocator:
        AllocatorPointers.TextureAllocator->Deallocate(*this);
        break;
    case ED3D12AllocatorType::BuddyAllocator:
        AllocatorPointers.BuddyAllocator->Deallocate(*this);
        break;
    case ED3D12AllocatorType::MultiBuddyAllocator:
        AllocatorPointers.MultiBuddyAllocator->Deallocate(*this);
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

    Reset();
}

void FD3D12ResourceStorage::InitStandalone(const FD3D12ResourceRef& InResource)
{
    Reset();
    Resource = InResource;
    ResourceOffset = 0;
    GpuVirtualAddress = InResource ? InResource->GetGPUVirtualAddress() : 0;
    MappedBaseAddress = nullptr;
}

void FD3D12ResourceStorage::ClearAllocator()
{
    AllocatorPointers.AsVoid = nullptr;
    AllocatorType = ED3D12AllocatorType::None;
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
