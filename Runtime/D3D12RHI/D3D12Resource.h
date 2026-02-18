#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12RefCounted.h"
#include "D3D12RHI/D3D12Heap.h"

typedef TSharedRef<class FD3D12Resource> FD3D12ResourceRef;

class FD3D12BuddyAllocator;
class FD3D12BucketAllocator;
class FD3D12PoolAllocator;
class FD3D12BaseResource;

enum class ED3D12ResourceLifetime : uint8
{
    Default,
    Transient
};

enum class ED3D12AllocatorType : uint8
{
    None,
    BuddyAllocator,
    BucketAllocator,
    PoolAllocator
};

enum class EResourceStorageType : uint8
{
    Unknown,
    Standalone,
    SuballocatedHeap,
    SuballocatedResource
};

struct FD3D12PoolAllocatorAllocationData
{
    uint32 PageIndex = UINT32_MAX;
    uint64 Offset    = 0;
    uint64 Size      = 0;
};

struct FD3D12BuddyAllocatorAllocationData
{
    uint32 Order            = 0;
    uint64 Offset           = 0;
    bool   bBackedByHeap    = false;
    FD3D12Heap* BackingHeap = nullptr;
};

struct FD3D12BucketAllocatorAllocationData
{
    uint32 BucketIndex = UINT32_MAX;
    uint32 SlotIndex   = UINT32_MAX;
    uint64 Offset      = 0;
    uint64 Size        = 0;
};

class FD3D12ResourceStorage : public FD3D12DeviceChild, public FNonCopyable
{
public:
    FD3D12ResourceStorage(FD3D12Device* InDevice);
    ~FD3D12ResourceStorage();

    void InitStandalone(FD3D12Resource* InResource);
    
    void Swap(FD3D12ResourceStorage& Other);
    void ReleaseResource();
    void Reset();
    void ResetAllocator();
    void SetResource(FD3D12Resource* InResource);

    FORCEINLINE bool IsValid()          const { return StorageType != EResourceStorageType::Unknown; }
    FORCEINLINE bool IsPlacedResource() const { return StorageType == EResourceStorageType::SuballocatedHeap; }

    FORCEINLINE void*                        GetMappedBaseAddress() const { return MappedBaseAddress; }
    FORCEINLINE uint64                       GetSize()              const { return Size; }
    FORCEINLINE uint64                       GetResourceOffset()    const { return ResourceOffset; }
    FORCEINLINE uint64                       GetGpuVirtualAddress() const { return GpuVirtualAddress; }
    FORCEINLINE const FD3D12ResidencyHandle& GetResidencyHandle()   const { return ResidencyHandle; }
    FORCEINLINE void*                        GetAllocator()         const { return AllocatorPointers.AsVoid; }
    FORCEINLINE ED3D12AllocatorType          GetAllocatorType()     const { return AllocatorType; }
    FORCEINLINE EResourceStorageType         GetStorageType()       const { return StorageType; }
    FORCEINLINE FD3D12Resource*              GetResource()          const { return Resource; }

    FORCEINLINE const FD3D12PoolAllocatorAllocationData&   GetPoolAllocationData()   const { return AllocationData.Pool; }
    FORCEINLINE const FD3D12BuddyAllocatorAllocationData&  GetBuddyAllocationData()  const { return AllocationData.Buddy; }
    FORCEINLINE const FD3D12BucketAllocatorAllocationData& GetBucketAllocationData() const { return AllocationData.Bucket; }

    FORCEINLINE void SetSize(uint64 InSize)                                              { Size = InSize; }
    FORCEINLINE void SetResourceOffset(uint64 InResourceOffset)                          { ResourceOffset = InResourceOffset; }
    FORCEINLINE void SetGpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS InGpuVirtualAddress) { GpuVirtualAddress = InGpuVirtualAddress; }
    FORCEINLINE void SetMappedBaseAddress(void* InMappedBaseAddress)                     { MappedBaseAddress = InMappedBaseAddress; }
    FORCEINLINE void SetStorageType(EResourceStorageType InStorageType)                  { StorageType = InStorageType; }

    FORCEINLINE void SetResidencyHandle(const FD3D12ResidencyHandle& InResidencyHandle)
    {
        ResidencyHandle = InResidencyHandle;
    }

    FORCEINLINE void SetBuddyAllocator(FD3D12BuddyAllocator* InAllocator)
    {
        AllocatorPointers.BuddyAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::BuddyAllocator;
    }

    FORCEINLINE void SetBucketAllocator(FD3D12BucketAllocator* InAllocator)
    {
        AllocatorPointers.BucketAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::BucketAllocator;
    }

    FORCEINLINE void SetPoolAllocator(FD3D12PoolAllocator* InAllocator)
    {
        AllocatorPointers.PoolAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::PoolAllocator;
    }

    FORCEINLINE void SetPoolAllocationData(const FD3D12PoolAllocatorAllocationData& InData)     { AllocationData.Pool = InData; }
    FORCEINLINE void SetBuddyAllocationData(const FD3D12BuddyAllocatorAllocationData& InData)   { AllocationData.Buddy = InData; }
    FORCEINLINE void SetBucketAllocationData(const FD3D12BucketAllocatorAllocationData& InData) { AllocationData.Bucket = InData; }

