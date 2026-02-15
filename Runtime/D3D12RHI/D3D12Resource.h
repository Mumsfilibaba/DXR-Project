#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12RefCounted.h"
#include "D3D12RHI/D3D12Heap.h"

typedef TSharedRef<class FD3D12Resource> FD3D12ResourceRef;

class FD3D12LinearAllocator;
class FD3D12DynamicConstantsAllocator;
class FD3D12UploadHeapAllocator;
class FD3D12BufferAllocatorPool;
class FD3D12BufferAllocator;
class FD3D12TextureAllocator;
class FD3D12BuddyAllocator;
class FD3D12MultiBuddyAllocator;
class FD3D12BucketAllocator;
class FD3D12PoolAllocator;
class FD3D12BaseResource;

enum class ED3D12PlacementKind : uint8
{
    Unknown,
    Committed,
    Placed,
    Reserved
};

enum class ED3D12ResourceKind : uint8
{
    Unknown,
    Buffer,
    Texture
};

enum class ED3D12ViewKind : uint8
{
    Unknown,
    WholeResource,
    Subrange
};

enum class ED3D12HeapUsage : uint8
{
    Any,
    BufferOnly,
    TextureOnly,
    RenderTargetDepthStencil
};

enum class ED3D12ResourceLifetime : uint8
{
    Default,
    Transient
};

enum class ED3D12ResourceType : uint8
{
    Buffer,
    Texture
};


enum class ED3D12AllocatorType : uint8
{
    None,
    LinearAllocator,
    DynamicConstantsAllocator,
    UploadHeapAllocator,
    BufferAllocatorPool,
    BufferAllocator,
    TextureAllocator,
    BuddyAllocator,
    MultiBuddyAllocator,
    BucketAllocator,
    PoolAllocator
};

struct FD3D12LinearAllocatorAllocationData
{
    uint32 PageIndex         = UINT32_MAX;
    uint64 PageOffset        = 0;
    uint64 AllocationSize    = 0;
    bool   bDirectAllocation = false;
};

struct FD3D12PoolAllocatorAllocationData
{
    uint32 PageIndex            = UINT32_MAX;
    uint64 Offset               = 0;
    uint64 Size                 = 0;
    bool   bIsStandalone        = false;
    bool   bBackedByHeap        = false;
    FD3D12Heap* BackingHeap     = nullptr;
};

struct FD3D12BuddyAllocatorAllocationData
{
    uint32 AllocatorIndex = 0;
    uint32 PageIndex      = UINT32_MAX;
    uint32 Order          = 0;
    uint64 Offset         = 0;
    bool   bBackedByHeap  = false;
    FD3D12Heap* BackingHeap = nullptr;
};

struct FD3D12MultiBuddyAllocatorAllocationData
{
    uint32 BuddyAllocatorIndex = UINT32_MAX;
    uint32 PageIndex           = UINT32_MAX;
    uint32 Order               = 0;
    uint64 Offset              = 0;
    bool   bBackedByHeap       = false;
    FD3D12Heap* BackingHeap    = nullptr;
};

struct FD3D12BucketAllocatorAllocationData
{
    uint32 BucketIndex = UINT32_MAX;
    uint32 SlotIndex   = UINT32_MAX;
    uint64 Offset      = 0;
    uint64 Size        = 0;
};

struct FD3D12BufferAllocatorAllocationData
{
    uint32 PoolIndex          = UINT32_MAX;
    bool bCommittedAllocation = false;
};

struct FD3D12TextureAllocatorAllocationData
{
    uint32 PoolIndex = UINT32_MAX;
    uint32 PoolClass = 0;
};

class ID3D12ResourceRelocationListener
{
public:
    virtual ~ID3D12ResourceRelocationListener() = default;
    virtual void OnRelocation(FD3D12BaseResource* Resource) = 0;
};

class FD3D12ResourceStorage : public FD3D12DeviceChild, public FNonCopyable
{
public:
    FD3D12ResourceStorage(FD3D12Device* InDevice);
    ~FD3D12ResourceStorage();

    void InitStandalone(const FD3D12ResourceRef& InResource);
    
    void Swap(FD3D12ResourceStorage& Other);
    void ReleaseResource();
    void Reset();

    FORCEINLINE bool IsValid() const { return Resource != nullptr; }

