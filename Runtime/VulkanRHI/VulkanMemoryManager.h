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
class FVulkanResource;
class FVulkanMemoryLocation;
class FVulkanCommandContext;
class FVulkanDevice;

void VulkanQueryBufferMemoryRequirements(FVulkanDevice* Device, const VkBufferCreateInfo& BufferCreateInfo, VkMemoryRequirements2& OutRequirements);
void VulkanQueryImageMemoryRequirements(FVulkanDevice* Device, const VkImageCreateInfo& ImageCreateInfo, VkMemoryRequirements2& OutRequirements, VkImage ExistingImage = VK_NULL_HANDLE);

enum class EVulkanAllocatorType : uint8
{
    None,
    BuddyAllocator,
    PoolAllocator,
};

enum class EVulkanMemoryLocationType : uint8
{
    Unknown,
    Dedicated,
    Suballocated,
};

#if VULKAN_ENABLE_STATS
struct FVulkanAllocatorUsage
{
    uint64 AllocatedBytes  = 0;
    uint64 UsedBytes       = 0;
    uint64 FragmentedBytes = 0;
};
#endif

struct FVulkanBuddyAllocatorAllocationData
{
    uint32 Order         = 0;
    uint64 Offset        = 0;
    uint64 RequestedSize = 0;
};

struct FVulkanPoolAllocatorAllocationData
{
    uint32                 PageIndex              = UINT32_MAX;
    uint64                 Offset                 = 0;
    uint64                 Size                   = 0;
    FVulkanMemoryLocation* Owner                  = nullptr;
    uint64                 EligibleFromFenceValue = UINT64_MAX;
};

struct FVulkanDefragCandidate
{
    FVulkanMemoryLocation*             SourceLocation = nullptr;
    FVulkanResource*                   Owner          = nullptr;
    FVulkanPoolAllocatorAllocationData AllocationData = {};
    VkBuffer                           BackingBuffer  = VK_NULL_HANDLE;
    VkDeviceSize                       BufferOffset   = 0;
};

struct FVulkanPendingDefragMove
{
    FVulkanMemoryLocation*             SourceLocation       = nullptr;
    VkImage                            NewImage             = VK_NULL_HANDLE;
    VkBuffer                           NewBuffer            = VK_NULL_HANDLE;
    VkDeviceAddress                    NewDeviceAddress     = 0;
    FVulkanPoolAllocator*              Allocator            = nullptr;
    FVulkanPoolAllocatorAllocationData OldAllocationData    = {};
    FVulkanPoolAllocatorAllocationData NewAllocationData    = {};
    uint64                             FenceValueAtCreation = 0;
    VkImageLayout                      RestLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
};

class FVulkanMemoryLocation : public FVulkanDeviceChild, public FNonCopyable
{
public:
    FVulkanMemoryLocation(FVulkanDevice* InDevice);
    ~FVulkanMemoryLocation();

    void Swap(FVulkanMemoryLocation& Other);
    void ReleaseMemory();
    void Reset();
    void ResetAllocator();
    void UpdateOwnership();
    void FinalizeAllocation();

    FORCEINLINE bool IsValid()        const { return LocationType != EVulkanMemoryLocationType::Unknown; }
    FORCEINLINE bool IsSuballocated() const { return LocationType == EVulkanMemoryLocationType::Suballocated; }

    // Memory Accessors
    FORCEINLINE VkDeviceMemory  GetMemory()            const { return DeviceMemory; }
    FORCEINLINE VkDeviceSize    GetMemoryOffset()      const { return MemoryOffset; }
    FORCEINLINE VkDeviceSize    GetSize()              const { return Size; }
    FORCEINLINE void*           GetMappedBaseAddress() const { return MappedBaseAddress; }

    // Buffer Accessors
    FORCEINLINE VkBuffer        GetBackingBuffer()     const { return BackingBuffer; }
    FORCEINLINE VkDeviceSize    GetBufferOffset()      const { return BufferOffset; }
    FORCEINLINE VkDeviceAddress GetDeviceAddress()     const { return DeviceAddress; }

