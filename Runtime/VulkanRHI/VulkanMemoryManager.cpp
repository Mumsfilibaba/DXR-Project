#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Math/Math.h"
#include "VulkanRHI/VulkanMemoryManager.h"
#include "VulkanRHI/VulkanStats.h"
#include "VulkanRHI/VulkanResource.h"
#include "VulkanRHI/VulkanTexture.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "VulkanRHI/VulkanFence.h"
#include "VulkanRHI/VulkanRHI.h"

#if VULKAN_ENABLE_MEMORY_LOGGING
static TAutoConsoleVariable<bool> CVarVulkanLogMemoryAllocations(
    "VulkanRHI.LogMemoryAllocations",
    "Log when new memory allocator pages or dedicated allocations are created",
    false);
#endif

static TAutoConsoleVariable<int32> CVarBufferAllocatorPageSize(
    "VulkanRHI.BufferAllocatorPageSize",
    "Page size for the buffer buddy allocator in MB",
    64);

static TAutoConsoleVariable<int32> CVarBufferAllocatorMaxSuballocationSize(
    "VulkanRHI.BufferAllocatorMaxSuballocationSize",
    "Max suballocation size before buffer allocations become dedicated in MB",
    32);

static TAutoConsoleVariable<int32> CVarTextureAllocatorDefaultPageSize(
    "VulkanRHI.TextureAllocatorDefaultPageSize",
    "Default page size for pooled texture allocator pages in MB",
    64);

static TAutoConsoleVariable<int32> CVarUploadHeapPageSize(
    "VulkanRHI.UploadHeapPageSize",
    "Page size for the upload heap allocator in KB",
    8 * 1024);

static TAutoConsoleVariable<int32> CVarUploadHeapSmallAllocationThreshold(
    "VulkanRHI.UploadHeapSmallAllocationThreshold",
    "Allocation size threshold for the upload small allocator path (bytes)",
    64 * 1024);

static TAutoConsoleVariable<int32> CVarUploadHeapLargeAllocationThreshold(
    "VulkanRHI.UploadHeapLargeAllocationThreshold",
    "Max suballocation size before upload allocations become standalone (bytes)",
    2 * 1024 * 1024);

static TAutoConsoleVariable<int32> CVarDynamicConstantsAllocatorPageSize(
    "VulkanRHI.DynamicConstantsAllocatorPageSize",
    "Page size for the dynamic constants linear allocator in KB",
    2 * 1024);

static TAutoConsoleVariable<int32> CVarStagingBufferPageSize(
    "VulkanRHI.StagingBufferPageSize",
    "Page size for the staging buffer linear allocator in KB",
    4 * 1024);

static constexpr uint64 BUFFER_MIN_BLOCK  = 256ull;
static constexpr uint64 UPLOAD_ALIGNMENT  = 256ull;

FVulkanMemoryLocation::FVulkanMemoryLocation(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Memory(VK_NULL_HANDLE)
    , MemoryOffset(0)
    , BackingBuffer(VK_NULL_HANDLE)
    , BufferOffset(0)
    , DeviceAddress(0)
    , MappedBaseAddress(nullptr)
    , Size(0)
    , AllocatorType(EVulkanAllocatorType::None)
    , LocationType(EVulkanMemoryLocationType::Unknown)
    , Owner(nullptr)
    , AllocationData()
    , AllocatorPointers()
{
}

FVulkanMemoryLocation::~FVulkanMemoryLocation()
{
    ReleaseMemory();
}

void FVulkanMemoryLocation::Swap(FVulkanMemoryLocation& Other)
{
    if (this == &Other)
    {
        return;
    }

    FMemory::Memswap(&AllocationData, &Other.AllocationData, sizeof(AllocationData));

    ::Swap(Device, Other.Device);
    ::Swap(Memory, Other.Memory);
    ::Swap(MemoryOffset, Other.MemoryOffset);
    ::Swap(BackingBuffer, Other.BackingBuffer);
    ::Swap(BufferOffset, Other.BufferOffset);
    ::Swap(DeviceAddress, Other.DeviceAddress);
    ::Swap(MappedBaseAddress, Other.MappedBaseAddress);
    ::Swap(Size, Other.Size);
    ::Swap(AllocatorType, Other.AllocatorType);
    ::Swap(LocationType, Other.LocationType);
    ::Swap(Owner, Other.Owner);
    ::Swap(AllocatorPointers.AsVoid, Other.AllocatorPointers.AsVoid);
}

void FVulkanMemoryLocation::ReleaseMemory()
{
    if (!IsValid())
    {
        return;
    }

    switch (AllocatorType)
    {
        case EVulkanAllocatorType::BuddyAllocator:
        {
            CHECK(AllocatorPointers.BuddyAllocator != nullptr);
            AllocatorPointers.BuddyAllocator->Deallocate(*this);
            break;
        }
        case EVulkanAllocatorType::PoolAllocator:
        {
            CHECK(AllocatorPointers.PoolAllocator != nullptr);
            AllocatorPointers.PoolAllocator->Deallocate(*this);
            break;
        }
        case EVulkanAllocatorType::None:
        {
            if (LocationType == EVulkanMemoryLocationType::Dedicated)
            {
                FVulkanDeviceRHI::DeferDeletion(Memory, BackingBuffer);
            }

            break;
        }
    }

    Reset();
}

void FVulkanMemoryLocation::Reset()
{
    Memory                   = VK_NULL_HANDLE;
    MemoryOffset             = 0;
    BackingBuffer            = VK_NULL_HANDLE;
    BufferOffset             = 0;
    DeviceAddress            = 0;
    MappedBaseAddress        = nullptr;
    Size                     = 0;
    AllocatorType            = EVulkanAllocatorType::None;
    LocationType              = EVulkanMemoryLocationType::Unknown;
    Owner                    = nullptr;
    AllocatorPointers.AsVoid = nullptr;
}

FVulkanMemoryManager::FVulkanMemoryManager(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , BufferAllocator(InDevice, static_cast<uint64>(CVarBufferAllocatorPageSize.GetValue()) * 1024ull * 1024ull, BUFFER_MIN_BLOCK, static_cast<uint64>(CVarBufferAllocatorMaxSuballocationSize.GetValue()) * 1024ull * 1024ull)
    , TextureAllocator(InDevice, static_cast<uint64>(CVarTextureAllocatorDefaultPageSize.GetValue()) * 1024ull * 1024ull)
    , UploadHeapAllocator(InDevice, static_cast<uint64>(CVarUploadHeapPageSize.GetValue()) * 1024ull, UPLOAD_ALIGNMENT, static_cast<uint64>(CVarUploadHeapSmallAllocationThreshold.GetValue()), static_cast<uint64>(CVarUploadHeapLargeAllocationThreshold.GetValue()))
    , DynamicConstantsAllocator(InDevice, static_cast<uint64>(CVarDynamicConstantsAllocatorPageSize.GetValue()) * 1024ull, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 0)
    , StagingBufferAllocator(InDevice, static_cast<uint64>(CVarStagingBufferPageSize.GetValue()) * 1024ull, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 0)
    , UploadMemoryTypeIndex(0)
    , MaxAllocationCount(0)
    , ActiveAllocationCount(0)
{
}

bool FVulkanMemoryManager::Initialize()
{
    MaxAllocationCount = GetDevice()->GetPhysicalDevice()->GetProperties().limits.maxMemoryAllocationCount;

    const VkMemoryPropertyFlags UploadMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = 256;
    BufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDeviceBufferMemoryRequirements DeviceBufferMemReqInfo = {};
    DeviceBufferMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
    DeviceBufferMemReqInfo.pCreateInfo = &BufferCreateInfo;

    VkMemoryRequirements2 MemReqs2 = {};
    MemReqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetDeviceBufferMemoryRequirements(GetDevice()->GetVkDevice(), &DeviceBufferMemReqInfo, &MemReqs2);

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemReqs2.memoryRequirements.memoryTypeBits, UploadMemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanMemoryManager: No suitable host-visible memory type for upload heap");
        return false;
    }

    UploadMemoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

    if (!BufferAllocator.Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanMemoryManager: Failed to initialize BufferAllocator");
        return false;
    }

    if (!TextureAllocator.Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanMemoryManager: Failed to initialize TextureAllocator");
        return false;
    }

    if (!UploadHeapAllocator.Initialize(UploadMemoryTypeIndex))
    {
        VULKAN_ERROR_CRITICAL("FVulkanMemoryManager: Failed to initialize UploadHeapAllocator");
        return false;
    }

    VULKAN_INFO("FVulkanMemoryManager: All allocators initialized (UploadMemoryTypeIndex=%u)", UploadMemoryTypeIndex);
    return true;
}

FVulkanMemoryManager::~FVulkanMemoryManager()
{
}

void FVulkanMemoryManager::CleanUpAllocators()
{
    BufferAllocator.CleanUp();
    TextureAllocator.CleanUp();
    UploadHeapAllocator.CleanUp();
    DynamicConstantsAllocator.CleanUp();
    StagingBufferAllocator.CleanUp();
}

bool FVulkanMemoryManager::AllocateBufferMemory(VkMemoryPropertyFlags PropertyFlags, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    return BufferAllocator.TryAllocate(PropertyFlags, UsageFlags, AllocateFlags, SizeInBytes, Alignment, OutLocation);
}

bool FVulkanMemoryManager::AllocateImageMemory(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryLocation& OutLocation)
{
    return TextureAllocator.TryAllocate(Image, ImageCreateInfo, PropertyFlags, AllocateFlags, OutLocation);
}

void* FVulkanMemoryManager::AllocateUploadMemory(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryLocation& OutLocation)
{
    return UploadHeapAllocator.Allocate(SizeInBytes, Alignment, BufferUsageFlags, OutLocation);
}

void* FVulkanMemoryManager::AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    void* Result = DynamicConstantsAllocator.Allocate(SizeInBytes, Alignment, OutLocation);
    if (!Result)
    {
        Result = UploadHeapAllocator.Allocate(SizeInBytes, Alignment, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, OutLocation);
    }
    
    return Result;
}

void* FVulkanMemoryManager::AllocateStagingBuffer(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    void* Result = StagingBufferAllocator.Allocate(SizeInBytes, Alignment, OutLocation);
    if (!Result)
    {
        Result = UploadHeapAllocator.Allocate(SizeInBytes, Alignment, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, OutLocation);
    }

    return Result;
}

void FVulkanMemoryManager::DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame)
{
#if VULKAN_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    TextureAllocator.DefragmentAllocations(InCommandContext, MaxMovesPerFrame);
#endif
#if VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    BufferAllocator.DefragmentAllocations(InCommandContext, MaxMovesPerFrame);
#endif
}

void FVulkanMemoryManager::CancelPendingDefragMoves(FVulkanResource* Owner)
{
#if VULKAN_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    TextureAllocator.CancelPendingDefragMoves(Owner);
#endif
#if VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    BufferAllocator.CancelPendingDefragMoves(Owner);
#endif
}

VkResult FVulkanMemoryManager::AllocateMemory(const VkMemoryAllocateInfo* AllocateInfo, VkDeviceMemory* OutMemory)
{
    VkResult Result = vkAllocateMemory(GetDevice()->GetVkDevice(), AllocateInfo, nullptr, OutMemory);
    if (!VULKAN_FAILED(Result))
    {
        const int64 Count = ActiveAllocationCount.Increment();
        if (static_cast<uint32>(Count) >= MaxAllocationCount)
        {
            VULKAN_ERROR("FVulkanMemoryManager: vkAllocateMemory count (%lld) has reached device limit (%u)", Count, MaxAllocationCount);
        }
        else if (static_cast<uint32>(Count) >= (MaxAllocationCount * 3) / 4)
        {
            VULKAN_WARNING("FVulkanMemoryManager: vkAllocateMemory count (%lld) is at 75%% of device limit (%u)", Count, MaxAllocationCount);
        }
    }
    return Result;
}