    FORCEINLINE void*                        GetMappedBaseAddress() const { return MappedBaseAddress; }
    FORCEINLINE uint64                       GetSize()              const { return Size; }
    FORCEINLINE uint64                       GetResourceOffset()    const { return ResourceOffset; }
    FORCEINLINE uint64                       GetGpuVirtualAddress() const { return GpuVirtualAddress; }
    FORCEINLINE const FD3D12ResidencyHandle& GetResidencyHandle()   const { return ResidencyHandle; }
    FORCEINLINE void*                        GetAllocator()         const { return AllocatorPointers.AsVoid; }
    FORCEINLINE ED3D12AllocatorType          GetAllocatorType()     const { return AllocatorType; }
    FORCEINLINE FD3D12Resource*              GetResource()          const { return Resource.Get(); }

    FORCEINLINE const FD3D12LinearAllocatorAllocationData&     GetLinearAllocationData()     const { return AllocationData.Linear; }
    FORCEINLINE const FD3D12PoolAllocatorAllocationData&       GetPoolAllocationData()       const { return AllocationData.Pool; }
    FORCEINLINE const FD3D12BuddyAllocatorAllocationData&      GetBuddyAllocationData()      const { return AllocationData.Buddy; }
    FORCEINLINE const FD3D12MultiBuddyAllocatorAllocationData& GetMultiBuddyAllocationData() const { return AllocationData.MultiBuddy; }
    FORCEINLINE const FD3D12BucketAllocatorAllocationData&     GetBucketAllocationData()     const { return AllocationData.Bucket; }
    FORCEINLINE const FD3D12BufferAllocatorAllocationData&     GetBufferAllocatorData()      const { return AllocationData.BufferAllocator; }
    FORCEINLINE const FD3D12TextureAllocatorAllocationData&    GetTextureAllocatorData()     const { return AllocationData.TextureAllocator; }

    FORCEINLINE void SetResource(const FD3D12ResourceRef& InResource)                    { Resource = InResource; }
    FORCEINLINE void SetSize(uint64 InSize)                                              { Size = InSize; }
    FORCEINLINE void SetResourceOffset(uint64 InResourceOffset)                          { ResourceOffset = InResourceOffset; }
    FORCEINLINE void SetGpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS InGpuVirtualAddress) { GpuVirtualAddress = InGpuVirtualAddress; }
    FORCEINLINE void SetMappedBaseAddress(void* InMappedBaseAddress)                     { MappedBaseAddress = InMappedBaseAddress; }

    FORCEINLINE void SetResidencyHandle(const FD3D12ResidencyHandle& InResidencyHandle)
    {
        ResidencyHandle = InResidencyHandle;
    }

    FORCEINLINE void ResetAllocator()
    {
        AllocatorPointers.AsVoid = nullptr;
        AllocatorType = ED3D12AllocatorType::None;
    }

    FORCEINLINE void SetLinearAllocator(FD3D12LinearAllocator* InAllocator)
    {
        AllocatorPointers.LinearAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::LinearAllocator;
    }

    FORCEINLINE void SetDynamicConstantsAllocator(FD3D12DynamicConstantsAllocator* InAllocator)
    {
        AllocatorPointers.DynamicConstantsAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::DynamicConstantsAllocator;
    }

    FORCEINLINE void SetUploadHeapAllocator(FD3D12UploadHeapAllocator* InAllocator)
    {
        AllocatorPointers.UploadHeapAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::UploadHeapAllocator;
    }

    FORCEINLINE void SetBufferAllocatorPool(FD3D12BufferAllocatorPool* InAllocator)
    {
        AllocatorPointers.BufferAllocatorPool = InAllocator;
        AllocatorType = ED3D12AllocatorType::BufferAllocatorPool;
    }

    FORCEINLINE void SetBufferAllocator(FD3D12BufferAllocator* InAllocator)
    {
        AllocatorPointers.BufferAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::BufferAllocator;
    }

    FORCEINLINE void SetTextureAllocator(FD3D12TextureAllocator* InAllocator)
    {
        AllocatorPointers.TextureAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::TextureAllocator;
    }

    FORCEINLINE void SetBuddyAllocator(FD3D12BuddyAllocator* InAllocator)
    {
        AllocatorPointers.BuddyAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::BuddyAllocator;
    }

    FORCEINLINE void SetMultiBuddyAllocator(FD3D12MultiBuddyAllocator* InAllocator)
    {
        AllocatorPointers.MultiBuddyAllocator = InAllocator;
        AllocatorType = ED3D12AllocatorType::MultiBuddyAllocator;
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

    FORCEINLINE void SetLinearAllocationData(const FD3D12LinearAllocatorAllocationData& InData) { AllocationData.Linear = InData; }
    FORCEINLINE void SetPoolAllocationData(const FD3D12PoolAllocatorAllocationData& InData) { AllocationData.Pool = InData; }
    FORCEINLINE void SetBuddyAllocationData(const FD3D12BuddyAllocatorAllocationData& InData) { AllocationData.Buddy = InData; }
    FORCEINLINE void SetMultiBuddyAllocationData(const FD3D12MultiBuddyAllocatorAllocationData& InData) { AllocationData.MultiBuddy = InData; }
    FORCEINLINE void SetBucketAllocationData(const FD3D12BucketAllocatorAllocationData& InData) { AllocationData.Bucket = InData; }
    FORCEINLINE void SetBufferAllocatorData(const FD3D12BufferAllocatorAllocationData& InData) { AllocationData.BufferAllocator = InData; }
    FORCEINLINE void SetTextureAllocatorData(const FD3D12TextureAllocatorAllocationData& InData) { AllocationData.TextureAllocator = InData; }

private:
    void ClearAllocator();

    union FAllocatorData
    {
        FD3D12LinearAllocatorAllocationData       Linear;
        FD3D12PoolAllocatorAllocationData         Pool;
        FD3D12BuddyAllocatorAllocationData        Buddy;
        FD3D12MultiBuddyAllocatorAllocationData   MultiBuddy;
        FD3D12BucketAllocatorAllocationData       Bucket;
        FD3D12BufferAllocatorAllocationData       BufferAllocator;
        FD3D12TextureAllocatorAllocationData      TextureAllocator;

        FAllocatorData() {}
        ~FAllocatorData() {}
    } AllocationData;

    union FAllocatorPointers
    {
        FD3D12LinearAllocator*           LinearAllocator;
        FD3D12DynamicConstantsAllocator* DynamicConstantsAllocator;
        FD3D12UploadHeapAllocator*       UploadHeapAllocator;
        FD3D12BufferAllocatorPool*       BufferAllocatorPool;
        FD3D12BufferAllocator*           BufferAllocator;
        FD3D12TextureAllocator*          TextureAllocator;
        FD3D12BuddyAllocator*            BuddyAllocator;
        FD3D12MultiBuddyAllocator*       MultiBuddyAllocator;
        FD3D12BucketAllocator*           BucketAllocator;
        FD3D12PoolAllocator*             PoolAllocator;
        void*                            AsVoid;

        FAllocatorPointers()
            : AsVoid(nullptr)
        {
        }
    } AllocatorPointers;

    FD3D12ResourceRef         Resource;
    uint64                    ResourceOffset;
    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress;
    void*                     MappedBaseAddress;
    uint64                    Size;
    FD3D12ResidencyHandle     ResidencyHandle;
    ED3D12AllocatorType       AllocatorType;
};

