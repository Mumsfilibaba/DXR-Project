#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/ScopedLock.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanRefCounted.h"

static constexpr uint64 VULKAN_MIN_BUDDY_ALLOCATOR_BLOCK_SIZE = 16ull;

class FVulkanBuddyAllocator;
class FVulkanPoolAllocator;

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
    uint32 PageIndex = 0;
    uint64 Offset    = 0;
    uint64 Size      = 0;
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

    union FAllocationData
    {
        FVulkanBuddyAllocatorAllocationData Buddy;
        FVulkanPoolAllocatorAllocationData  Pool;

        FAllocationData() { }
    } AllocationData;

    union FAllocatorPointers
    {
        FVulkanBuddyAllocator* BuddyAllocator;
        FVulkanPoolAllocator*  PoolAllocator;
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
    void RecycleAllocation(const FVulkanBuddyAllocatorAllocationData& AllocationData);

    bool Initialize();
    void Destroy();

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
    FVulkanPoolAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanPoolAllocatorPage();

    bool TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FVulkanMemoryStorage& OutStorage);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, FVulkanPoolAllocatorAllocationData& OutData);
    void RecycleAllocation(uint64 Offset, uint64 SizeInBytes);

    bool Initialize();
    
    uint64         GetUsedBytes()    const { return UsedBytes; }
    uint64         GetPageSize()     const { return PageSizeBytes; }
    VkDeviceMemory GetDeviceMemory() const { return DeviceMemory; }

    bool IsEmpty() const
    {
        return UsedBytes == 0;
    }

    const TArray<FFreeRange>& GetFreeRanges() const
    {
        return FreeRanges;
    }

private:
    void CoalesceFreeRanges();

    uint64                PageSizeBytes;
    uint64                Alignment;
    uint64                UsedBytes;
    uint32                MemoryTypeIndex;
    VkMemoryAllocateFlags AllocateFlags;
    VkDeviceMemory        DeviceMemory;
    uint8*                MappedBaseAddress;
    TArray<FFreeRange>    FreeRanges;
};

class FVulkanPoolAllocator : public FVulkanDeviceChild
{
    static constexpr uint32 TLSFFirstLevelCount  = 32;
    static constexpr uint32 TLSFSecondLevelCount = 8;

public:
    FVulkanPoolAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxAllocationSize, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags);
    ~FVulkanPoolAllocator();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    bool TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 ExcludePageIndex, FVulkanPoolAllocatorAllocationData& OutData);
    void Deallocate(const FVulkanMemoryStorage& Storage);
    void RecycleAllocation(const FVulkanPoolAllocatorAllocationData& AllocationData);

    bool Initialize();
    void Destroy();
    void CleanUp();

    VkDeviceMemory GetBackingMemory(uint32 PageIndex);

    uint64 GetFragmentedBytes() const
    {
        return FragmentedBytes;
    }
    
    uint64 GetAlignment() const
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
    uint64                            FragmentedBytes;
    TArray<FVulkanPoolAllocatorPage*> Pages;
    mutable FCriticalSection          PagesCS;
};

class FVulkanLinearAllocatorPage : public FVulkanDeviceChild
{
public:
    FVulkanLinearAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags);
    ~FVulkanLinearAllocatorPage();

    bool TryAllocate(uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset);
    bool Initialize();
    void Reset();

    bool IsExhausted() const { return CurrentOffset >= PageSizeBytes; }
    void ResetOffset()       { CurrentOffset = 0; }

    VkBuffer       GetBuffer()       const { return Buffer; }
    VkDeviceMemory GetDeviceMemory() const { return DeviceMemory; }
    uint8*         GetMappedMemory() const { return MappedBaseAddress; }
    uint64         GetPageSize()     const { return PageSizeBytes; }

private:
    uint64                PageSizeBytes;
    uint32                MemoryTypeIndex;
    VkMemoryAllocateFlags AllocateFlags;
    VkBufferUsageFlags    BufferUsageFlags;
    uint64                CurrentOffset;
    VkDeviceMemory        DeviceMemory;
    VkBuffer              Buffer;
    uint8*                MappedBaseAddress;
};

class FVulkanLinearAllocator : public FVulkanDeviceChild
{
public:
    FVulkanLinearAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags);
    ~FVulkanLinearAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void CleanUp();

private:
    FVulkanLinearAllocatorPage* CreatePage();
    void RetirePage(FVulkanLinearAllocatorPage* Page);

    uint64                              PageSizeBytes;
    uint32                              MemoryTypeIndex;
    VkMemoryAllocateFlags               AllocateFlags;
    VkBufferUsageFlags                  BufferUsageFlags;
    TArray<FVulkanLinearAllocatorPage*> Pages;
    TArray<FVulkanLinearAllocatorPage*> FullPages;
    FCriticalSection                    PagesCS;
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

public:
    FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes);
    ~FVulkanTextureAllocator();

    bool TryAllocate(VkImage Image, VkMemoryPropertyFlags MemoryProperties, VkImageUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryStorage& OutStorage);

    bool Initialize();
    void Destroy();
    void CleanUp();

private:
    ETexturePoolClass ClassifyTexture(VkImageUsageFlags UsageFlags, uint64 Alignment) const;
    void ReleasePools();

    uint64                DefaultPageSizeBytes;
    FVulkanPoolAllocator* Pools[TexturePoolClassCount];
    FCriticalSection      PoolsCS;
};

class FVulkanUploadHeapAllocator : public FVulkanDeviceChild
{
public:
    FVulkanUploadHeapAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallThreshold, uint64 InLargeThreshold);
    ~FVulkanUploadHeapAllocator();

    void* Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void* AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

    bool Initialize(uint32 InMemoryTypeIndex);
    void Destroy();
    void CleanUp();

private:
    void* AllocateOversized(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

    uint64                      PageSizeBytes;
    uint64                      DefaultAlignment;
    uint64                      SmallThreshold;
    uint64                      LargeThreshold;
    uint32                      MemoryTypeIndex;
    FVulkanMultiBuddyAllocator* SmallAllocator;
    FVulkanMultiBuddyAllocator* LargeAllocator;
    FVulkanMultiBuddyAllocator* ConstantsAllocator;
};

class FVulkanMemoryManager : public FVulkanDeviceChild
{
public:
    FVulkanMemoryManager(FVulkanDevice* InDevice, uint32 InUploadMemoryTypeIndex);
    ~FVulkanMemoryManager();

    bool Initialize();

    void CleanUpAllocators();
    void CleanUpLinearAllocators();

    bool  AllocateBufferMemory(VkMemoryPropertyFlags PropertyFlags, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    bool  AllocateImageMemory(VkImage Image, VkMemoryPropertyFlags PropertyFlags, VkImageUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryStorage& OutStorage);
    void* AllocateUploadMemory(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void* AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);
    void* AllocateStagingBuffer(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage);

private:
    FVulkanBufferAllocator     BufferAllocator;
    FVulkanTextureAllocator    TextureAllocator;
    FVulkanUploadHeapAllocator UploadHeapAllocator;
    FVulkanLinearAllocator     DynamicConstantsAllocator;
    FVulkanLinearAllocator     StagingBufferAllocator;
    uint32                     UploadMemoryTypeIndex;
};