void FVulkanMemoryManager::FreeMemory(VkDeviceMemory Memory)
{
    if (Memory != VK_NULL_HANDLE)
    {
        const int64 Count = ActiveAllocationCount.Decrement();
        CHECK(Count >= 0);
        vkFreeMemory(GetDevice()->GetVkDevice(), Memory, nullptr);
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanMemoryManager::UpdateMemoryStats()
{
    BufferAllocator.UpdateMemoryStats();
    TextureAllocator.UpdateMemoryStats();
    UploadHeapAllocator.UpdateMemoryStats();
    STAT_SET(STAT_Vulkan_ActiveAllocations, ActiveAllocationCount.Load());
}
#endif

FVulkanBuddyAllocator::FVulkanBuddyAllocator(FVulkanDevice* InDevice, uint64 InBackingStorageSize, uint64 InMinBlockBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags)
    : FVulkanDeviceChild(InDevice)
    , BackingStorageSize(InBackingStorageSize)
    , MinBlockBytes(Math::Max<uint64>(InMinBlockBytes, VULKAN_MIN_BUDDY_ALLOCATOR_BLOCK_SIZE))
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , BufferUsageFlags(InBufferUsageFlags)
    , DeviceMemory(VK_NULL_HANDLE)
    , SharedBuffer(VK_NULL_HANDLE)
    , BaseDeviceAddress(0)
    , MappedBaseAddress(nullptr)
    , FreeOffsets()
    , AllocatorCS()
{
}

FVulkanBuddyAllocator::~FVulkanBuddyAllocator()
{
    Destroy();
}

bool FVulkanBuddyAllocator::Initialize()
{
    SCOPED_LOCK(AllocatorCS);

    Destroy();

    uint64 BackingSize = MinBlockBytes;
    while (BackingSize < BackingStorageSize)
    {
        BackingSize <<= 1;
    }

    BackingStorageSize = BackingSize;

    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = BackingStorageSize;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

    VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = AllocateFlags;

    AddToStructChain(AllocateInfo, AllocateFlagsInfo);

    VkResult Result = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanBuddyAllocator: vkAllocateMemory failed with %s (Size=%llu, MemoryTypeIndex=%u)", ToString(Result), BackingStorageSize, MemoryTypeIndex);
        return false;
    }

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    if (BufferUsageFlags != 0)
    {
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size        = BackingStorageSize;
        BufferCreateInfo.usage       = BufferUsageFlags;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Result = vkCreateBuffer(VulkanDevice, &BufferCreateInfo, nullptr, &SharedBuffer);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanBuddyAllocator: vkCreateBuffer failed for shared buffer");
            Destroy();
            return false;
        }

        Result = vkBindBufferMemory(VulkanDevice, SharedBuffer, DeviceMemory, 0);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanBuddyAllocator: vkBindBufferMemory failed for shared buffer");
            Destroy();
            return false;
        }

        if (AllocateFlags & VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo AddressInfo = {};
            AddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            AddressInfo.buffer = SharedBuffer;
            BaseDeviceAddress = vkGetBufferDeviceAddress(VulkanDevice, &AddressInfo);
        }
    }

    const VkPhysicalDeviceMemoryProperties& MemoryProperties = GetDevice()->GetPhysicalDevice()->GetMemoryProperties();
    const VkMemoryPropertyFlags PropertyFlags = MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags;
    if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* Mapped = nullptr;
        Result = vkMapMemory(VulkanDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, &Mapped);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanBuddyAllocator: vkMapMemory failed");
            Destroy();
            return false;
        }

        MappedBaseAddress = static_cast<uint8*>(Mapped);
    }

    const uint32 MaxOrder = GetOrderForSize(BackingStorageSize);
    FreeOffsets.Resize(MaxOrder + 1);
    FreeOffsets[MaxOrder].Add(0);

#if VULKAN_ENABLE_MEMORY_LOGGING
    if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanBuddyAllocator: Initialized (Size=%llu, MinBlock=%llu, MemoryTypeIndex=%u, HasSharedBuffer=%s)",
            BackingStorageSize, MinBlockBytes, MemoryTypeIndex, SharedBuffer != VK_NULL_HANDLE ? "true" : "false");
    }
#endif

    return true;
}

void FVulkanBuddyAllocator::Destroy()
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    if (SharedBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(VulkanDevice, SharedBuffer, nullptr);
        SharedBuffer = VK_NULL_HANDLE;
    }

    if (MappedBaseAddress)
    {
        vkUnmapMemory(VulkanDevice, DeviceMemory);
        MappedBaseAddress = nullptr;
    }

    if (DeviceMemory != VK_NULL_HANDLE)
    {
        GetDevice()->GetMemoryManager().FreeMemory(DeviceMemory);
        DeviceMemory = VK_NULL_HANDLE;
    }

    BaseDeviceAddress = 0;
    FreeOffsets.Clear();
}

uint32 FVulkanBuddyAllocator::GetOrderForSize(uint64 SizeInBytes) const
{
    uint32 Order     = 0;
    uint64 BlockSize = MinBlockBytes;

    while (BlockSize < SizeInBytes)
    {
        BlockSize <<= 1;
        ++Order;
    }

    return Order;
}

uint64 FVulkanBuddyAllocator::GetOrderBlockSize(uint32 Order) const
{
    return MinBlockBytes << Order;
}

bool FVulkanBuddyAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    if (SizeInBytes == 0 || FreeOffsets.IsEmpty())
    {
        return false;
    }

    const uint64 UsedAlignment  = Alignment ? Alignment : MinBlockBytes;
    const uint64 AllocationSize = Math::AlignUp<uint64>(Math::Max(SizeInBytes, UsedAlignment), MinBlockBytes);

    SCOPED_LOCK(AllocatorCS);

    uint64 Offset = 0;
    uint32 Order  = 0;

    {
        const uint64 RequestedSize = Math::Max(AllocationSize, UsedAlignment);
        const uint32 TargetOrder   = GetOrderForSize(RequestedSize);

        bool bAllocated = false;
        for (uint32 CurrentOrder = TargetOrder; CurrentOrder < static_cast<uint32>(FreeOffsets.Size()); ++CurrentOrder)
        {
            TArray<uint64>& CurrentList = FreeOffsets[CurrentOrder];
            if (CurrentList.IsEmpty())
            {
                continue;
            }

            const int32 LastIndex = CurrentList.Size() - 1;
            uint64 BlockOffset    = CurrentList[LastIndex];
            CurrentList.Pop();

            while (CurrentOrder > TargetOrder)
            {
                --CurrentOrder;
                const uint64 SplitBlockSize = GetOrderBlockSize(CurrentOrder);
                FreeOffsets[CurrentOrder].Add(BlockOffset + SplitBlockSize);
            }

            Offset     = BlockOffset;
            Order      = TargetOrder;
            bAllocated = true;
            break;
        }

        if (!bAllocated)
        {
            return false;
        }
    }

    const uint64 BlockSize = GetOrderBlockSize(Order);
#if VULKAN_ENABLE_STATS
    TrackedUsedBytes.Add(static_cast<int64>(BlockSize));
    TrackedWastedBytes.Add(static_cast<int64>(BlockSize - AllocationSize));
#endif

    OutLocation.Reset();
    OutLocation.SetSize(BlockSize);
    OutLocation.SetMemory(DeviceMemory);
    OutLocation.SetMemoryOffset(Offset);
    OutLocation.SetLocationType(EVulkanMemoryLocationType::Suballocated);

    if (SharedBuffer != VK_NULL_HANDLE)
    {
        OutLocation.SetBackingBuffer(SharedBuffer);
        OutLocation.SetBufferOffset(Offset);
        OutLocation.SetDeviceAddress(BaseDeviceAddress ? (BaseDeviceAddress + Offset) : 0);
    }

    OutLocation.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + Offset) : nullptr);

    FVulkanBuddyAllocatorAllocationData AllocationData = {};
    AllocationData.Order         = Order;
    AllocationData.Offset        = Offset;
    AllocationData.RequestedSize = AllocationSize;

    OutLocation.SetBuddyAllocationData(AllocationData);
    OutLocation.SetBuddyAllocator(this);
    return true;
}

void FVulkanBuddyAllocator::Deallocate(const FVulkanMemoryLocation& Location)
{
    FVulkanDeviceRHI::DeferDeletion(this, Location.GetBuddyAllocationData());
}

void FVulkanBuddyAllocator::RecycleAllocation(const FVulkanBuddyAllocatorAllocationData& AllocationData)
{
#if VULKAN_ENABLE_STATS
    {
        const uint64 BlockSize = GetOrderBlockSize(AllocationData.Order);
        TrackedUsedBytes.Subtract(static_cast<int64>(BlockSize));
        TrackedWastedBytes.Subtract(static_cast<int64>(BlockSize - AllocationData.RequestedSize));
    }
#endif

    SCOPED_LOCK(AllocatorCS);

    if (FreeOffsets.IsEmpty())
    {
        return;
    }

    const uint32 MaxOrder = static_cast<uint32>(FreeOffsets.Size() - 1);

    uint64 CurrentOffset = AllocationData.Offset;
    uint32 CurrentOrder  = AllocationData.Order;

    while (CurrentOrder < MaxOrder)
    {
        const uint64 BlockSize   = GetOrderBlockSize(CurrentOrder);
        const uint64 BuddyOffset = CurrentOffset ^ BlockSize;
        TArray<uint64>& FreeList = FreeOffsets[CurrentOrder];

        int32 BuddyIndex = -1;
        for (int32 Index = 0; Index < FreeList.Size(); ++Index)
        {
            if (FreeList[Index] == BuddyOffset)
            {
                BuddyIndex = Index;
                break;
            }
        }

        if (BuddyIndex < 0)
        {
            break;
        }

        FreeList.RemoveAtSwap(BuddyIndex);
        CurrentOffset = Math::Min(CurrentOffset, BuddyOffset);
        ++CurrentOrder;
    }

    FreeOffsets[CurrentOrder].Add(CurrentOffset);
}

bool FVulkanBuddyAllocator::IsEmpty() const
{
    SCOPED_LOCK(AllocatorCS);

    if (FreeOffsets.IsEmpty())
    {
        return true;
    }

    const uint32 MaxOrder = static_cast<uint32>(FreeOffsets.Size() - 1);
    return FreeOffsets[MaxOrder].Size() == 1 && FreeOffsets[MaxOrder][0] == 0;
}

#if VULKAN_ENABLE_STATS
void FVulkanBuddyAllocator::UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const
{
    OutUsage.AllocatedBytes  += BackingStorageSize;
    OutUsage.UsedBytes       += static_cast<uint64>(TrackedUsedBytes.Load());
    OutUsage.FragmentedBytes += static_cast<uint64>(TrackedWastedBytes.Load());
}
#endif

FVulkanMultiBuddyAllocator::FVulkanMultiBuddyAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , BufferUsageFlags(InBufferUsageFlags)
    , Allocators()
    , AllocatorsCS()
{
}

FVulkanMultiBuddyAllocator::~FVulkanMultiBuddyAllocator()
{
    Destroy();
}

bool FVulkanMultiBuddyAllocator::Initialize()
{
    Destroy();
    return true;
}

void FVulkanMultiBuddyAllocator::Destroy()
{
    SCOPED_LOCK(AllocatorsCS);

    for (FVulkanBuddyAllocator* Allocator : Allocators)
    {
        delete Allocator;
    }

    Allocators.Clear();
}

bool FVulkanMultiBuddyAllocator::CreateAllocator()
{
    FVulkanBuddyAllocator* Allocator = new FVulkanBuddyAllocator(GetDevice(), PageSizeBytes, MinBlockBytes, MemoryTypeIndex, AllocateFlags, BufferUsageFlags);
    if (!Allocator->Initialize())
    {
        delete Allocator;
        return false;
    }

    Allocators.Add(Allocator);
    return true;
}