    // Allocator Accessors
    FORCEINLINE void*                     GetAllocator()      const { return AllocatorPointers.AsVoid; }
    FORCEINLINE FVulkanBuddyAllocator*    GetBuddyAllocator() const { return (AllocatorType == EVulkanAllocatorType::BuddyAllocator) ? AllocatorPointers.BuddyAllocator : nullptr; }
    FORCEINLINE FVulkanPoolAllocator*     GetPoolAllocator()  const { return (AllocatorType == EVulkanAllocatorType::PoolAllocator) ? AllocatorPointers.PoolAllocator : nullptr; }
    FORCEINLINE FVulkanResource*          GetOwner()          const { return Owner; }
    FORCEINLINE EVulkanAllocatorType      GetAllocatorType()  const { return AllocatorType; }
    FORCEINLINE EVulkanMemoryLocationType GetLocationType()   const { return LocationType; }

    FORCEINLINE const FVulkanPoolAllocatorAllocationData&  GetPoolAllocationData()  const { return AllocationData.Pool; }
    FORCEINLINE const FVulkanBuddyAllocatorAllocationData& GetBuddyAllocationData() const { return AllocationData.Buddy; }

    FORCEINLINE void SetMemory(VkDeviceMemory InMemory)                { DeviceMemory = InMemory; }
    FORCEINLINE void SetMemoryOffset(VkDeviceSize InOffset)            { MemoryOffset = InOffset; }
    FORCEINLINE void SetBackingBuffer(VkBuffer InBuffer)               { BackingBuffer = InBuffer; }
    FORCEINLINE void SetBufferOffset(VkDeviceSize InOffset)            { BufferOffset = InOffset; }
    FORCEINLINE void SetDeviceAddress(VkDeviceAddress InAddress)       { DeviceAddress = InAddress; }
    FORCEINLINE void SetMappedBaseAddress(void* InAddress)             { MappedBaseAddress = InAddress; }
    FORCEINLINE void SetSize(VkDeviceSize InSize)                      { Size = InSize; }
    FORCEINLINE void SetLocationType(EVulkanMemoryLocationType InType) { LocationType = InType; }
    FORCEINLINE void SetOwner(FVulkanResource* InOwner)                { Owner = InOwner; }

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

    union FAllocationData
    {
        FVulkanPoolAllocatorAllocationData  Pool;
        FVulkanBuddyAllocatorAllocationData Buddy;

        FAllocationData() {}
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

    VkDeviceMemory            DeviceMemory;
    VkDeviceSize              MemoryOffset;
    VkBuffer                  BackingBuffer;
    VkDeviceSize              BufferOffset;
    VkDeviceAddress           DeviceAddress;
    void*                     MappedBaseAddress;
    VkDeviceSize              Size;
    FVulkanResource*          Owner;
    EVulkanAllocatorType      AllocatorType;
    EVulkanMemoryLocationType LocationType;
};

class FVulkanBuddyAllocator : public FVulkanDeviceChild
{
public:
    FVulkanBuddyAllocator(FVulkanDevice* InDevice, uint64 InBackingStorageSize, uint64 InMinBlockBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags);
    ~FVulkanBuddyAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);
    void Deallocate(const FVulkanMemoryLocation& Location);
    
    bool Initialize();
    void Destroy();

    void RecycleAllocation(const FVulkanBuddyAllocatorAllocationData& AllocationData);

    bool IsEmpty() const;

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const;
#endif

    uint64 GetBackingStorageSize() const
    {
        return BackingStorageSize;
    }

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
    AtomicInt64              TrackedUsedBytes;
    AtomicInt64              TrackedWastedBytes;
    mutable FCriticalSection AllocatorCS;
};

