#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/ScopedLock.h"
#include "VulkanRHI/VulkanDeviceChild.h"
static constexpr uint64 VULKAN_MIN_BUDDY_ALLOCATOR_BLOCK_SIZE = 16ull;

class FVulkanBuddyAllocator;
class FVulkanPoolAllocator;
class FVulkanGenericResource;
class FVulkanMemoryStorage;

enum class EVulkanAllocatorType : uint8
{
    None,
    BuddyAllocator,
    PoolAllocator
};

enum class EVulkanMemoryStorageType : uint8
{
    Unknown,
    Dedicated,
    Suballocated
};

struct FVulkanBuddyAllocatorAllocationData
{
    uint32 Order  = 0;
    uint64 Offset = 0;
};

struct FVulkanPoolAllocatorAllocationData
{
    uint32                PageIndex = UINT32_MAX;
    uint64                Offset    = 0;
    uint64                Size      = 0;
    FVulkanMemoryStorage* Owner     = nullptr;
};

class FVulkanMemoryStorage : public FVulkanDeviceChild, public FNonCopyable
{
public:
    FVulkanMemoryStorage(FVulkanDevice* InDevice);
    ~FVulkanMemoryStorage();

    void Swap(FVulkanMemoryStorage& Other);
    void ReleaseMemory();
    void Reset();

    bool IsValid()        const { return StorageType != EVulkanMemoryStorageType::Unknown; }
    bool IsSuballocated() const { return StorageType == EVulkanMemoryStorageType::Suballocated; }

    FORCEINLINE VkDeviceMemory           GetMemory()            const { return Memory; }
    FORCEINLINE VkDeviceSize             GetMemoryOffset()      const { return MemoryOffset; }
    FORCEINLINE VkBuffer                 GetBackingBuffer()     const { return BackingBuffer; }
    FORCEINLINE VkDeviceSize             GetBufferOffset()      const { return BufferOffset; }
    FORCEINLINE VkDeviceAddress          GetDeviceAddress()     const { return DeviceAddress; }
    FORCEINLINE void*                    GetMappedBaseAddress() const { return MappedBaseAddress; }
    FORCEINLINE VkDeviceSize             GetSize()              const { return Size; }
    FORCEINLINE EVulkanAllocatorType     GetAllocatorType()     const { return AllocatorType; }
    FORCEINLINE EVulkanMemoryStorageType GetStorageType()       const { return StorageType; }
    FORCEINLINE FVulkanGenericResource*  GetOwner()             const { return Owner; }

    FORCEINLINE FVulkanBuddyAllocator* GetBuddyAllocator() const
    {
        return (AllocatorType == EVulkanAllocatorType::BuddyAllocator) ? AllocatorPointers.BuddyAllocator : nullptr;
    }

    FORCEINLINE FVulkanPoolAllocator* GetPoolAllocator() const
    {
        return (AllocatorType == EVulkanAllocatorType::PoolAllocator) ? AllocatorPointers.PoolAllocator : nullptr;
    }

    FORCEINLINE const FVulkanPoolAllocatorAllocationData&  GetPoolAllocationData()  const { return AllocationData.Pool; }
    FORCEINLINE const FVulkanBuddyAllocatorAllocationData& GetBuddyAllocationData() const { return AllocationData.Buddy; }

    FORCEINLINE void SetMemory(VkDeviceMemory InMemory)              { Memory = InMemory; }
    FORCEINLINE void SetMemoryOffset(VkDeviceSize InOffset)          { MemoryOffset = InOffset; }
    FORCEINLINE void SetBackingBuffer(VkBuffer InBuffer)             { BackingBuffer = InBuffer; }
    FORCEINLINE void SetBufferOffset(VkDeviceSize InOffset)          { BufferOffset = InOffset; }
    FORCEINLINE void SetDeviceAddress(VkDeviceAddress InAddress)     { DeviceAddress = InAddress; }
    FORCEINLINE void SetMappedBaseAddress(void* InAddress)           { MappedBaseAddress = InAddress; }
    FORCEINLINE void SetSize(VkDeviceSize InSize)                    { Size = InSize; }
    FORCEINLINE void SetStorageType(EVulkanMemoryStorageType InType) { StorageType = InType; }
    FORCEINLINE void SetOwner(FVulkanGenericResource* InOwner)       { Owner = InOwner; }