void FVulkanMultiBuddyAllocator::CleanUp()
{
    SCOPED_LOCK(AllocatorsCS);

    for (int32 Index = Allocators.Size() - 1; Index >= 0; --Index)
    {
        FVulkanBuddyAllocator* Allocator = Allocators[Index];
        if (Allocator && Allocator->IsEmpty())
        {
            delete Allocator;
            Allocators.RemoveAtSwap(Index);
        }
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanMultiBuddyAllocator::UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const
{
    SCOPED_LOCK(AllocatorsCS);

    for (const FVulkanBuddyAllocator* Allocator : Allocators)
    {
        if (Allocator)
        {
            Allocator->UpdateMemoryStats(OutUsage);
        }
    }
}
#endif

bool FVulkanMultiBuddyAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    SCOPED_LOCK(AllocatorsCS);

    for (FVulkanBuddyAllocator* Allocator : Allocators)
    {
        if (Allocator && Allocator->TryAllocate(SizeInBytes, Alignment, OutLocation))
        {
            return true;
        }
    }

    if (!CreateAllocator())
    {
        return false;
    }

    FVulkanBuddyAllocator* Allocator = Allocators[Allocators.Size() - 1];
    return Allocator && Allocator->TryAllocate(SizeInBytes, Alignment, OutLocation);
}

FVulkanPoolAllocatorPage::FVulkanPoolAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , UsedBytes(0)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , BufferUsageFlags(InBufferUsageFlags)
    , DeviceMemory(VK_NULL_HANDLE)
    , SharedBuffer(VK_NULL_HANDLE)
    , MappedBaseAddress(nullptr)
    , FreeRanges()
{
}

FVulkanPoolAllocatorPage::~FVulkanPoolAllocatorPage()
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    if (MappedBaseAddress)
    {
        vkUnmapMemory(VulkanDevice, DeviceMemory);
        MappedBaseAddress = nullptr;
    }

    if (VULKAN_CHECK_HANDLE(SharedBuffer))
    {
        vkDestroyBuffer(VulkanDevice, SharedBuffer, nullptr);
        SharedBuffer = VK_NULL_HANDLE;
    }

    if (DeviceMemory != VK_NULL_HANDLE)
    {
        GetDevice()->GetMemoryManager().FreeMemory(DeviceMemory);
        DeviceMemory = VK_NULL_HANDLE;
    }
}

bool FVulkanPoolAllocatorPage::Initialize()
{
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = PageSizeBytes;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

    VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = AllocateFlags;

    AddToStructChain(AllocateInfo, AllocateFlagsInfo);

    VkResult Result = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanPoolAllocatorPage: vkAllocateMemory failed with %s (Size=%llu, MemoryTypeIndex=%u)", ToString(Result), PageSizeBytes, MemoryTypeIndex);
        return false;
    }

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    if (BufferUsageFlags != 0)
    {
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size        = PageSizeBytes;
        BufferCreateInfo.usage       = BufferUsageFlags;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Result = vkCreateBuffer(VulkanDevice, &BufferCreateInfo, nullptr, &SharedBuffer);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanPoolAllocatorPage: vkCreateBuffer failed for shared buffer");
            return false;
        }

        Result = vkBindBufferMemory(VulkanDevice, SharedBuffer, DeviceMemory, 0);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanPoolAllocatorPage: vkBindBufferMemory failed for shared buffer");
            return false;
        }
    }

    const VkPhysicalDeviceMemoryProperties& MemoryProperties = GetDevice()->GetPhysicalDevice()->GetMemoryProperties();
    const VkMemoryPropertyFlags PropertyFlags = MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags;
    if (PropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        void* Mapped = nullptr;
        Result = vkMapMemory(VulkanDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, &Mapped);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanPoolAllocatorPage: vkMapMemory failed");
            return false;
        }

        MappedBaseAddress = static_cast<uint8*>(Mapped);
    }

    UsedBytes = 0;
    FreeRanges.Clear();

    FFreeRange InitialRange;
    InitialRange.Offset = 0;
    InitialRange.Size   = PageSizeBytes;
    FreeRanges.Add(InitialRange);
    return true;
}

bool FVulkanPoolAllocatorPage::TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FVulkanMemoryLocation& OutLocation)
{
    for (int32 Index = 0; Index < FreeRanges.Size(); ++Index)
    {
        const FFreeRange Range = FreeRanges[Index];

        const uint64 UsedAlignment = Math::Max<uint64>(InAlignment, Alignment);
        const uint64 AlignedOffset = Math::AlignUp<uint64>(Range.Offset, UsedAlignment);
        const uint64 Padding       = AlignedOffset - Range.Offset;
        const uint64 RequiredSize  = Padding + SizeInBytes;

        if (Range.Size < RequiredSize)
        {
            continue;
        }

        const uint64 TailSize = Range.Size - RequiredSize;
        if (Padding > 0)
        {
            FreeRanges[Index].Size = Padding;

            if (TailSize > 0)
            {
                FFreeRange TailRange;
                TailRange.Offset = AlignedOffset + SizeInBytes;
                TailRange.Size   = TailSize;
                FreeRanges.Add(TailRange);
            }
        }
        else if (TailSize > 0)
        {
            FreeRanges[Index].Offset = AlignedOffset + SizeInBytes;
            FreeRanges[Index].Size   = TailSize;
        }
        else
        {
            FreeRanges.RemoveAtSwap(Index);
        }

        UsedBytes += SizeInBytes;

        OutLocation.Reset();
        OutLocation.SetSize(SizeInBytes);
        OutLocation.SetMemory(DeviceMemory);
        OutLocation.SetMemoryOffset(AlignedOffset);
        OutLocation.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + AlignedOffset) : nullptr);
        OutLocation.SetLocationType(EVulkanMemoryLocationType::Suballocated);

        if (VULKAN_CHECK_HANDLE(SharedBuffer))
        {
            OutLocation.SetBackingBuffer(SharedBuffer);
            OutLocation.SetBufferOffset(AlignedOffset);
        }

        FVulkanPoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex = InPageIndex;
        AllocationData.Offset    = AlignedOffset;
        AllocationData.Size      = SizeInBytes;
        AllocationData.Owner     = &OutLocation;

        OutLocation.SetPoolAllocationData(AllocationData);
        LiveAllocations.Add(AllocationData);
        return true;
    }

    return false;
}

bool FVulkanPoolAllocatorPage::TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, FVulkanPoolAllocatorAllocationData& OutData)
{
    for (int32 Index = 0; Index < FreeRanges.Size(); ++Index)
    {
        const FFreeRange Range = FreeRanges[Index];

        const uint64 UsedAlignment = Math::Max<uint64>(InAlignment, Alignment);
        const uint64 AlignedOffset = Math::AlignUp<uint64>(Range.Offset, UsedAlignment);
        const uint64 Padding       = AlignedOffset - Range.Offset;
        const uint64 RequiredSize  = Padding + SizeInBytes;

        if (Range.Size < RequiredSize)
        {
            continue;
        }

        const uint64 TailSize = Range.Size - RequiredSize;
        if (Padding > 0)
        {
            FreeRanges[Index].Size = Padding;
            if (TailSize > 0)
            {
                FFreeRange TailRange;
                TailRange.Offset = AlignedOffset + SizeInBytes;
                TailRange.Size   = TailSize;
                FreeRanges.Add(TailRange);
            }
        }
        else if (TailSize > 0)
        {
            FreeRanges[Index].Offset = AlignedOffset + SizeInBytes;
            FreeRanges[Index].Size   = TailSize;
        }
        else
        {
            FreeRanges.RemoveAtSwap(Index);
        }

        UsedBytes += SizeInBytes;

        OutData.Offset = AlignedOffset;
        OutData.Size   = SizeInBytes;

        LiveAllocations.Add(OutData);
        return true;
    }

    return false;
}

void FVulkanPoolAllocatorPage::TransferOwnership(uint64 Offset, FVulkanMemoryLocation* NewLocation)
{
    for (FVulkanPoolAllocatorAllocationData& Alloc : LiveAllocations)
    {
        if (Alloc.Offset == Offset)
        {
            Alloc.Owner = NewLocation;
            return;
        }
    }
}

void FVulkanPoolAllocatorPage::RecycleAllocation(uint64 Offset, uint64 SizeInBytes)
{
    if (SizeInBytes == 0)
    {
        return;
    }

    FFreeRange NewRange;
    NewRange.Offset = Offset;
    NewRange.Size   = SizeInBytes;
    FreeRanges.Add(NewRange);

    for (int32 Index = 0; Index < LiveAllocations.Size(); ++Index)
    {
        if (LiveAllocations[Index].Offset == Offset)
        {
            LiveAllocations.RemoveAtSwap(Index);
            break;
        }
    }

    UsedBytes = UsedBytes > SizeInBytes ? (UsedBytes - SizeInBytes) : 0;
    CoalesceFreeRanges();
}

void FVulkanPoolAllocatorPage::CoalesceFreeRanges()
{
    if (FreeRanges.IsEmpty())
    {
        return;
    }

    FreeRanges.SortWithPredicate([](const FFreeRange& A, const FFreeRange& B)
    {
        return A.Offset < B.Offset;
    });

    for (int32 Index = 0; Index + 1 < FreeRanges.Size();)
    {
        FFreeRange& Current = FreeRanges[Index];
        FFreeRange& Next    = FreeRanges[Index + 1];

        if (Current.Offset + Current.Size == Next.Offset)
        {
            Current.Size += Next.Size;
            FreeRanges.RemoveAt(Index + 1);
        }
        else
        {
            ++Index;
        }
    }
}

FVulkanPoolAllocator::FVulkanPoolAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxAllocationSize, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , MaxAllocationSize(InMaxAllocationSize)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , BufferUsageFlags(InBufferUsageFlags)
    , FragmentedBytes(0)
    , Pages()
    , PagesCS()
{
}

FVulkanPoolAllocator::~FVulkanPoolAllocator()
{
    Destroy();
}

bool FVulkanPoolAllocator::Initialize()
{
    Destroy();
    return true;
}

void FVulkanPoolAllocator::Destroy()
{
    SCOPED_LOCK(PagesCS);

    for (FVulkanPoolAllocatorPage* Page : Pages)
    {
        delete Page;
    }

    Pages.Clear();
    FragmentedBytes = 0;
}

void FVulkanPoolAllocator::CleanUp()
{
    SCOPED_LOCK(PagesCS);

    for (int32 Index = Pages.Size() - 1; Index >= 0; --Index)
    {
        FVulkanPoolAllocatorPage* Page = Pages[Index];
        if (Page && Page->IsEmpty())
        {
        #if VULKAN_ENABLE_STATS
            TrackedAllocatedBytes.Subtract(static_cast<int64>(Page->GetPageSize()));
        #endif

            delete Page;
            Pages[Index] = nullptr;
        }
    }

    while (!Pages.IsEmpty() && Pages[Pages.Size() - 1] == nullptr)
    {
        Pages.Pop();
    }
}

FVulkanPoolAllocatorPage* FVulkanPoolAllocator::CreatePage(uint64 MinimumSize, uint32& OutPageIndex)
{
    OutPageIndex = UINT32_MAX;

    const uint64 RequiredSize   = Math::AlignUp<uint64>(MinimumSize, Alignment);
    const uint64 ActualPageSize = Math::Max(PageSizeBytes, RequiredSize);

    FVulkanPoolAllocatorPage* NewPage = new FVulkanPoolAllocatorPage(GetDevice(), ActualPageSize, Alignment, MemoryTypeIndex, AllocateFlags, BufferUsageFlags);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

#if VULKAN_ENABLE_STATS
    TrackedAllocatedBytes.Add(static_cast<int64>(ActualPageSize));
#endif

    OutPageIndex = static_cast<uint32>(Pages.Size());
    Pages.Add(NewPage);
    return NewPage;
}

void FVulkanPoolAllocator::RebuildFragmentationData()
{
    FragmentedBytes = 0;

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        const FVulkanPoolAllocatorPage* CurrentPage = Pages[PageIndex];
        if (!CurrentPage || CurrentPage->GetFreeRanges().Size() <= 1)
        {
            continue;
        }

        for (const FVulkanPoolAllocatorPage::FFreeRange& Range : CurrentPage->GetFreeRanges())
        {
            FragmentedBytes += Range.Size;
        }
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanPoolAllocator::UpdateMemoryStats(FVulkanAllocatorUsage& OutUsage) const
{
    OutUsage.AllocatedBytes  += static_cast<uint64>(TrackedAllocatedBytes.Load());
    OutUsage.UsedBytes       += static_cast<uint64>(TrackedUsedBytes.Load());
    OutUsage.FragmentedBytes += FragmentedBytes;
}
#endif

bool FVulkanPoolAllocator::TryAllocate(uint64 SizeInBytes, uint64 InAlignment, FVulkanMemoryLocation& OutLocation)
{
    if (SizeInBytes == 0)
    {
        return false;
    }

    const uint64 UsedAlignment = InAlignment ? InAlignment : Alignment;
    const uint64 SizeAligned   = Math::AlignUp<uint64>(SizeInBytes, UsedAlignment);

    if (SizeAligned > MaxAllocationSize)
    {
        return false;
    }

    SCOPED_LOCK(PagesCS);

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        FVulkanPoolAllocatorPage* Page = Pages[PageIndex];
        if (Page && Page->TryAllocate(SizeAligned, UsedAlignment, PageIndex, OutLocation))
        {
        #if VULKAN_ENABLE_STATS
            TrackedUsedBytes.Add(static_cast<int64>(SizeAligned));
        #endif

            OutLocation.SetPoolAllocator(this);
            return true;
        }
    }

    uint32 NewPageIndex = UINT32_MAX;
    FVulkanPoolAllocatorPage* NewPage = CreatePage(SizeAligned, NewPageIndex);
    if (!NewPage)
    {
        return false;
    }

    if (!NewPage->TryAllocate(SizeAligned, UsedAlignment, NewPageIndex, OutLocation))
    {
        return false;
    }

#if VULKAN_ENABLE_STATS
    TrackedUsedBytes.Add(static_cast<int64>(SizeAligned));
#endif

    OutLocation.SetPoolAllocator(this);
    return true;
}

