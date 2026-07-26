#pragma once
#include "Core/RefCountedBase.h"
#include "Core/Containers/Array.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Heap.h"
#include "D3D12RHI/D3D12ResourceState.h"

typedef TSharedRef<class FD3D12Resource> FD3D12ResourceRef;

class FD3D12PoolAllocator;
class FD3D12BucketAllocator;
class FD3D12BuddyAllocator;
class FD3D12ResourceBase;
class FD3D12ResourceStorage;

enum class ED3D12ResourceStateMode : uint8;

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
    uint32                 PageIndex = UINT32_MAX;
    uint64                 Offset    = 0;
    uint64                 Size      = 0;
    FD3D12ResourceStorage* Owner     = nullptr;
};

struct FD3D12BuddyAllocatorAllocationData
{
    uint32 Order         = 0;
    uint64 Offset        = 0;
    uint64 RequestedSize = 0;
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
    void UpdateOwnership();
    void ReleaseResource();
    void Reset();
    void ResetAllocator();
    void SetResource(FD3D12Resource* InResource);

    FORCEINLINE bool IsValid()          const { return StorageType != EResourceStorageType::Unknown; }
    FORCEINLINE bool IsPlacedResource() const { return StorageType == EResourceStorageType::SuballocatedHeap; }

    FORCEINLINE void*                GetMappedBaseAddress() const { return MappedBaseAddress; }
    FORCEINLINE uint64               GetSize()              const { return Size; }
    FORCEINLINE uint64               GetResourceOffset()    const { return ResourceOffset; }
    FORCEINLINE uint64               GetGPUVirtualAddress() const { return GpuVirtualAddress; }
    FORCEINLINE void*                GetAllocator()         const { return AllocatorPointers.AsVoid; }
    FORCEINLINE FD3D12Resource*      GetResource()          const { return Resource; }
    FORCEINLINE FD3D12ResourceBase*  GetOwner()             const { return Owner; }
    FORCEINLINE FD3D12PoolAllocator* GetPoolAllocator()     const { return (AllocatorType == ED3D12AllocatorType::PoolAllocator) ? AllocatorPointers.PoolAllocator : nullptr; }
    FORCEINLINE ED3D12AllocatorType  GetAllocatorType()     const { return AllocatorType; }
    FORCEINLINE EResourceStorageType GetStorageType()       const { return StorageType; }

    FORCEINLINE const FD3D12PoolAllocatorAllocationData&   GetPoolAllocationData()   const { return AllocationData.Pool; }
    FORCEINLINE const FD3D12BuddyAllocatorAllocationData&  GetBuddyAllocationData()  const { return AllocationData.Buddy; }
    FORCEINLINE const FD3D12BucketAllocatorAllocationData& GetBucketAllocationData() const { return AllocationData.Bucket; }

    FORCEINLINE void SetSize(uint64 InSize)                                              { Size = InSize; }
    FORCEINLINE void SetResourceOffset(uint64 InResourceOffset)                          { ResourceOffset = InResourceOffset; }
    FORCEINLINE void SetGpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS InGpuVirtualAddress) { GpuVirtualAddress = InGpuVirtualAddress; }
    FORCEINLINE void SetMappedBaseAddress(void* InMappedBaseAddress)                     { MappedBaseAddress = InMappedBaseAddress; }
    FORCEINLINE void SetStorageType(EResourceStorageType InStorageType)                  { StorageType = InStorageType; }
    FORCEINLINE void SetOwner(FD3D12ResourceBase* InOwner)                               { Owner = InOwner; }

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
    FORCEINLINE void SetBucketAllocationData(const FD3D12BucketAllocatorAllocationData& InData) { AllocationData.Bucket = InData; }
    FORCEINLINE void SetBuddyAllocationData(const FD3D12BuddyAllocatorAllocationData& InData)   { AllocationData.Buddy = InData; }

private:

    union FAllocatorData
    {
        FD3D12PoolAllocatorAllocationData   Pool;
        FD3D12BucketAllocatorAllocationData Bucket;
        FD3D12BuddyAllocatorAllocationData  Buddy;

        FAllocatorData() {}
    } AllocationData;

    union FAllocatorPointers
    {
        FD3D12PoolAllocator*   PoolAllocator;
        FD3D12BucketAllocator* BucketAllocator;
        FD3D12BuddyAllocator*  BuddyAllocator;
        void*                  AsVoid;

        FAllocatorPointers()
            : AsVoid(nullptr)
        {
        }

    } AllocatorPointers;

    FD3D12Resource*           Resource;
    FD3D12ResourceBase*       Owner;
    uint64                    ResourceOffset;
    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
    void*                     MappedBaseAddress;
    uint64                    Size;
    ED3D12AllocatorType       AllocatorType;
    EResourceStorageType      StorageType;
};