    FORCEINLINE void SetBuddyAllocator(FVulkanBuddyAllocator* InAllocator)
    {
        AllocatorPointers.BuddyAllocator = InAllocator;
        AllocatorType = EVulkanAllocatorType::BuddyAllocator;
    }

    FORCEINLINE void SetPoolAllocator(FVulkanPoolAllocator* InAllocator)
    {
        AllocatorPointers.PoolAllocator = InAllocator;
        AllocatorType = EVulkanAllocatorType::PoolAllocator;
    }

    FORCEINLINE void SetPoolAllocationData(const FVulkanPoolAllocatorAllocationData& InData)   { AllocationData.Pool = InData; }
    FORCEINLINE void SetBuddyAllocationData(const FVulkanBuddyAllocatorAllocationData& InData) { AllocationData.Buddy = InData; }

private:
    VkDeviceMemory           Memory;
    VkDeviceSize             MemoryOffset;
    VkBuffer                 BackingBuffer;
    VkDeviceSize             BufferOffset;
    VkDeviceAddress          DeviceAddress;
    void*                    MappedBaseAddress;
    VkDeviceSize             Size;
    EVulkanAllocatorType     AllocatorType;
    EVulkanMemoryStorageType StorageType;
    FVulkanGenericResource*  Owner;

    union FAllocationData
    {
        FVulkanPoolAllocatorAllocationData  Pool;
        FVulkanBuddyAllocatorAllocationData Buddy;

        FAllocationData() { }
    } AllocationData;

    union FAllocatorPointers
    {
        FVulkanPoolAllocator*  PoolAllocator;
        FVulkanBuddyAllocator* BuddyAllocator;
        void*                  AsVoid;

        FAllocatorPointers()
            : AsVoid(nullptr)
        {
        }
        
    } AllocatorPointers;
};

class FVulkanBuddyAllocator : public FVulkanDeviceChild
{
public:
    FVulkanBuddyAllocator(FVulkanDevice* InDevice, uint64 InBackingStorageSize, uint64 InMinBlockBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags);
    ~FVulkanBuddyAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void Deallocate(const FVulkanMemoryStorage& Storage);
    
    bool Initialize();
    void Destroy();

    void RecycleAllocation(const FVulkanBuddyAllocatorAllocationData& AllocationData);

    bool IsEmpty() const;
    
    FORCEINLINE bool HasSharedBuffer() const
    {
        return SharedBuffer != VK_NULL_HANDLE;
    }

    FORCEINLINE uint32 GetMemoryTypeIndex() const
    {
        return MemoryTypeIndex;
    }
    
    FORCEINLINE VkBufferUsageFlags GetBufferUsageFlags() const
    {
        return BufferUsageFlags;
    }

private:
    uint32 GetOrderForSize(uint64 SizeInBytes) const;
    uint64 GetOrderBlockSize(uint32 Order) const;

    uint64                   BackingStorageSize;
    uint64                   MinBlockBytes;
    uint32                   MemoryTypeIndex;
    VkMemoryAllocateFlags    AllocateFlags;
    VkBufferUsageFlags       BufferUsageFlags;
    VkDeviceMemory           DeviceMemory;
    VkBuffer                 SharedBuffer;
    VkDeviceAddress          BaseDeviceAddress;
    uint8*                   MappedBaseAddress;
    TArray<TArray<uint64>>   FreeOffsets;
    mutable FCriticalSection AllocatorCS;
};