class FVulkanMultiBuddyAllocator : public FVulkanDeviceChild
{
public:
    FVulkanMultiBuddyAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags);
    ~FVulkanMultiBuddyAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);

    bool Initialize();
    void Destroy();
    void CleanUp();

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const;
#endif

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

    bool TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FVulkanMemoryLocation& OutLocation);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FVulkanPoolAllocatorAllocationData& OutData);
    
    bool Initialize();
    
    void RecycleAllocation(uint64 Offset, uint64 SizeInBytes);
    bool TransferOwnership(uint64 Offset, FVulkanMemoryLocation* NewLocation);
    bool FinalizeAllocation(uint64 Offset, uint64 EligibleFromFenceValue);

    FORCEINLINE bool IsEmpty() const
    {
        return UsedBytes == 0;
    }

    FORCEINLINE uint64 GetUsedBytes() const
    {
        return UsedBytes;
    }

    FORCEINLINE uint64 GetPageSize() const
    {
        return PageSizeBytes;
    }
    
    FORCEINLINE VkDeviceMemory GetDeviceMemory() const
    {
        return DeviceMemory;
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
    VkDeviceAddress                            BaseDeviceAddress;
    uint8*                                     MappedBaseAddress;
    TArray<FFreeRange>                         FreeRanges;
    TArray<FVulkanPoolAllocatorAllocationData> LiveAllocations;
};

class FVulkanPoolAllocator : public FVulkanDeviceChild
{
    static constexpr uint32 TLSF_FIRST_LEVEL_COUNT  = 32;
    static constexpr uint32 TLSF_SECOND_LEVEL_COUNT = 8;

public:
    FVulkanPoolAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxAllocationSize, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags = 0);
    ~FVulkanPoolAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 ExcludePageIndex, FVulkanPoolAllocatorAllocationData& OutData);
    void Deallocate(const FVulkanMemoryLocation& Location);
    
    bool Initialize();
    void Destroy();
    void CleanUp();

    bool GetDefragCandidate(FVulkanDefragCandidate& OutCandidate) const;
    void RecycleAllocation(const FVulkanPoolAllocatorAllocationData& AllocationData);
    void TransferOwnership(const FVulkanPoolAllocatorAllocationData& Data, FVulkanMemoryLocation* NewLocation);

    void FinalizeAllocation(const FVulkanPoolAllocatorAllocationData& Data);

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const;
#endif

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
    AtomicInt64                       TrackedAllocatedBytes;
    AtomicInt64                       TrackedUsedBytes;
    TArray<FVulkanPoolAllocatorPage*> Pages;
    mutable FCriticalSection          PagesCS;
};

class FVulkanLinearAllocatorPage : public FVulkanDeviceChild
{
public:
    FVulkanLinearAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, VkMemoryPropertyFlags InMemoryProperties, VkBufferUsageFlags InBufferUsageFlags, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanLinearAllocatorPage() = default;

    bool Initialize();

    FORCEINLINE FVulkanMemoryLocation& GetBackingLocation()
    {
        return BackingLocation;
    }

    FORCEINLINE const FVulkanMemoryLocation& GetBackingLocation() const
    {
        return BackingLocation;
    }

private:
    uint64                PageSizeBytes;
    VkMemoryPropertyFlags MemoryProperties;
    VkBufferUsageFlags    BufferUsageFlags;
    VkMemoryAllocateFlags AllocateFlags;
    FVulkanMemoryLocation BackingLocation;
};

class FVulkanLinearAllocator : public FVulkanDeviceChild
{
    static constexpr int32 MAX_POOL_PAGES = 8;

public:
    FVulkanLinearAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, VkMemoryPropertyFlags InMemoryProperties, VkBufferUsageFlags InBufferUsageFlags, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanLinearAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);
    void  ReturnPage(FVulkanLinearAllocatorPage* InPage);
    void  CleanUp();