class FD3D12Resource : public FD3D12DeviceChild, public FRefCountedBase
{
public:
    FD3D12Resource(FD3D12Device* InDevice, ID3D12Resource* InResource, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InInitialState, const D3D12_CLEAR_VALUE* InClearValue = nullptr, FD3D12Heap* InHeap = nullptr);
    ~FD3D12Resource();

    void* MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    void  UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    
    void DeferredRelease();
    
    void StartResidencyTracking();
    void EndResidencyTracking();
    bool RequiresResourceStateTracking() const;

    void SetDebugName(const String& InDebugName);
    void GetDebugName(String& OutDebugName) const;
    
    bool IsPlacedResource() const { return Heap != nullptr; }

    bool ShouldDeferredRelease() const { return bShouldDeferredRelease; }
    void DisableDeferredRelease()      { bShouldDeferredRelease = false; }

    // Texture Accessors
    uint64 GetWidth()  const { return Desc.Width; }
    uint64 GetHeight() const { return Desc.Height; }
    uint64 GetDepth()  const { return Desc.DepthOrArraySize; }

    // Buffer Accessors
    uint64                    GetSize()              const { return Desc.Width; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return Address; }
    
    // Resource Accessors
    FD3D12ResourceState&       GetResourceState()       { return ResourceState; }
    const FD3D12ResourceState& GetResourceState() const { return ResourceState; }

    D3D12_RESOURCE_DIMENSION GetDimension()  const { return Desc.Dimension; }
    D3D12_HEAP_TYPE          GetHeapType()   const { return HeapType; }
    const D3D12_CLEAR_VALUE& GetClearValue() const { return ClearValue; }
    FD3D12Heap*              GetHeap()       const { return Heap.Get(); }

    FD3D12ResidencyHandle* GetResidencyHandle()
    {
        return Heap ? Heap->GetResidencyHandle() : &ResidencyHandle;
    }

    void SetClearValue(const D3D12_CLEAR_VALUE& InClearValue)
    {
        ClearValue     = InClearValue;
        bHasClearValue = true;
    }

    bool HasClearValue() const
    {
        return bHasClearValue;
    }

    void SetDefaultState(D3D12_RESOURCE_STATES InDefaultState)
    {
        DefaultState     = InDefaultState;
        bHasDefaultState = true;
    }

    bool HasDefaultState() const
    {
        return bHasDefaultState;
    }

    D3D12_RESOURCE_STATES GetDefaultState() const
    {
        return DefaultState;
    }

    void SetResourceStateMode(ED3D12ResourceStateMode InStateMode)
    {
        StateMode = InStateMode;
    }

    ED3D12ResourceStateMode GetResourceStateMode() const
    {
        return StateMode;
    }

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

    uint64 GetAllocationSize() const
    {
        return AllocationSize;
    }

private:
    void InitializeStateTracking(D3D12_RESOURCE_STATES InitialState);

    TComPtr<ID3D12Resource>   Resource;
    FD3D12ResourceState       ResourceState;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_CLEAR_VALUE         ClearValue;
    D3D12_GPU_VIRTUAL_ADDRESS Address;
    D3D12_RESOURCE_STATES     DefaultState;
    ED3D12ResourceStateMode   StateMode;
    FD3D12ResidencyHandle     ResidencyHandle;
    FD3D12HeapRef             Heap;
    uint64                    AllocationSize;
    uint32                    NumSubresources;
    bool                      bShouldDeferredRelease : 1;
    bool                      bHasClearValue : 1;
    bool                      bHasDefaultState : 1;
};

struct ID3D12ResourceRelocationListener
{
    virtual ~ID3D12ResourceRelocationListener() = default;
    virtual void OnResourceRelocated(FD3D12ResourceBase* RelocatedResource, FD3D12ResourceStorage* NewResourceStorage) = 0;
};

class FD3D12ResourceBase : public FD3D12DeviceChild
{
public:
    FD3D12ResourceBase(FD3D12Device* InDevice);
    virtual ~FD3D12ResourceBase();
    
    void AddResourceRelocatedListener(ID3D12ResourceRelocationListener* Listener);
    void RemoveResourceRelocatedListener(ID3D12ResourceRelocationListener* Listener);

    void ResourceRelocated(FD3D12ResourceStorage* NewResourceStorage);

    FD3D12ResourceStorage&       GetResourceStorage()       { return ResourceStorage; }
    const FD3D12ResourceStorage& GetResourceStorage() const { return ResourceStorage; }

    FD3D12Resource* GetResource() const
    {
        return ResourceStorage.GetResource();
    }

protected:
    FD3D12ResourceStorage ResourceStorage;

private:
    TArray<ID3D12ResourceRelocationListener*> Listeners;
    FCriticalSection                          ListenersCS;
};