private:

    union FAllocatorData
    {
        FD3D12PoolAllocatorAllocationData   Pool;
        FD3D12BuddyAllocatorAllocationData  Buddy;
        FD3D12BucketAllocatorAllocationData Bucket;

        FAllocatorData() {}
    } AllocationData;

    union FAllocatorPointers
    {
        FD3D12BuddyAllocator*  BuddyAllocator;
        FD3D12BucketAllocator* BucketAllocator;
        FD3D12PoolAllocator*   PoolAllocator;
        void*                  AsVoid;

        FAllocatorPointers()
            : AsVoid(nullptr)
        {
        }

    } AllocatorPointers;

    FD3D12Resource*           Resource;
    uint64                    ResourceOffset;
    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
    void*                     MappedBaseAddress;
    uint64                    Size;
    FD3D12ResidencyHandle     ResidencyHandle;
    ED3D12AllocatorType       AllocatorType;
    EResourceStorageType      StorageType;
};

class FD3D12Resource : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Resource(FD3D12Device* InDevice, ID3D12Resource* InResource, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState);
    ~FD3D12Resource();

    void SetResourceState(D3D12_RESOURCE_STATES InState) { ResourceState = InState; }
    void* MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    void UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);

    void SetDebugName(const FString& Name);
    FString GetDebugName() const;

    void DeferredRelease();

    void StartResidencyTracking();
    void EndResidencyTracking();

    void DisableDeferredRelease() { bShouldDeferredRelease = false; }
    bool ShouldDeferredRelease() const { return bShouldDeferredRelease; }

    // Texture Accessors
    uint64 GetWidth()  const { return Desc.Width; }
    uint64 GetHeight() const { return Desc.Height; }
    uint64 GetDepth()  const { return Desc.DepthOrArraySize; }

    // Buffer Accessors
    uint64                    GetSize()              const { return Desc.Width; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return Address; }
    
    // Resource Accessors
    D3D12_HEAP_TYPE          GetHeapType()  const { return HeapType; }
    D3D12_RESOURCE_STATES    GetState()     const { return ResourceState; }
    D3D12_RESOURCE_DIMENSION GetDimension() const { return Desc.Dimension; }

    uint32 GetNumSubresources() const 
    {
        return NumSubresources;
    }

    ID3D12Resource* GetD3D12Resource() const 
    { 
        return Resource.Get(); 
    }

    const D3D12_RESOURCE_DESC& GetDesc() const
    {
        return Desc;
    }

    const FD3D12ResidencyHandle& GetResidencyHandle() const
    {
        return ResidencyHandle;
    }

private:
    TComPtr<ID3D12Resource>   Resource;
    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address;
    uint32                    NumSubresources;
    FD3D12ResidencyHandle     ResidencyHandle;
    bool                      bShouldDeferredRelease;
};

struct ID3D12ResourceRelocationListener
{
    virtual ~ID3D12ResourceRelocationListener() = default;
    virtual void OnRelocation(FD3D12BaseResource* Resource) = 0;
};

class FD3D12BaseResource : public FD3D12DeviceChild
{
public:
    FD3D12BaseResource(FD3D12Device* InDevice);
    virtual ~FD3D12BaseResource();

    void AddListener(ID3D12ResourceRelocationListener* Listener);
    void RemoveListener(ID3D12ResourceRelocationListener* Listener);

    void NotifyRelocation();

    FD3D12Resource* GetResource() 
    {
        return ResourceStorage.GetResource();
    }

    const FD3D12Resource* GetResource() const
    {
        return ResourceStorage.GetResource();
    }

protected:
    FD3D12ResourceStorage ResourceStorage;

private:
    TArray<ID3D12ResourceRelocationListener*> Listeners;
    FCriticalSection                          ListenersCS;
};