bool FVulkanPoolAllocator::TryAllocateForDefrag(uint64 SizeInBytes, uint64 InAlignment, uint32 ExcludePageIndex, FVulkanPoolAllocatorAllocationData& OutData)
{
    SCOPED_LOCK(PagesCS);

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        if (PageIndex == ExcludePageIndex)
        {
            continue;
        }

        FVulkanPoolAllocatorPage* Page = Pages[PageIndex];
        if (!Page)
        {
            continue;
        }

        if (Page->TryAllocateForDefrag(SizeInBytes, InAlignment, OutData))
        {
            OutData.PageIndex = PageIndex;
            return true;
        }
    }

    return false;
}

void FVulkanPoolAllocator::Deallocate(const FVulkanMemoryLocation& Location)
{
    FVulkanDeviceRHI::DeferDeletion(this, Location.GetPoolAllocationData());
}

void FVulkanPoolAllocator::RecycleAllocation(const FVulkanPoolAllocatorAllocationData& AllocationData)
{
    if (AllocationData.PageIndex == UINT32_MAX || AllocationData.Size == 0)
    {
        return;
    }

#if VULKAN_ENABLE_STATS
    TrackedUsedBytes.Subtract(static_cast<int64>(AllocationData.Size));
#endif

    SCOPED_LOCK(PagesCS);

    if (AllocationData.PageIndex >= static_cast<uint32>(Pages.Size()))
    {
        return;
    }

    FVulkanPoolAllocatorPage* Page = Pages[AllocationData.PageIndex];
    if (!Page)
    {
        return;
    }

    Page->RecycleAllocation(AllocationData.Offset, AllocationData.Size);
    RebuildFragmentationData();
}

bool FVulkanPoolAllocator::GetDefragCandidate(FVulkanPoolAllocatorAllocationData& OutCandidate) const
{
    SCOPED_LOCK(PagesCS);

    uint32 BestPageIndex     = UINT32_MAX;
    uint64 LowestUtilization = UINT64_MAX;

    for (uint32 PageIndex = 0; PageIndex < static_cast<uint32>(Pages.Size()); ++PageIndex)
    {
        const FVulkanPoolAllocatorPage* Page = Pages[PageIndex];
        if (!Page || Page->IsEmpty() || Page->GetFreeRanges().Size() <= 1)
        {
            continue;
        }

        const uint64 Utilization = Page->GetUsedBytes();
        if (Utilization < LowestUtilization)
        {
            LowestUtilization = Utilization;
            BestPageIndex     = PageIndex;
        }
    }

    if (BestPageIndex == UINT32_MAX)
    {
        return false;
    }

    const FVulkanPoolAllocatorPage* SourcePage = Pages[BestPageIndex];
    for (const FVulkanPoolAllocatorAllocationData& LiveAlloc : SourcePage->GetLiveAllocations())
    {
        if (LiveAlloc.Owner)
        {
            OutCandidate = LiveAlloc;
            return true;
        }
    }

    return false;
}

void FVulkanPoolAllocator::TransferOwnership(const FVulkanPoolAllocatorAllocationData& Data, FVulkanMemoryLocation* NewLocation)
{
    if (Data.PageIndex == UINT32_MAX)
    {
        return;
    }

    SCOPED_LOCK(PagesCS);

    if (Data.PageIndex < static_cast<uint32>(Pages.Size()) && Pages[Data.PageIndex])
    {
        Pages[Data.PageIndex]->TransferOwnership(Data.Offset, NewLocation);
    }
}

VkDeviceMemory FVulkanPoolAllocator::GetBackingMemory(uint32 PageIndex)
{
    SCOPED_LOCK(PagesCS);

    if (PageIndex >= static_cast<uint32>(Pages.Size()))
    {
        return VK_NULL_HANDLE;
    }

    FVulkanPoolAllocatorPage* Page = Pages[PageIndex];
    return Page ? Page->GetDeviceMemory() : VK_NULL_HANDLE;
}

FVulkanLinearAllocatorPage::FVulkanLinearAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, VkMemoryPropertyFlags InMemoryProperties, VkBufferUsageFlags InBufferUsageFlags, VkMemoryAllocateFlags InAllocateFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MemoryProperties(InMemoryProperties)
    , BufferUsageFlags(InBufferUsageFlags)
    , AllocateFlags(InAllocateFlags)
    , BackingLocation(InDevice)
{
}

bool FVulkanLinearAllocatorPage::Initialize()
{
    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        return MemoryManager.AllocateUploadMemory(PageSizeBytes, 16, BufferUsageFlags, BackingLocation) != nullptr;
    }

    return MemoryManager.AllocateBufferMemory(MemoryProperties, BufferUsageFlags, AllocateFlags, PageSizeBytes, 16, BackingLocation);
}

FVulkanLinearAllocator::FVulkanLinearAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, VkMemoryPropertyFlags InMemoryProperties, VkBufferUsageFlags InBufferUsageFlags, VkMemoryAllocateFlags InAllocateFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MemoryProperties(InMemoryProperties)
    , BufferUsageFlags(InBufferUsageFlags)
    , AllocateFlags(InAllocateFlags)
    , CurrentPage(nullptr)
    , CurrentOffset(0)
    , PagePool()
    , AllocatorCS()
{
}

FVulkanLinearAllocator::~FVulkanLinearAllocator()
{
    delete CurrentPage;
    CurrentPage = nullptr;

    for (FVulkanLinearAllocatorPage* Page : PagePool)
    {
        delete Page;
    }

    PagePool.Clear();
}

FVulkanLinearAllocatorPage* FVulkanLinearAllocator::CreatePage()
{
    FVulkanLinearAllocatorPage* NewPage = new FVulkanLinearAllocatorPage(GetDevice(), PageSizeBytes, MemoryProperties, BufferUsageFlags, AllocateFlags);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

    return NewPage;
}

FVulkanLinearAllocatorPage* FVulkanLinearAllocator::AcquirePage()
{
    if (!PagePool.IsEmpty())
    {
        FVulkanLinearAllocatorPage* Page = PagePool.LastElement();
        PagePool.Pop();
        return Page;
    }

    return CreatePage();
}

void* FVulkanLinearAllocator::AllocateOversized(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    const uint64 AlignedSize = Math::AlignUp<uint64>(SizeInBytes, Math::Max<uint64>(Alignment, 16ull));

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = AlignedSize;
    BufferCreateInfo.usage       = BufferUsageFlags;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDeviceBufferMemoryRequirements DeviceBufferMemReqInfo = {};
    DeviceBufferMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
    DeviceBufferMemReqInfo.pCreateInfo = &BufferCreateInfo;

    VkMemoryRequirements2 MemReqs2 = {};
    MemReqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetDeviceBufferMemoryRequirements(VulkanDevice, &DeviceBufferMemReqInfo, &MemReqs2);

    const VkMemoryRequirements& MemReqs = MemReqs2.memoryRequirements;

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemReqs.memoryTypeBits, MemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanLinearAllocator: No suitable memory type for oversized allocation");
        return nullptr;
    }

    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = MemReqs.size;
    AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

    VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = AllocateFlags;

    AddToStructChain(AllocateInfo, AllocateFlagsInfo);

    VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
    VkResult Result = MemoryManager.AllocateMemory(&AllocateInfo, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanLinearAllocator: Oversized vkAllocateMemory failed with %s (Size=%llu)", ToString(Result), MemReqs.size);
        return nullptr;
    }

    VkBuffer Buffer = VK_NULL_HANDLE;
    Result = vkCreateBuffer(VulkanDevice, &BufferCreateInfo, nullptr, &Buffer);
    if (VULKAN_FAILED(Result))
    {
        MemoryManager.FreeMemory(DeviceMemory);
        VULKAN_ERROR_CRITICAL("FVulkanLinearAllocator: Oversized vkCreateBuffer failed (Size=%llu)", AlignedSize);
        return nullptr;
    }

    Result = vkBindBufferMemory(VulkanDevice, Buffer, DeviceMemory, 0);
    if (VULKAN_FAILED(Result))
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        MemoryManager.FreeMemory(DeviceMemory);
        VULKAN_ERROR_CRITICAL("FVulkanLinearAllocator: Oversized vkBindBufferMemory failed");
        return nullptr;
    }

    OutLocation.Reset();
    OutLocation.SetMemory(DeviceMemory);
    OutLocation.SetMemoryOffset(0);
    OutLocation.SetBackingBuffer(Buffer);
    OutLocation.SetBufferOffset(0);
    OutLocation.SetSize(MemReqs.size);
    OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);

    void* MappedMemory = nullptr;
    if (MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        Result = vkMapMemory(VulkanDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, &MappedMemory);
        if (VULKAN_FAILED(Result))
        {
            vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
            MemoryManager.FreeMemory(DeviceMemory);
            VULKAN_ERROR_CRITICAL("FVulkanLinearAllocator: Oversized vkMapMemory failed");
            return nullptr;
        }

        OutLocation.SetMappedBaseAddress(MappedMemory);
    }

    return MappedMemory ? MappedMemory : reinterpret_cast<void*>(1);
}

void* FVulkanLinearAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    if (SizeInBytes == 0)
    {
        return nullptr;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 SizeAligned   = Math::AlignUp<uint64>(SizeInBytes, UsedAlignment);

    if (SizeAligned > PageSizeBytes)
    {
        return AllocateOversized(SizeAligned, UsedAlignment, OutLocation);
    }

    SCOPED_LOCK(AllocatorCS);

    const uint64 AlignedOffset = CurrentPage ? Math::AlignUp<uint64>(CurrentOffset, UsedAlignment) : PageSizeBytes;
    if (AlignedOffset + SizeAligned > PageSizeBytes)
    {
        if (CurrentPage)
        {
            FVulkanDeviceRHI::DeferDeletion(this, CurrentPage);
            CurrentPage = nullptr;
        }

        CurrentPage = AcquirePage();
        if (!CurrentPage)
        {
            return nullptr;
        }

        CurrentOffset = 0;
    }

    const uint64 AllocationOffset = Math::AlignUp<uint64>(CurrentOffset, UsedAlignment);

    const FVulkanMemoryLocation& BackingLocation = CurrentPage->GetBackingLocation();
    OutLocation.Reset();
    OutLocation.SetMemory(BackingLocation.GetMemory());
    OutLocation.SetMemoryOffset(BackingLocation.GetMemoryOffset() + AllocationOffset);
    OutLocation.SetSize(SizeAligned);
    OutLocation.SetLocationType(EVulkanMemoryLocationType::Suballocated);

    if (BackingLocation.GetBackingBuffer() != VK_NULL_HANDLE)
    {
        OutLocation.SetBackingBuffer(BackingLocation.GetBackingBuffer());
        OutLocation.SetBufferOffset(BackingLocation.GetBufferOffset() + AllocationOffset);
    }

    uint8* MappedBase = static_cast<uint8*>(BackingLocation.GetMappedBaseAddress());
    OutLocation.SetMappedBaseAddress(MappedBase ? (MappedBase + AllocationOffset) : nullptr);

    CurrentOffset = AllocationOffset + SizeAligned;
    return OutLocation.GetMappedBaseAddress();
}

void FVulkanLinearAllocator::ReturnPage(FVulkanLinearAllocatorPage* InPage)
{
    CHECK(InPage != nullptr);

    SCOPED_LOCK(AllocatorCS);
    PagePool.Add(InPage);
}

void FVulkanLinearAllocator::CleanUp()
{
    SCOPED_LOCK(AllocatorCS);

    while (PagePool.Size() > MAX_POOL_PAGES)
    {
        delete PagePool.LastElement();
        PagePool.Pop();
    }
}