class FVulkanMultiBuddyAllocator : public FVulkanDeviceChild
{
public:
    FVulkanMultiBuddyAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags);
    ~FVulkanMultiBuddyAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

    bool Initialize();
    void Destroy();
    void CleanUp();

private:
    bool CreateAllocator();

    uint64                         PageSizeBytes;
    uint64                         MinBlockBytes;
    uint32                         MemoryTypeIndex;
    VkMemoryAllocateFlags          AllocateFlags;
    VkBufferUsageFlags             BufferUsageFlags;
    TArray<FVulkanBuddyAllocator*> Allocators;
    mutable FCriticalSection       AllocatorsCS;
};

class FVulkanPoolAllocatorPage : public FVulkanDeviceChild
{
public:
    struct FFreeRange
    {
        uint64 Offset;
        uint64 Size;
    };

public:
    FVulkanPoolAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags = 0);
    ~FVulkanPoolAllocatorPage();

    bool TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FVulkanMemoryStorage& OutStorage);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, FVulkanPoolAllocatorAllocationData& OutData);
    
    bool Initialize();
    
    void RecycleAllocation(uint64 Offset, uint64 SizeInBytes);
    void TransferOwnership(uint64 Offset, FVulkanMemoryStorage* NewStorage);

    FORCEINLINE uint64         GetUsedBytes()    const { return UsedBytes; }
    FORCEINLINE uint64         GetPageSize()     const { return PageSizeBytes; }
    FORCEINLINE VkDeviceMemory GetDeviceMemory() const { return DeviceMemory; }

    FORCEINLINE bool IsEmpty() const
    {
        return UsedBytes == 0;
    }

    FORCEINLINE const TArray<FFreeRange>& GetFreeRanges() const
    {
        return FreeRanges;
    }

    FORCEINLINE const TArray<FVulkanPoolAllocatorAllocationData>& GetLiveAllocations() const
    {
        return LiveAllocations;
    }

private:
    void CoalesceFreeRanges();

    uint64                                     PageSizeBytes;
    uint64                                     Alignment;
    uint64                                     UsedBytes;
    uint32                                     MemoryTypeIndex;
    VkMemoryAllocateFlags                      AllocateFlags;
    VkBufferUsageFlags                         BufferUsageFlags;
    VkDeviceMemory                             DeviceMemory;
    VkBuffer                                   SharedBuffer;
    uint8*                                     MappedBaseAddress;
    TArray<FFreeRange>                         FreeRanges;
    TArray<FVulkanPoolAllocatorAllocationData> LiveAllocations;
};

class FVulkanPoolAllocator : public FVulkanDeviceChild
{
    static constexpr uint32 TLSFFirstLevelCount  = 32;
    static constexpr uint32 TLSFSecondLevelCount = 8;

public:
    FVulkanPoolAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxAllocationSize, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags = 0);
    ~FVulkanPoolAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 ExcludePageIndex, FVulkanPoolAllocatorAllocationData& OutData);
    void Deallocate(const FVulkanMemoryStorage& Storage);
    
    bool Initialize();
    void Destroy();
    void CleanUp();

    bool GetDefragCandidate(FVulkanPoolAllocatorAllocationData& OutCandidate) const;
    void RecycleAllocation(const FVulkanPoolAllocatorAllocationData& AllocationData);
    void TransferOwnership(const FVulkanPoolAllocatorAllocationData& Data, FVulkanMemoryStorage* NewStorage);

    VkDeviceMemory GetBackingMemory(uint32 PageIndex);

    FORCEINLINE uint64 GetFragmentedBytes() const
    {
        return FragmentedBytes;
    }
    
    FORCEINLINE uint64 GetAlignment() const
    {
        return Alignment;
    }