private:
    FVulkanLinearAllocatorPage* AcquirePage();
    FVulkanLinearAllocatorPage* CreatePage();
    void* AllocateOversized(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);

    uint64                              PageSizeBytes;
    VkMemoryPropertyFlags               MemoryProperties;
    VkBufferUsageFlags                  BufferUsageFlags;
    VkMemoryAllocateFlags               AllocateFlags;
    FVulkanLinearAllocatorPage*         CurrentPage;
    uint64                              CurrentOffset;
    TArray<FVulkanLinearAllocatorPage*> PagePool;
    FCriticalSection                    AllocatorCS;
};

#if VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
class FVulkanBufferAllocatorPool : public FVulkanDeviceChild
{
public:
    FVulkanBufferAllocatorPool(FVulkanDevice* InDevice, uint32 InMemoryTypeIndex, VkBufferUsageFlags InBufferUsageFlags, uint64 InPageSizeBytes, uint64 InMaxSuballocationSize, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanBufferAllocatorPool();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);

    bool Initialize();
    void CleanUp();

    bool           GetDefragCandidate(FVulkanDefragCandidate& OutCandidate);
    bool           TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 ExcludePageIndex, FVulkanPoolAllocatorAllocationData& OutData);
    void           TransferOwnership(const FVulkanPoolAllocatorAllocationData& Data, FVulkanMemoryLocation* NewLocation);
    VkDeviceMemory GetBackingMemory(uint32 PageIndex);

    FVulkanPoolAllocator& GetPoolAllocator() { return PoolAllocator; }

    FORCEINLINE uint32                GetMemoryTypeIndex()  const { return MemoryTypeIndex;  }
    FORCEINLINE VkBufferUsageFlags    GetBufferUsageFlags() const { return BufferUsageFlags; }
    FORCEINLINE VkMemoryAllocateFlags GetAllocateFlags()    const { return AllocateFlags;    }
    FORCEINLINE uint64                GetFragmentedBytes()  const { return PoolAllocator.GetFragmentedBytes(); }
    FORCEINLINE uint64                GetAlignment()        const { return PoolAllocator.GetAlignment();       }

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const
    {
        PoolAllocator.UpdateMemoryStats(OutUsage);
    }
#endif

private:
    void Destroy();

    uint32                MemoryTypeIndex;
    VkBufferUsageFlags    BufferUsageFlags;
    VkMemoryAllocateFlags AllocateFlags;
    uint64                PageSizeBytes;
    uint64                MaxSuballocationSize;
    FVulkanPoolAllocator  PoolAllocator;
};
#else
class FVulkanBufferAllocatorPool : public FVulkanDeviceChild
{
public:
    FVulkanBufferAllocatorPool(FVulkanDevice* InDevice, uint32 InMemoryTypeIndex, VkBufferUsageFlags InBufferUsageFlags, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanBufferAllocatorPool();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);

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

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const
    {
        MultiBuddyAllocator.UpdateMemoryStats(OutUsage);
    }
#endif

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
#endif

#if VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
class FVulkanBufferAllocator : public FVulkanDeviceChild
{
public:
    FVulkanBufferAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize);
    ~FVulkanBufferAllocator();

    bool TryAllocate(VkMemoryPropertyFlags MemoryProperties, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);

    void DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame);
    void CancelPendingDefragMoves(FVulkanResource* Owner);

    bool Initialize();
    void Destroy();
    void CleanUp();

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    bool GetDefragCandidate(FVulkanDefragCandidate& OutCandidate, FVulkanBufferAllocatorPool*& OutPool);
    void ReleasePools();

    uint64                              PageSizeBytes;
    uint64                              MaxSuballocationSize;
    TArray<FVulkanBufferAllocatorPool*> Pools;
    TArray<FVulkanPendingDefragMove>    PendingDefragMoves;
    FCriticalSection                    PoolsCS;
};
#else
class FVulkanBufferAllocator : public FVulkanDeviceChild
{
public:
    FVulkanBufferAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize);
    ~FVulkanBufferAllocator();

    bool TryAllocate(VkMemoryPropertyFlags MemoryProperties, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);

    bool Initialize();
    void Destroy();
    void CleanUp();

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    void ReleasePools();

    uint64                              PageSizeBytes;
    uint64                              MinBlockBytes;
    uint64                              MaxSuballocationSize;
    TArray<FVulkanBufferAllocatorPool*> Pools;
    FCriticalSection                    PoolsCS;
};
#endif