#if !VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR

FVulkanBufferAllocatorPool::FVulkanBufferAllocatorPool(FVulkanDevice* InDevice, uint32 InMemoryTypeIndex, VkBufferUsageFlags InBufferUsageFlags, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize, VkMemoryAllocateFlags InAllocateFlags)
    : FVulkanDeviceChild(InDevice)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , BufferUsageFlags(InBufferUsageFlags)
    , AllocateFlags(InAllocateFlags)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
    , MultiBuddyAllocator(InDevice, InPageSizeBytes, InMinBlockBytes, InMemoryTypeIndex, InAllocateFlags, InBufferUsageFlags)
{
}

FVulkanBufferAllocatorPool::~FVulkanBufferAllocatorPool()
{
    Destroy();
}

bool FVulkanBufferAllocatorPool::Initialize()
{
    Destroy();
    return MultiBuddyAllocator.Initialize();
}

void FVulkanBufferAllocatorPool::CleanUp()
{
    MultiBuddyAllocator.CleanUp();
}

void FVulkanBufferAllocatorPool::Destroy()
{
    MultiBuddyAllocator.Destroy();
}

bool FVulkanBufferAllocatorPool::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    if (SizeInBytes > MaxSuballocationSize)
    {
        return false;
    }

    return MultiBuddyAllocator.TryAllocate(SizeInBytes, Alignment, OutLocation);
}

FVulkanBufferAllocator::FVulkanBufferAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InMinBlockBytes, uint64 InMaxSuballocationSize)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MinBlockBytes(InMinBlockBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
    , Pools()
    , PoolsCS()
{
}

FVulkanBufferAllocator::~FVulkanBufferAllocator()
{
    Destroy();
}

bool FVulkanBufferAllocator::Initialize()
{
    Destroy();
    return true;
}

void FVulkanBufferAllocator::Destroy()
{
    ReleasePools();
}

void FVulkanBufferAllocator::CleanUp()
{
    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (Pool)
        {
            Pool->CleanUp();
        }
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanBufferAllocator::UpdateMemoryStats()
{
    SCOPED_LOCK(PoolsCS);

    FVulkanAllocatorUsage Usage;
    for (const FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (Pool)
        {
            Pool->UpdateMemoryStats(Usage);
        }
    }

    STAT_SET(STAT_Vulkan_BufferPoolAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Vulkan_BufferPoolUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Vulkan_BufferPoolFragmented, Usage.FragmentedBytes);
}
#endif

void FVulkanBufferAllocator::ReleasePools()
{
    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        delete Pool;
    }

    Pools.Clear();
}

bool FVulkanBufferAllocator::TryAllocate(VkMemoryPropertyFlags MemoryProperties, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = SizeInBytes;
    BufferCreateInfo.usage       = UsageFlags;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkDeviceBufferMemoryRequirements DeviceBufferMemReqInfo = {};
    DeviceBufferMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
    DeviceBufferMemReqInfo.pCreateInfo = &BufferCreateInfo;

    VkMemoryRequirements2 MemoryRequirements2 = {};
    MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetDeviceBufferMemoryRequirements(VulkanDevice, &DeviceBufferMemReqInfo, &MemoryRequirements2);

    const VkMemoryRequirements& MemoryRequirements = MemoryRequirements2.memoryRequirements;

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: No suitable memory type for buffer");
        return false;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, MemoryRequirements.alignment);

    if (SizeInBytes > MaxSuballocationSize)
    {
        VkMemoryAllocateInfo AllocateInfo = {};
        AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize  = MemoryRequirements.size;
        AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

        VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
        AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        AllocateFlagsInfo.flags = AllocateFlags;

        AddToStructChain(AllocateInfo, AllocateFlagsInfo);

        VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
        VkResult DedicatedResult = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DedicatedMemory);
        if (VULKAN_FAILED(DedicatedResult))
        {
            VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: Dedicated vkAllocateMemory failed with %s (Size=%llu)", ToString(DedicatedResult), MemoryRequirements.size);
            return false;
        }

        OutLocation.Reset();
        OutLocation.SetMemory(DedicatedMemory);
        OutLocation.SetMemoryOffset(0);
        OutLocation.SetSize(MemoryRequirements.size);
        OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);

    #if VULKAN_ENABLE_MEMORY_LOGGING
        if (CVarVulkanLogMemoryAllocations.GetValue())
        {
            VULKAN_INFO("FVulkanBufferAllocator: Using dedicated allocation for buffer (Size=%llu)", MemoryRequirements.size);
        }
    #endif

        return true;
    }

    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (!Pool)
        {
            continue;
        }

        if (Pool->GetMemoryTypeIndex() != static_cast<uint32>(MemoryTypeIndex) || Pool->GetBufferUsageFlags() != UsageFlags)
        {
            continue;
        }

        if (Pool->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
        {
            return true;
        }
    }

    FVulkanBufferAllocatorPool* NewPool = new FVulkanBufferAllocatorPool(GetDevice(), static_cast<uint32>(MemoryTypeIndex), UsageFlags, PageSizeBytes, MinBlockBytes, MaxSuballocationSize, AllocateFlags);
    if (!NewPool->Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: Failed to create new pool (Size=%llu, PageSize=%llu, MaxSuballocation=%llu, MemoryTypeIndex=%d)", SizeInBytes, PageSizeBytes, MaxSuballocationSize, MemoryTypeIndex);
        delete NewPool;
        return false;
    }

    Pools.Add(NewPool);

    if (!NewPool->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: Allocation failed after creating new pool (Size=%llu, PageSize=%llu, MaxSuballocation=%llu)", SizeInBytes, PageSizeBytes, MaxSuballocationSize);
        return false;
    }

    return true;
}

#else // VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR

FVulkanBufferAllocatorPool::FVulkanBufferAllocatorPool(FVulkanDevice* InDevice, uint32 InMemoryTypeIndex, VkBufferUsageFlags InBufferUsageFlags, uint64 InPageSizeBytes, uint64 InMaxSuballocationSize, VkMemoryAllocateFlags InAllocateFlags)
    : FVulkanDeviceChild(InDevice)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , BufferUsageFlags(InBufferUsageFlags)
    , AllocateFlags(InAllocateFlags)
    , PageSizeBytes(InPageSizeBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
    , PoolAllocator(InDevice, InPageSizeBytes, 16ull, InMaxSuballocationSize, InMemoryTypeIndex, InAllocateFlags, InBufferUsageFlags)
{
}

FVulkanBufferAllocatorPool::~FVulkanBufferAllocatorPool()
{
    Destroy();
}

bool FVulkanBufferAllocatorPool::Initialize()
{
    Destroy();
    return PoolAllocator.Initialize();
}

void FVulkanBufferAllocatorPool::CleanUp()
{
    PoolAllocator.CleanUp();
}

void FVulkanBufferAllocatorPool::Destroy()
{
    PoolAllocator.Destroy();
}

bool FVulkanBufferAllocatorPool::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    if (SizeInBytes > MaxSuballocationSize)
    {
        return false;
    }

    return PoolAllocator.TryAllocate(SizeInBytes, Alignment, OutLocation);
}

bool FVulkanBufferAllocatorPool::GetDefragCandidate(FVulkanPoolAllocatorAllocationData& OutCandidate)
{
    return PoolAllocator.GetDefragCandidate(OutCandidate);
}

bool FVulkanBufferAllocatorPool::TryAllocateForDefrag(uint64 SizeInBytes, uint64 Alignment, uint32 ExcludePageIndex, FVulkanPoolAllocatorAllocationData& OutData)
{
    return PoolAllocator.TryAllocateForDefrag(SizeInBytes, Alignment, ExcludePageIndex, OutData);
}

void FVulkanBufferAllocatorPool::TransferOwnership(const FVulkanPoolAllocatorAllocationData& Data, FVulkanMemoryLocation* NewLocation)
{
    PoolAllocator.TransferOwnership(Data, NewLocation);
}

VkDeviceMemory FVulkanBufferAllocatorPool::GetBackingMemory(uint32 PageIndex)
{
    return PoolAllocator.GetBackingMemory(PageIndex);
}

FVulkanBufferAllocator::FVulkanBufferAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 /*InMinBlockBytes*/, uint64 InMaxSuballocationSize)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MaxSuballocationSize(InMaxSuballocationSize)
    , Pools()
    , PendingDefragMoves()
    , PoolsCS()
{
}

FVulkanBufferAllocator::~FVulkanBufferAllocator()
{
    Destroy();
}

bool FVulkanBufferAllocator::Initialize()
{
    Destroy();
    return true;
}

void FVulkanBufferAllocator::Destroy()
{
    PendingDefragMoves.Clear();
    ReleasePools();
}

void FVulkanBufferAllocator::CleanUp()
{
    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (Pool)
        {
            Pool->CleanUp();
        }
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanBufferAllocator::UpdateMemoryStats()
{
    SCOPED_LOCK(PoolsCS);

    FVulkanAllocatorUsage Usage;
    for (const FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (Pool)
        {
            Pool->UpdateMemoryStats(Usage);
        }
    }

    STAT_SET(STAT_Vulkan_BufferPoolAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Vulkan_BufferPoolUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Vulkan_BufferPoolFragmented, Usage.FragmentedBytes);
}
#endif

void FVulkanBufferAllocator::ReleasePools()
{
    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        delete Pool;
    }

    Pools.Clear();
}

bool FVulkanBufferAllocator::TryAllocate(VkMemoryPropertyFlags MemoryProperties, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryLocation& OutLocation)
{
    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = SizeInBytes;
    BufferCreateInfo.usage       = UsageFlags;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkDeviceBufferMemoryRequirements DeviceBufferMemReqInfo = {};
    DeviceBufferMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
    DeviceBufferMemReqInfo.pCreateInfo = &BufferCreateInfo;

    VkMemoryRequirements2 MemoryRequirements2 = {};
    MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetDeviceBufferMemoryRequirements(VulkanDevice, &DeviceBufferMemReqInfo, &MemoryRequirements2);

    const VkMemoryRequirements& MemoryRequirements = MemoryRequirements2.memoryRequirements;

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: No suitable memory type for buffer");
        return false;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, MemoryRequirements.alignment);

    if (SizeInBytes > MaxSuballocationSize)
    {
        VkMemoryAllocateInfo AllocateInfo = {};
        AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize  = MemoryRequirements.size;
        AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

        VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
        AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        AllocateFlagsInfo.flags = AllocateFlags;

        AddToStructChain(AllocateInfo, AllocateFlagsInfo);

        VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
        VkResult DedicatedResult = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DedicatedMemory);
        if (VULKAN_FAILED(DedicatedResult))
        {
            VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: Dedicated vkAllocateMemory failed with %s (Size=%llu)", ToString(DedicatedResult), MemoryRequirements.size);
            return false;
        }

        OutLocation.Reset();
        OutLocation.SetMemory(DedicatedMemory);
        OutLocation.SetMemoryOffset(0);
        OutLocation.SetSize(MemoryRequirements.size);
        OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);
        return true;
    }

    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (!Pool)
        {
            continue;
        }

        if (Pool->GetMemoryTypeIndex() != static_cast<uint32>(MemoryTypeIndex) || Pool->GetBufferUsageFlags() != UsageFlags)
        {
            continue;
        }

        if (Pool->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
        {
            return true;
        }
    }

    FVulkanBufferAllocatorPool* NewPool = new FVulkanBufferAllocatorPool(GetDevice(), static_cast<uint32>(MemoryTypeIndex), UsageFlags, PageSizeBytes, MaxSuballocationSize, AllocateFlags);
    if (!NewPool->Initialize())
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: Failed to create new pool (Size=%llu, PageSize=%llu, MaxSuballocation=%llu, MemoryTypeIndex=%d)", SizeInBytes, PageSizeBytes, MaxSuballocationSize, MemoryTypeIndex);
        delete NewPool;
        return false;
    }

    Pools.Add(NewPool);

    if (!NewPool->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: Allocation failed after creating new pool (Size=%llu, PageSize=%llu, MaxSuballocation=%llu)", SizeInBytes, PageSizeBytes, MaxSuballocationSize);
        return false;
    }

    return true;
}