private:
    FVulkanPoolAllocatorPage* CreatePage(uint64 MinimumSize, uint32& OutPageIndex);
    void RebuildFragmentationData();

    uint64                            PageSizeBytes;
    uint64                            Alignment;
    uint64                            MaxAllocationSize;
    uint32                            MemoryTypeIndex;
    VkMemoryAllocateFlags             AllocateFlags;
    VkBufferUsageFlags                BufferUsageFlags;
    uint64                            FragmentedBytes;
    TArray<FVulkanPoolAllocatorPage*> Pages;
    mutable FCriticalSection          PagesCS;
};

class FVulkanLinearAllocatorPage : public FVulkanDeviceChild
{
public:
    FVulkanLinearAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, VkMemoryPropertyFlags InMemoryProperties, VkBufferUsageFlags InBufferUsageFlags, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanLinearAllocatorPage() = default;

    bool Initialize();

    FORCEINLINE FVulkanMemoryStorage& GetBackingStorage()
    {
        return BackingStorage;
    }

    FORCEINLINE const FVulkanMemoryStorage& GetBackingStorage() const
    {
        return BackingStorage;
    }

private:
    uint64                PageSizeBytes;
    VkMemoryPropertyFlags MemoryProperties;
    VkBufferUsageFlags    BufferUsageFlags;
    VkMemoryAllocateFlags AllocateFlags;
    FVulkanMemoryStorage  BackingStorage;
};

class FVulkanLinearAllocator : public FVulkanDeviceChild
{
    static constexpr int32 MAX_POOL_PAGES = 8;

public:
    FVulkanLinearAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, VkMemoryPropertyFlags InMemoryProperties, VkBufferUsageFlags InBufferUsageFlags, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanLinearAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void  ReturnPage(FVulkanLinearAllocatorPage* InPage);
    void  CleanUp();

private:
    FVulkanLinearAllocatorPage* AcquirePage();
    FVulkanLinearAllocatorPage* CreatePage();
    void* AllocateOversized(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

    uint64                              PageSizeBytes;
    VkMemoryPropertyFlags               MemoryProperties;
    VkBufferUsageFlags                  BufferUsageFlags;
    VkMemoryAllocateFlags               AllocateFlags;
    FVulkanLinearAllocatorPage*         CurrentPage;
    uint64                              CurrentOffset;
    TArray<FVulkanLinearAllocatorPage*> PagePool;
    FCriticalSection                    AllocatorCS;
};

class FVulkanBufferAllocatorPool : public FVulkanDeviceChild
{
public:
    FVulkanBufferAllocatorPool(FVulkanDevice* InDevice, uint32 InMemoryTypeIndex, VkBufferUsageFlags InBufferUsageFlags, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanBufferAllocatorPool();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

    bool Initialize();
    void CleanUp();

    FORCEINLINE uint32 GetMemoryTypeIndex() const
    {
        return MemoryTypeIndex;
    }

    FORCEINLINE VkBufferUsageFlags GetBufferUsageFlags() const
    {
        return BufferUsageFlags;
    }

private:
    void Destroy();

    uint32                     MemoryTypeIndex;
    VkBufferUsageFlags         BufferUsageFlags;
    VkMemoryAllocateFlags      AllocateFlags;
    uint64                     PageSizeBytes;
    uint64                     MinBlockBytes;
    uint64                     MaxSuballocationSize;
    FVulkanMultiBuddyAllocator MultiBuddyAllocator;
};

class FVulkanBufferAllocator : public FVulkanDeviceChild
{
public:
    FVulkanBufferAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize);
    ~FVulkanBufferAllocator();

    bool TryAllocate(VkMemoryPropertyFlags MemoryProperties, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

    bool Initialize();
    void Destroy();
    void CleanUp();

private:
    void ReleasePools();

    uint64                              PageSizeBytes;
    uint64                              MinBlockBytes;
    uint64                              MaxSuballocationSize;
    TArray<FVulkanBufferAllocatorPool*> Pools;
    FCriticalSection                    PoolsCS;
};

class FVulkanCommandContext;

class FVulkanTextureAllocator : public FVulkanDeviceChild
{
    enum class ETexturePoolClass : uint32
    {
        SmallReadOnly            = 0,
        ReadOnly                 = 1,
        RenderTargetDepthStencil = 2,
        StorageOnly              = 3,
        Count
    };