class FD3D12Resource : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Resource(FD3D12Device* InDevice, const TComPtr<ID3D12Resource>& InNativeResource);
    FD3D12Resource(FD3D12Device* InDevice, const D3D12_RESOURCE_DESC& InDesc, D3D12_HEAP_TYPE InHeapType);
    ~FD3D12Resource();

    void SetResource(const TComPtr<ID3D12Resource>& InNativeResource);
    void InitializeFromNative(D3D12_RESOURCE_STATES InitialState);
    void SetResourceState(D3D12_RESOURCE_STATES InState) { ResourceState = InState; }
    void* MapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);
    void UnmapRange(uint32 SubresourceIndex, const D3D12_RANGE* Range);

    void SetDebugName(const FString& Name);
    FString GetDebugName() const;

    void DisableDeferDeletion() { bDeferDeletion = false; }

    // Texture accessors
    uint64 GetWidth()  const { return Desc.Width; }
    uint64 GetHeight() const { return Desc.Height; }
    uint64 GetDepth()  const { return Desc.DepthOrArraySize; }

    // Buffer accessors
    uint64 GetSize() const { return Desc.Width; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return Address; }
    
    D3D12_HEAP_TYPE GetHeapType() const { return HeapType; }
    D3D12_RESOURCE_STATES GetState() const { return ResourceState; }
    D3D12_RESOURCE_DIMENSION GetDimension() const { return Desc.Dimension; }

    uint32 GetNumSubresources() const { return NumSubresources; }

    ID3D12Resource* GetD3D12Resource() const 
    { 
        return Resource.Get(); 
    }

    const D3D12_RESOURCE_DESC& GetDesc() const
    {
        return Desc;
    }

private:
    void ReleaseResource();

    TComPtr<ID3D12Resource>   Resource;
    D3D12_RESOURCE_STATES     ResourceState;
    D3D12_HEAP_TYPE           HeapType;
    D3D12_RESOURCE_DESC       Desc;
    D3D12_GPU_VIRTUAL_ADDRESS Address;
    uint32                    NumSubresources;
    bool                      bDeferDeletion = true;
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