bool FVulkanBufferAllocator::GetDefragCandidate(FVulkanPoolAllocatorAllocationData& OutCandidate, FVulkanBufferAllocatorPool*& OutPool)
{
    SCOPED_LOCK(PoolsCS);

    FVulkanBufferAllocatorPool* BestPool = nullptr;
    uint64 MostFragmented = 0;

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        if (!Pool)
        {
            continue;
        }

        // Skip mapped (host-visible) memory types - we don't want to relocate while CPU may have mappings
        const VkMemoryType& MemType = GetDevice()->GetPhysicalDevice()->GetMemoryProperties().memoryTypes[Pool->GetMemoryTypeIndex()];
        if ((MemType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            continue;
        }

        if (Pool->GetFragmentedBytes() > MostFragmented)
        {
            MostFragmented = Pool->GetFragmentedBytes();
            BestPool       = Pool;
        }
    }

    if (BestPool && BestPool->GetDefragCandidate(OutCandidate))
    {
        OutPool = BestPool;
        return true;
    }

    return false;
}

void FVulkanBufferAllocator::DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame)
{
    if (MaxMovesPerFrame <= 0)
    {
        return;
    }

    CHECK(InCommandContext != nullptr);

    FVulkanTimelineFence& FrameFence = GetDevice()->GetFrameFence();
    const uint64 CompletedFenceValue = FrameFence.GetCompletedValue();

    for (int32 Index = PendingDefragMoves.Size() - 1; Index >= 0; --Index)
    {
        FVulkanPendingDefragMove& Move = PendingDefragMoves[Index];
        if (CompletedFenceValue <= Move.FenceValueAtCreation)
        {
            continue;
        }

        FVulkanResource* Owner = Move.SourceLocation ? Move.SourceLocation->GetOwner() : nullptr;

        if (Owner && Move.SourceLocation)
        {
            Move.SourceLocation->SetMemory(Move.Allocator->GetBackingMemory(Move.NewAllocationData.PageIndex));
            Move.SourceLocation->SetMemoryOffset(Move.NewAllocationData.Offset);
            Move.SourceLocation->SetBackingBuffer(Move.NewBuffer);
            Move.SourceLocation->SetBufferOffset(0);
            Move.SourceLocation->SetPoolAllocationData(Move.NewAllocationData);

            Move.Allocator->TransferOwnership(Move.NewAllocationData, Move.SourceLocation);

            Owner->ResourceRelocated(Move.SourceLocation);
        }

        Move.Allocator->TransferOwnership(Move.OldAllocationData, nullptr);

        FVulkanPoolAllocatorAllocationData OldData = Move.OldAllocationData;
        OldData.Owner = nullptr;

        FVulkanDeviceRHI::DeferDeletion(Move.Allocator, OldData);

        STAT_ADD(STAT_Vulkan_BufferDefragMovesCompleted, 1);
        PendingDefragMoves.RemoveAtSwap(Index);
    }

    int32 MovesAvailable = MaxMovesPerFrame - PendingDefragMoves.Size();
    if (MovesAvailable <= 0)
    {
        STAT_SET(STAT_Vulkan_BufferDefragPending, static_cast<int64>(PendingDefragMoves.Size()));
        return;
    }

    for (int32 MoveIndex = 0; MoveIndex < MovesAvailable; ++MoveIndex)
    {
        FVulkanPoolAllocatorAllocationData Candidate = {};
        FVulkanBufferAllocatorPool* SourcePool = nullptr;
        if (!GetDefragCandidate(Candidate, SourcePool))
        {
            break;
        }

        if (!Candidate.Owner || !Candidate.Owner->GetOwner() || !SourcePool)
        {
            break;
        }

        FVulkanPoolAllocatorAllocationData NewAllocationData = {};
        if (!SourcePool->TryAllocateForDefrag(Candidate.Size, SourcePool->GetAlignment(), Candidate.PageIndex, NewAllocationData))
        {
            break;
        }

        VkBuffer OldBuffer = Candidate.Owner->GetBackingBuffer();
        if (OldBuffer == VK_NULL_HANDLE)
        {
            break;
        }

        VkBufferCreateInfo NewBufferCreateInfo = {};
        NewBufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        NewBufferCreateInfo.size        = Candidate.Size;
        NewBufferCreateInfo.usage       = SourcePool->GetBufferUsageFlags();
        NewBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer NewBuffer = VK_NULL_HANDLE;
        VkResult Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &NewBufferCreateInfo, nullptr, &NewBuffer);
        if (VULKAN_FAILED(Result))
        {
            break;
        }

        VkDeviceMemory NewMemory = SourcePool->GetBackingMemory(NewAllocationData.PageIndex);
        Result = vkBindBufferMemory(GetDevice()->GetVkDevice(), NewBuffer, NewMemory, NewAllocationData.Offset);
        if (VULKAN_FAILED(Result))
        {
            vkDestroyBuffer(GetDevice()->GetVkDevice(), NewBuffer, nullptr);
            break;
        }

        VkBufferCopy Region = {};
        Region.srcOffset = Candidate.Owner->GetBufferOffset();
        Region.dstOffset = 0;
        Region.size      = Candidate.Size;

        InCommandContext->GetCommandBuffer()->CopyBuffer(OldBuffer, NewBuffer, 1, &Region);

        FVulkanPendingDefragMove PendingMove = {};
        PendingMove.SourceLocation        = Candidate.Owner;
        PendingMove.NewBuffer            = NewBuffer;
        PendingMove.Allocator            = &SourcePool->GetPoolAllocator();
        PendingMove.OldAllocationData    = Candidate;
        PendingMove.NewAllocationData    = NewAllocationData;
        PendingMove.FenceValueAtCreation = FrameFence.GetLastSignaledValue();

        PendingMove.Allocator->TransferOwnership(Candidate, nullptr);
        STAT_ADD(STAT_Vulkan_BufferDefragBytesMoved, Candidate.Size);
        PendingDefragMoves.Add(PendingMove);
    }

    STAT_SET(STAT_Vulkan_BufferDefragPending, static_cast<int64>(PendingDefragMoves.Size()));
}

void FVulkanBufferAllocator::CancelPendingDefragMoves(FVulkanResource* Owner)
{
    for (int32 Index = PendingDefragMoves.Size() - 1; Index >= 0; --Index)
    {
        FVulkanPendingDefragMove& Move = PendingDefragMoves[Index];
        if (Move.SourceLocation && Move.SourceLocation->GetOwner() == Owner)
        {
            FVulkanPoolAllocatorAllocationData NewData = Move.NewAllocationData;
            NewData.Owner = nullptr;

            FVulkanDeviceRHI::DeferDeletion(Move.Allocator, NewData);

            if (VULKAN_CHECK_HANDLE(Move.NewBuffer))
            {
                vkDestroyBuffer(GetDevice()->GetVkDevice(), Move.NewBuffer, nullptr);
            }

            PendingDefragMoves.RemoveAtSwap(Index);
        }
    }
}

#endif // VULKAN_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR

#if VULKAN_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR

FVulkanTextureAllocator::FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes)
    : FVulkanDeviceChild(InDevice)
    , DefaultPageSizeBytes(InDefaultPageSizeBytes)
    , PoolsCS()
{
    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        Pools[Index] = nullptr;
    }
}

FVulkanTextureAllocator::~FVulkanTextureAllocator()
{
    Destroy();
}

bool FVulkanTextureAllocator::Initialize()
{
    Destroy();
    return true;
}

void FVulkanTextureAllocator::Destroy()
{
    SCOPED_LOCK(PoolsCS);
    ReleasePools();
}

void FVulkanTextureAllocator::CleanUp()
{
    SCOPED_LOCK(PoolsCS);

    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index])
        {
            Pools[Index]->CleanUp();
        }
    }
}

void FVulkanTextureAllocator::ReleasePools()
{
    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index])
        {
            delete Pools[Index];
            Pools[Index] = nullptr;
        }
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanTextureAllocator::UpdateMemoryStats()
{
    SCOPED_LOCK(PoolsCS);

    FVulkanAllocatorUsage Usage;
    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index])
        {
            Pools[Index]->UpdateMemoryStats(Usage);
        }
    }

    STAT_SET(STAT_Vulkan_TexturePoolAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Vulkan_TexturePoolUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Vulkan_TexturePoolFragmented, Usage.FragmentedBytes);
}
#endif

FVulkanTextureAllocator::ETexturePoolClass FVulkanTextureAllocator::ClassifyTexture(VkImageUsageFlags UsageFlags, uint64 Alignment) const
{
    const bool bIsRTDS    = (UsageFlags & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0;
    const bool bIsStorage = (UsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) != 0;

    if (bIsRTDS)
    {
        return ETexturePoolClass::RenderTargetDepthStencil;
    }

    if (bIsStorage)
    {
        return ETexturePoolClass::StorageOnly;
    }

    if (Alignment <= 4096)
    {
        return ETexturePoolClass::SmallReadOnly;
    }

    return ETexturePoolClass::ReadOnly;
}

bool FVulkanTextureAllocator::TryAllocate(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags MemoryProperties, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryLocation& OutLocation)
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkMemoryDedicatedRequirements DedicatedRequirements = {};
    DedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;

    VkMemoryRequirements2 MemoryRequirements2 = {};
    MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkDeviceImageMemoryRequirements DeviceImageMemReqInfo = {};
    DeviceImageMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
    DeviceImageMemReqInfo.pCreateInfo = &ImageCreateInfo;

    AddToStructChain(MemoryRequirements2, DedicatedRequirements);

    vkGetDeviceImageMemoryRequirements(VulkanDevice, &DeviceImageMemReqInfo, &MemoryRequirements2);

    const VkMemoryRequirements& MemReqs = MemoryRequirements2.memoryRequirements;
    const bool bRequiresDedicated = DedicatedRequirements.requiresDedicatedAllocation == VK_TRUE || DedicatedRequirements.prefersDedicatedAllocation == VK_TRUE;

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemReqs.memoryTypeBits, MemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: No suitable memory type for image");
        return false;
    }

    if (bRequiresDedicated)
    {
        VkMemoryAllocateInfo AllocateInfo = {};
        AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize  = MemReqs.size;
        AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

        VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
        AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        AllocateFlagsInfo.flags = AllocateFlags;

        VkMemoryDedicatedAllocateInfo DedicatedAllocateInfo = {};
        DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        DedicatedAllocateInfo.image = Image;

        AddAllToStructChain(AllocateInfo, AllocateFlagsInfo, DedicatedAllocateInfo);

        VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
        VkResult Result = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DedicatedMemory);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: Dedicated vkAllocateMemory failed with %s", ToString(Result));
            return false;
        }

        OutLocation.Reset();
        OutLocation.SetMemory(DedicatedMemory);
        OutLocation.SetMemoryOffset(0);
        OutLocation.SetSize(MemReqs.size);
        OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);

    #if VULKAN_ENABLE_MEMORY_LOGGING
        if (CVarVulkanLogMemoryAllocations.GetValue())
        {
            VULKAN_INFO("FVulkanTextureAllocator: Using dedicated allocation for image (Size=%llu)", MemReqs.size);
        }
    #endif

        return true;
    }

    const ETexturePoolClass PoolClass = ClassifyTexture(ImageCreateInfo.usage, MemReqs.alignment);
    const uint32 PoolIndex = static_cast<uint32>(PoolClass);

    bool bPoolAllocated = false;
    if (PoolIndex < TEXTURE_POOL_CLASS_COUNT)
    {
        SCOPED_LOCK(PoolsCS);

        if (!Pools[PoolIndex])
        {
            Pools[PoolIndex] = new FVulkanPoolAllocator(GetDevice(), DefaultPageSizeBytes, MemReqs.alignment, DefaultPageSizeBytes, static_cast<uint32>(MemoryTypeIndex), AllocateFlags);
            if (!Pools[PoolIndex]->Initialize())
            {
                delete Pools[PoolIndex];
                Pools[PoolIndex] = nullptr;
            }
        }

        if (Pools[PoolIndex])
        {
            bPoolAllocated = Pools[PoolIndex]->TryAllocate(MemReqs.size, MemReqs.alignment, OutLocation);
        }
    }

    if (bPoolAllocated)
    {
        return true;
    }

    // Fallback to dedicated allocation for textures too large for suballocation
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = MemReqs.size;
    AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

    VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = AllocateFlags;

    AddToStructChain(AllocateInfo, AllocateFlagsInfo);

    VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
    VkResult Result = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DedicatedMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: Fallback dedicated vkAllocateMemory failed with %s (Size=%llu)", ToString(Result), MemReqs.size);
        return false;
    }

    OutLocation.Reset();
    OutLocation.SetMemory(DedicatedMemory);
    OutLocation.SetMemoryOffset(0);
    OutLocation.SetSize(MemReqs.size);
    OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);