    static constexpr uint32 TexturePoolClassCount = static_cast<uint32>(ETexturePoolClass::Count);

    struct FPendingDefragMove
    {
        FVulkanMemoryStorage*                SourceStorage;
        VkImage                              NewImage;
        FVulkanPoolAllocator*                Allocator;
        FVulkanPoolAllocatorAllocationData   OldAllocationData;
        FVulkanPoolAllocatorAllocationData   NewAllocationData;
        uint64                               FenceValueAtCreation;
    };

public:
    FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes);
    ~FVulkanTextureAllocator();

    bool TryAllocate(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags MemoryProperties, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryStorage& OutStorage);

    void DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame);
    void CancelPendingDefragMoves(FVulkanGenericResource* Owner);

    bool Initialize();
    void Destroy();
    void CleanUp();

private:
    ETexturePoolClass ClassifyTexture(VkImageUsageFlags UsageFlags, uint64 Alignment) const;
    bool GetDefragCandidate(FVulkanPoolAllocatorAllocationData& OutCandidate, FVulkanPoolAllocator*& OutAllocator);
    void ReleasePools();

    uint64                     DefaultPageSizeBytes;
    FVulkanPoolAllocator*      Pools[TexturePoolClassCount];
    TArray<FPendingDefragMove> PendingDefragMoves;
    FCriticalSection           PoolsCS;
};

class FVulkanUploadHeapAllocator : public FVulkanDeviceChild
{
public:
    FVulkanUploadHeapAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallThreshold, uint64 InLargeThreshold);
    ~FVulkanUploadHeapAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryStorage& OutStorage);

    bool Initialize(uint32 InMemoryTypeIndex);
    void Destroy();
    void CleanUp();

private:
    void* AllocateOversized(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryStorage& OutStorage);

    uint64                      PageSizeBytes;
    uint64                      DefaultAlignment;
    uint64                      SmallThreshold;
    uint64                      LargeThreshold;
    uint32                      MemoryTypeIndex;
    FVulkanMultiBuddyAllocator* SmallAllocator;
    FVulkanPoolAllocator*       LargeAllocator;
    FVulkanMultiBuddyAllocator* ConstantsAllocator;
};

class FVulkanMemoryManager : public FVulkanDeviceChild
{
public:
    FVulkanMemoryManager(FVulkanDevice* InDevice);
    ~FVulkanMemoryManager();
        
    void* AllocateUploadMemory(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryStorage& OutStorage);
    void* AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void* AllocateStagingBuffer(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    bool  AllocateBufferMemory(VkMemoryPropertyFlags PropertyFlags, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    bool  AllocateImageMemory(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryStorage& OutStorage);

    bool Initialize();
    void CleanUpAllocators();

    void DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame);
    void CancelPendingDefragMoves(FVulkanGenericResource* Owner);

    VkResult AllocateMemory(const VkMemoryAllocateInfo* AllocateInfo, VkDeviceMemory* OutMemory);
    void     FreeMemory(VkDeviceMemory Memory);

    int64  GetActiveAllocationCount() const { return ActiveAllocationCount.Load(); }
    uint32 GetMaxAllocationCount()    const { return MaxAllocationCount; }

private:
    FVulkanBufferAllocator     BufferAllocator;
    FVulkanTextureAllocator    TextureAllocator;
    FVulkanUploadHeapAllocator UploadHeapAllocator;
    FVulkanLinearAllocator     DynamicConstantsAllocator;
    FVulkanLinearAllocator     StagingBufferAllocator;
    uint32                     UploadMemoryTypeIndex;
    uint32                     MaxAllocationCount;
    FAtomicInt64               ActiveAllocationCount;
};