#if VULKAN_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
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

    static constexpr uint32 TEXTURE_POOL_CLASS_COUNT = static_cast<uint32>(ETexturePoolClass::Count);

public:
    FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes);
    ~FVulkanTextureAllocator();

    bool TryAllocate(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags MemoryProperties, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryLocation& OutLocation);

    void DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame);
    void CancelPendingDefragMoves(FVulkanResource* Owner);

    bool Initialize();
    void Destroy();
    void CleanUp();

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    ETexturePoolClass ClassifyTexture(VkImageUsageFlags UsageFlags, uint64 Alignment) const;
    bool GetDefragCandidate(FVulkanDefragCandidate& OutCandidate, FVulkanPoolAllocator*& OutAllocator);
    void ReleasePools();

    uint64                           DefaultPageSizeBytes;
    FVulkanPoolAllocator*            Pools[TEXTURE_POOL_CLASS_COUNT];
    TArray<FVulkanPendingDefragMove> PendingDefragMoves;
    FCriticalSection                 PoolsCS;
};
#else
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

    static constexpr uint32 TEXTURE_POOL_CLASS_COUNT = static_cast<uint32>(ETexturePoolClass::Count);

public:
    FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes);
    ~FVulkanTextureAllocator();

    bool TryAllocate(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags MemoryProperties, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryLocation& OutLocation);

    bool Initialize();
    void Destroy();
    void CleanUp();

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    ETexturePoolClass ClassifyTexture(VkImageUsageFlags UsageFlags, uint64 Alignment) const;
    void ReleasePools();

    uint64                      DefaultPageSizeBytes;
    FVulkanMultiBuddyAllocator* Pools[TEXTURE_POOL_CLASS_COUNT];
    FCriticalSection            PoolsCS;
};
#endif

class FVulkanUploadHeapAllocator : public FVulkanDeviceChild
{
public:
    FVulkanUploadHeapAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallThreshold, uint64 InLargeThreshold);
    ~FVulkanUploadHeapAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryLocation& OutLocation);

    bool Initialize(uint32 InMemoryTypeIndex);
    void Destroy();
    void CleanUp();

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats();
#endif

private:
    void* AllocateOversized(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryLocation& OutLocation);

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
        
    void* AllocateUploadMemory(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryLocation& OutLocation);
    void* AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);
    void* AllocateStagingBuffer(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);
    bool  AllocateBufferMemory(VkMemoryPropertyFlags PropertyFlags, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation);
    bool  AllocateImageMemory(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryLocation& OutLocation);

    VkResult AllocateMemory(const VkMemoryAllocateInfo* AllocateInfo, VkDeviceMemory* OutMemory);
    void FreeMemory(VkDeviceMemory Memory);

    bool Initialize();
    void CleanUpAllocators();

    void DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame);
    void CancelPendingDefragMoves(FVulkanResource* Owner);

#if VULKAN_ENABLE_STATS
    void UpdateMemoryStats();
#endif

    int64 GetActiveAllocationCount() const
    {
        return ActiveAllocationCount.Load();
    }
    
    uint32 GetMaxAllocationCount() const
    {
        return MaxAllocationCount;
    }

private:
    FVulkanBufferAllocator     BufferAllocator;
    FVulkanTextureAllocator    TextureAllocator;
    FVulkanUploadHeapAllocator UploadHeapAllocator;
    FVulkanLinearAllocator     DynamicConstantsAllocator;
    FVulkanLinearAllocator     StagingBufferAllocator;
    uint32                     UploadMemoryTypeIndex;
    uint32                     MaxAllocationCount;
    AtomicInt64                ActiveAllocationCount;
};