#if VULKAN_ENABLE_MEMORY_LOGGING
    if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanTextureAllocator: Pool suballocation failed, using dedicated allocation (Size=%llu)", MemReqs.size);
    }
#endif

    return true;
}

bool FVulkanTextureAllocator::GetDefragCandidate(FVulkanPoolAllocatorAllocationData& OutCandidate, FVulkanPoolAllocator*& OutAllocator)
{
    SCOPED_LOCK(PoolsCS);

    uint64 MostFragmented = 0;
    FVulkanPoolAllocator* BestPool = nullptr;

    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index] && Pools[Index]->GetFragmentedBytes() > MostFragmented)
        {
            MostFragmented = Pools[Index]->GetFragmentedBytes();
            BestPool       = Pools[Index];
        }
    }

    if (BestPool && BestPool->GetDefragCandidate(OutCandidate))
    {
        OutAllocator = BestPool;
        return true;
    }

    return false;
}

void FVulkanTextureAllocator::DefragmentAllocations(FVulkanCommandContext* InCommandContext, int32 MaxMovesPerFrame)
{
    if (MaxMovesPerFrame <= 0)
    {
        return;
    }

    CHECK(InCommandContext != nullptr);

    FVulkanTimelineFence& FrameFence = GetDevice()->GetFrameFence();
    const uint64 CompletedFenceValue = FrameFence.GetCompletedValue();

    for (int32 Index = PendingDefragMoves.Size() - 1; Index >= 0; --Index)
    {
        FVulkanPendingDefragMove& Move = PendingDefragMoves[Index];
        if (CompletedFenceValue <= Move.FenceValueAtCreation)
        {
            continue;
        }

        FVulkanResource* Owner = Move.SourceLocation ? Move.SourceLocation->GetOwner() : nullptr;

        VkImage OldImage = VK_NULL_HANDLE;
        if (Owner)
        {
            FVulkanTextureRHI* Texture = static_cast<FVulkanTextureRHI*>(Owner);
            OldImage = Texture->GetVkImage();

            Texture->SetVkImage(Move.NewImage);

            Move.SourceLocation->SetMemory(Move.Allocator->GetBackingMemory(Move.NewAllocationData.PageIndex));
            Move.SourceLocation->SetMemoryOffset(Move.NewAllocationData.Offset);
            Move.SourceLocation->SetPoolAllocationData(Move.NewAllocationData);

            Move.Allocator->TransferOwnership(Move.NewAllocationData, Move.SourceLocation);

            Owner->ResourceRelocated(Move.SourceLocation);
        }

        Move.Allocator->TransferOwnership(Move.OldAllocationData, nullptr);

        FVulkanPoolAllocatorAllocationData OldData = Move.OldAllocationData;
        OldData.Owner = nullptr;
        
        FVulkanDeviceRHI::DeferDeletion(Move.Allocator, OldData);

        if (VULKAN_CHECK_HANDLE(OldImage))
        {
            vkDestroyImage(GetDevice()->GetVkDevice(), OldImage, nullptr);
        }

        STAT_ADD(STAT_Vulkan_TextureDefragMovesCompleted, 1);
        PendingDefragMoves.RemoveAtSwap(Index);
    }

    int32 MovesAvailable = MaxMovesPerFrame - PendingDefragMoves.Size();
    if (MovesAvailable <= 0)
    {
        STAT_SET(STAT_Vulkan_TextureDefragPending, static_cast<int64>(PendingDefragMoves.Size()));
        return;
    }

    FVulkanBarrierBatcher& BarrierBatcher = InCommandContext->GetBarrierBatcher();
    for (int32 MoveIndex = 0; MoveIndex < MovesAvailable; ++MoveIndex)
    {
        FVulkanPoolAllocatorAllocationData Candidate = {};
        
        FVulkanPoolAllocator* SourceAllocator = nullptr;
        if (!GetDefragCandidate(Candidate, SourceAllocator))
        {
            break;
        }

        if (!Candidate.Owner || !Candidate.Owner->GetOwner() || !SourceAllocator)
        {
            break;
        }

        FVulkanPoolAllocatorAllocationData NewAllocationData = {};
        if (!SourceAllocator->TryAllocateForDefrag(Candidate.Size, SourceAllocator->GetAlignment(), Candidate.PageIndex, NewAllocationData))
        {
            break;
        }

        FVulkanTextureRHI* Texture = static_cast<FVulkanTextureRHI*>(Candidate.Owner->GetOwner());
        
        VkImage OldImage = Texture->GetVkImage();
        VkImage NewImage = VK_NULL_HANDLE;
        
        const VkImageLayout      CurrentLayout = Texture->GetImageLayoutState().GetImageLayout();
        const VkImageCreateInfo& OldCreateInfo = Texture->GetVkImageCreateInfo();
        
        VkResult Result = vkCreateImage(GetDevice()->GetVkDevice(), &OldCreateInfo, nullptr, &NewImage);
        if (VULKAN_FAILED(Result))
        {
            break;
        }

        VkDeviceMemory NewMemory = SourceAllocator->GetBackingMemory(NewAllocationData.PageIndex);
        Result = vkBindImageMemory(GetDevice()->GetVkDevice(), NewImage, NewMemory, NewAllocationData.Offset);
        if (VULKAN_FAILED(Result))
        {
            vkDestroyImage(GetDevice()->GetVkDevice(), NewImage, nullptr);
            break;
        }

        const VkImageAspectFlags AspectMask = GetImageAspectFlagsFromFormat(OldCreateInfo.format);

        {
            VkImageMemoryBarrier2 SrcBarrier = {};
            SrcBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            SrcBarrier.srcAccessMask                   = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            SrcBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_READ_BIT;
            SrcBarrier.oldLayout                       = CurrentLayout;
            SrcBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            SrcBarrier.image                           = OldImage;
            SrcBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            SrcBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            SrcBarrier.subresourceRange.aspectMask     = AspectMask;
            SrcBarrier.subresourceRange.baseArrayLayer = 0;
            SrcBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            SrcBarrier.subresourceRange.baseMipLevel   = 0;
            SrcBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

            VkImageMemoryBarrier2 DstBarrier = {};
            DstBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            DstBarrier.srcAccessMask                   = 0;
            DstBarrier.dstAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            DstBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
            DstBarrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            DstBarrier.image                           = NewImage;
            DstBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
            DstBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            DstBarrier.subresourceRange.aspectMask     = AspectMask;
            DstBarrier.subresourceRange.baseArrayLayer = 0;
            DstBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            DstBarrier.subresourceRange.baseMipLevel   = 0;
            DstBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

            BarrierBatcher.AddImageMemoryBarrier(0, SrcBarrier);
            BarrierBatcher.AddImageMemoryBarrier(0, DstBarrier);
            BarrierBatcher.FlushBarriers(InCommandContext->GetCommandBuffer());
        }

        for (uint32 MipLevel = 0; MipLevel < OldCreateInfo.mipLevels; ++MipLevel)
        {
            VkImageCopy Region = {};
            Region.srcSubresource.aspectMask     = AspectMask;
            Region.srcSubresource.mipLevel       = MipLevel;
            Region.srcSubresource.baseArrayLayer = 0;
            Region.srcSubresource.layerCount     = OldCreateInfo.arrayLayers;
            Region.dstSubresource                = Region.srcSubresource;
            Region.extent.width                  = Math::Max(OldCreateInfo.extent.width >> MipLevel, 1u);
            Region.extent.height                 = Math::Max(OldCreateInfo.extent.height >> MipLevel, 1u);
            Region.extent.depth                  = Math::Max(OldCreateInfo.extent.depth >> MipLevel, 1u);

            InCommandContext->GetCommandBuffer()->CopyImage(OldImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, NewImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
        }

        {
            VkImageMemoryBarrier2 SrcBarrier = {};
            SrcBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            SrcBarrier.srcAccessMask                   = VK_ACCESS_2_TRANSFER_READ_BIT;
            SrcBarrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            SrcBarrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            SrcBarrier.newLayout                       = CurrentLayout;
            SrcBarrier.image                           = OldImage;
            SrcBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            SrcBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            SrcBarrier.subresourceRange.aspectMask     = AspectMask;
            SrcBarrier.subresourceRange.baseArrayLayer = 0;
            SrcBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            SrcBarrier.subresourceRange.baseMipLevel   = 0;
            SrcBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

            VkImageMemoryBarrier2 DstBarrier = {};
            DstBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            DstBarrier.srcAccessMask                   = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            DstBarrier.dstAccessMask                   = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
            DstBarrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            DstBarrier.newLayout                       = CurrentLayout;
            DstBarrier.image                           = NewImage;
            DstBarrier.srcStageMask                    = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            DstBarrier.dstStageMask                    = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            DstBarrier.subresourceRange.aspectMask     = AspectMask;
            DstBarrier.subresourceRange.baseArrayLayer = 0;
            DstBarrier.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
            DstBarrier.subresourceRange.baseMipLevel   = 0;
            DstBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;

            BarrierBatcher.AddImageMemoryBarrier(0, SrcBarrier);
            BarrierBatcher.AddImageMemoryBarrier(0, DstBarrier);
            BarrierBatcher.FlushBarriers(InCommandContext->GetCommandBuffer());
        }

        FVulkanPendingDefragMove PendingMove = {};
        PendingMove.SourceLocation       = Candidate.Owner;
        PendingMove.NewImage             = NewImage;
        PendingMove.Allocator            = SourceAllocator;
        PendingMove.OldAllocationData    = Candidate;
        PendingMove.NewAllocationData    = NewAllocationData;
        PendingMove.FenceValueAtCreation = FrameFence.GetLastSignaledValue();

        SourceAllocator->TransferOwnership(Candidate, nullptr);
        STAT_ADD(STAT_Vulkan_TextureDefragBytesMoved, Candidate.Size);
        PendingDefragMoves.Add(PendingMove);
    }

    STAT_SET(STAT_Vulkan_TextureDefragPending, static_cast<int64>(PendingDefragMoves.Size()));
}

void FVulkanTextureAllocator::CancelPendingDefragMoves(FVulkanResource* Owner)
{
    for (int32 Index = PendingDefragMoves.Size() - 1; Index >= 0; --Index)
    {
        FVulkanPendingDefragMove& Move = PendingDefragMoves[Index];
        if (Move.SourceLocation && Move.SourceLocation->GetOwner() == Owner)
        {
            FVulkanPoolAllocatorAllocationData NewData = Move.NewAllocationData;
            NewData.Owner = nullptr;

            FVulkanDeviceRHI::DeferDeletion(Move.Allocator, NewData);

            if (VULKAN_CHECK_HANDLE(Move.NewImage))
            {
                vkDestroyImage(GetDevice()->GetVkDevice(), Move.NewImage, nullptr);
            }

            PendingDefragMoves.RemoveAtSwap(Index);
        }
    }
}

#else // VULKAN_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR

FVulkanTextureAllocator::FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes)
    : FVulkanDeviceChild(InDevice)
    , DefaultPageSizeBytes(InDefaultPageSizeBytes)
    , PoolsCS()
{
    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        Pools[Index] = nullptr;
    }
}

FVulkanTextureAllocator::~FVulkanTextureAllocator()
{
    Destroy();
}

bool FVulkanTextureAllocator::Initialize()
{
    Destroy();
    return true;
}

void FVulkanTextureAllocator::Destroy()
{
    SCOPED_LOCK(PoolsCS);
    ReleasePools();
}

void FVulkanTextureAllocator::CleanUp()
{
    SCOPED_LOCK(PoolsCS);

    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index])
        {
            Pools[Index]->CleanUp();
        }
    }
}

void FVulkanTextureAllocator::ReleasePools()
{
    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index])
        {
            delete Pools[Index];
            Pools[Index] = nullptr;
        }
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanTextureAllocator::UpdateMemoryStats()
{
    SCOPED_LOCK(PoolsCS);

    FVulkanAllocatorUsage Usage;
    for (uint32 Index = 0; Index < TEXTURE_POOL_CLASS_COUNT; ++Index)
    {
        if (Pools[Index])
        {
            Pools[Index]->UpdateMemoryStats(Usage);
        }
    }

    STAT_SET(STAT_Vulkan_TexturePoolAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Vulkan_TexturePoolUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Vulkan_TexturePoolFragmented, Usage.FragmentedBytes);
}
#endif

FVulkanTextureAllocator::ETexturePoolClass FVulkanTextureAllocator::ClassifyTexture(VkImageUsageFlags UsageFlags, uint64 Alignment) const
{
    const bool bIsRTDS    = (UsageFlags & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0;
    const bool bIsStorage = (UsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) != 0;

    if (bIsRTDS)
    {
        return ETexturePoolClass::RenderTargetDepthStencil;
    }

    if (bIsStorage)
    {
        return ETexturePoolClass::StorageOnly;
    }

    if (Alignment <= 4096)
    {
        return ETexturePoolClass::SmallReadOnly;
    }

    return ETexturePoolClass::ReadOnly;
}

bool FVulkanTextureAllocator::TryAllocate(VkImage Image, const VkImageCreateInfo& ImageCreateInfo, VkMemoryPropertyFlags MemoryProperties, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryLocation& OutLocation)
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkMemoryDedicatedRequirements DedicatedRequirements = {};
    DedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;

    VkMemoryRequirements2 MemoryRequirements2 = {};
    MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkDeviceImageMemoryRequirements DeviceImageMemReqInfo = {};
    DeviceImageMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
    DeviceImageMemReqInfo.pCreateInfo = &ImageCreateInfo;

    AddToStructChain(MemoryRequirements2, DedicatedRequirements);

    vkGetDeviceImageMemoryRequirements(VulkanDevice, &DeviceImageMemReqInfo, &MemoryRequirements2);

    const VkMemoryRequirements& MemReqs = MemoryRequirements2.memoryRequirements;
    const bool bRequiresDedicated = DedicatedRequirements.requiresDedicatedAllocation == VK_TRUE || DedicatedRequirements.prefersDedicatedAllocation == VK_TRUE;

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemReqs.memoryTypeBits, MemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: No suitable memory type for image");
        return false;
    }

    if (bRequiresDedicated)
    {
        VkMemoryAllocateInfo AllocateInfo = {};
        AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize  = MemReqs.size;
        AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

        VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
        AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        AllocateFlagsInfo.flags = AllocateFlags;

        VkMemoryDedicatedAllocateInfo DedicatedAllocateInfo = {};
        DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        DedicatedAllocateInfo.image = Image;

        AddAllToStructChain(AllocateInfo, AllocateFlagsInfo, DedicatedAllocateInfo);

        VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
        VkResult Result = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DedicatedMemory);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: Dedicated vkAllocateMemory failed with %s", ToString(Result));
            return false;
        }

        OutLocation.Reset();
        OutLocation.SetMemory(DedicatedMemory);
        OutLocation.SetMemoryOffset(0);
        OutLocation.SetSize(MemReqs.size);
        OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);
        return true;
    }

    const ETexturePoolClass PoolClass = ClassifyTexture(ImageCreateInfo.usage, MemReqs.alignment);
    const uint32 PoolIndex = static_cast<uint32>(PoolClass);

    bool bPoolAllocated = false;
    if (PoolIndex < TEXTURE_POOL_CLASS_COUNT)
    {
        SCOPED_LOCK(PoolsCS);

        if (!Pools[PoolIndex])
        {
            Pools[PoolIndex] = new FVulkanMultiBuddyAllocator(GetDevice(), DefaultPageSizeBytes, VULKAN_MIN_BUDDY_ALLOCATOR_BLOCK_SIZE, static_cast<uint32>(MemoryTypeIndex), AllocateFlags, /*BufferUsageFlags=*/0);
            if (!Pools[PoolIndex]->Initialize())
            {
                delete Pools[PoolIndex];
                Pools[PoolIndex] = nullptr;
            }
        }

        if (Pools[PoolIndex])
        {
            bPoolAllocated = Pools[PoolIndex]->TryAllocate(MemReqs.size, MemReqs.alignment, OutLocation);
        }
    }

    if (bPoolAllocated)
    {
        return true;
    }

    // Fallback to dedicated allocation for textures too large for suballocation
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = MemReqs.size;
    AllocateInfo.memoryTypeIndex = static_cast<uint32>(MemoryTypeIndex);

    VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = AllocateFlags;

    AddToStructChain(AllocateInfo, AllocateFlagsInfo);

    VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
    VkResult Result = GetDevice()->GetMemoryManager().AllocateMemory(&AllocateInfo, &DedicatedMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: Fallback dedicated vkAllocateMemory failed with %s (Size=%llu)", ToString(Result), MemReqs.size);
        return false;
    }

    OutLocation.Reset();
    OutLocation.SetMemory(DedicatedMemory);
    OutLocation.SetMemoryOffset(0);
    OutLocation.SetSize(MemReqs.size);
    OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);
    return true;
}

#endif // VULKAN_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR

FVulkanUploadHeapAllocator::FVulkanUploadHeapAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InSmallThreshold, uint64 InLargeThreshold)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , DefaultAlignment(InAlignment)
    , SmallThreshold(InSmallThreshold)
    , LargeThreshold(InLargeThreshold)
    , MemoryTypeIndex(0)
    , SmallAllocator(nullptr)
    , LargeAllocator(nullptr)
    , ConstantsAllocator(nullptr)
{
}

FVulkanUploadHeapAllocator::~FVulkanUploadHeapAllocator()
{
    Destroy();
}

bool FVulkanUploadHeapAllocator::Initialize(uint32 InMemoryTypeIndex)
{
    Destroy();

    MemoryTypeIndex = InMemoryTypeIndex;

    const VkMemoryAllocateFlags AllocateFlags    = 0;
    const VkBufferUsageFlags    TransferSrcUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    const VkBufferUsageFlags    UniformUsage     = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    SmallAllocator = new FVulkanMultiBuddyAllocator(GetDevice(), PageSizeBytes, DefaultAlignment, MemoryTypeIndex, AllocateFlags, TransferSrcUsage);
    if (!SmallAllocator->Initialize())
    {
        Destroy();
        return false;
    }

    static constexpr uint64 LARGE_PAGE_SIZE = 16ull * 1024ull * 1024ull;
    LargeAllocator = new FVulkanPoolAllocator(GetDevice(), LARGE_PAGE_SIZE, DefaultAlignment, LARGE_PAGE_SIZE, MemoryTypeIndex, AllocateFlags, TransferSrcUsage);
    if (!LargeAllocator->Initialize())
    {
        Destroy();
        return false;
    }

    const uint64 ConstantsAlignment = GetDevice()->GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
    ConstantsAllocator = new FVulkanMultiBuddyAllocator(GetDevice(), PageSizeBytes, ConstantsAlignment, MemoryTypeIndex, AllocateFlags, UniformUsage);
    if (!ConstantsAllocator->Initialize())
    {
        Destroy();
        return false;
    }

    return true;
}

void FVulkanUploadHeapAllocator::Destroy()
{
    if (SmallAllocator)
    {
        SmallAllocator->Destroy();
        delete SmallAllocator;
        SmallAllocator = nullptr;
    }

    if (LargeAllocator)
    {
        LargeAllocator->Destroy();
        delete LargeAllocator;
        LargeAllocator = nullptr;
    }

    if (ConstantsAllocator)
    {
        ConstantsAllocator->Destroy();
        delete ConstantsAllocator;
        ConstantsAllocator = nullptr;
    }
}

void FVulkanUploadHeapAllocator::CleanUp()
{
    if (SmallAllocator)
    {
        SmallAllocator->CleanUp();
    }

    if (LargeAllocator)
    {
        LargeAllocator->CleanUp();
    }

    if (ConstantsAllocator)
    {
        ConstantsAllocator->CleanUp();
    }
}

#if VULKAN_ENABLE_STATS
void FVulkanUploadHeapAllocator::UpdateMemoryStats()
{
    FVulkanAllocatorUsage Usage;

    if (SmallAllocator)
    {
        SmallAllocator->UpdateMemoryStats(Usage);
    }
    if (LargeAllocator)
    {
        LargeAllocator->UpdateMemoryStats(Usage);
    }
    if (ConstantsAllocator)
    {
        ConstantsAllocator->UpdateMemoryStats(Usage);
    }

    STAT_SET(STAT_Vulkan_UploadHeapAllocated,  Usage.AllocatedBytes);
    STAT_SET(STAT_Vulkan_UploadHeapUsed,       Usage.UsedBytes);
    STAT_SET(STAT_Vulkan_UploadHeapFragmented, Usage.FragmentedBytes);
}
#endif

void* FVulkanUploadHeapAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryLocation& OutLocation)
{
    if (BufferUsageFlags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        const uint64 ConstantsAlignment = GetDevice()->GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
        const uint64 UsedAlignment      = Alignment ? Alignment : ConstantsAlignment;

        if (ConstantsAllocator && ConstantsAllocator->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
        {
            return OutLocation.GetMappedBaseAddress();
        }

        return AllocateOversized(SizeInBytes, UsedAlignment, BufferUsageFlags, OutLocation);
    }

    const uint64 UsedAlignment = Alignment ? Alignment : DefaultAlignment;
    if (SizeInBytes <= SmallThreshold)
    {
        if (SmallAllocator && SmallAllocator->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
        {
            return OutLocation.GetMappedBaseAddress();
        }
    }
    else
    {
        if (LargeAllocator && LargeAllocator->TryAllocate(SizeInBytes, UsedAlignment, OutLocation))
        {
            return OutLocation.GetMappedBaseAddress();
        }
    }

    return AllocateOversized(SizeInBytes, UsedAlignment, BufferUsageFlags, OutLocation);
}

void* FVulkanUploadHeapAllocator::AllocateOversized(uint64 SizeInBytes, uint64 Alignment, VkBufferUsageFlags BufferUsageFlags, FVulkanMemoryLocation& OutLocation)
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    const uint64 AlignedSize = Math::AlignUp<uint64>(SizeInBytes, Math::Max<uint64>(Alignment, 16ull));

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = AlignedSize;
    BufferCreateInfo.usage       = BufferUsageFlags;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDeviceBufferMemoryRequirements DeviceBufferMemReqInfo = {};
    DeviceBufferMemReqInfo.sType       = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
    DeviceBufferMemReqInfo.pCreateInfo = &BufferCreateInfo;

    VkMemoryRequirements2 MemReqs2 = {};
    MemReqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    vkGetDeviceBufferMemoryRequirements(VulkanDevice, &DeviceBufferMemReqInfo, &MemReqs2);

    const VkMemoryRequirements& MemReqs = MemReqs2.memoryRequirements;

    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = MemReqs.size;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();

    VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
    VkResult Result = MemoryManager.AllocateMemory(&AllocateInfo, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkAllocateMemory failed with %s (Size=%llu)", ToString(Result), MemReqs.size);
        return nullptr;
    }

    VkBuffer Buffer = VK_NULL_HANDLE;
    Result = vkCreateBuffer(VulkanDevice, &BufferCreateInfo, nullptr, &Buffer);
    if (VULKAN_FAILED(Result))
    {
        MemoryManager.FreeMemory(DeviceMemory);
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkCreateBuffer failed (Size=%llu)", AlignedSize);
        return nullptr;
    }

    Result = vkBindBufferMemory(VulkanDevice, Buffer, DeviceMemory, 0);
    if (VULKAN_FAILED(Result))
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        MemoryManager.FreeMemory(DeviceMemory);
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkBindBufferMemory failed");
        return nullptr;
    }

    void* MappedMemory = nullptr;
    Result = vkMapMemory(VulkanDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, &MappedMemory);
    
    if (VULKAN_FAILED(Result))
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        MemoryManager.FreeMemory(DeviceMemory);
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkMapMemory failed");
        return nullptr;
    }

    OutLocation.Reset();
    OutLocation.SetMemory(DeviceMemory);
    OutLocation.SetMemoryOffset(0);
    OutLocation.SetBackingBuffer(Buffer);
    OutLocation.SetBufferOffset(0);
    OutLocation.SetMappedBaseAddress(MappedMemory);
    OutLocation.SetSize(MemReqs.size);
    OutLocation.SetLocationType(EVulkanMemoryLocationType::Dedicated);

#if VULKAN_ENABLE_MEMORY_LOGGING
    if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanUploadHeapAllocator: Oversized upload allocation (Size=%llu)", MemReqs.size);
    }
#endif

    return MappedMemory;
}
