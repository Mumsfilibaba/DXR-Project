#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Math/Math.h"
#include "VulkanRHI/VulkanMemoryManager.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanRHI.h"

static TAutoConsoleVariable<bool> CVarVulkanLogMemoryAllocations(
    "VulkanRHI.LogMemoryAllocations",
    "Log when new memory allocator pages or dedicated allocations are created",
    false);

static constexpr uint64 BUFFER_PAGE_SIZE          = 64ull * 1024ull * 1024ull;
static constexpr uint64 BUFFER_MIN_BLOCK          = 256ull;
static constexpr uint64 BUFFER_MAX_SUBALLOCATION  = 32ull * 1024ull * 1024ull;
static constexpr uint64 TEXTURE_PAGE_SIZE         = 64ull * 1024ull * 1024ull;
static constexpr uint64 UPLOAD_PAGE_SIZE          = 8ull * 1024ull * 1024ull;
static constexpr uint64 UPLOAD_ALIGNMENT          = 256ull;
static constexpr uint64 UPLOAD_SMALL_THRESHOLD    = 64ull * 1024ull;
static constexpr uint64 UPLOAD_LARGE_THRESHOLD    = 2ull * 1024ull * 1024ull;
static constexpr uint64 CONSTANTS_PAGE_SIZE       = 2ull * 1024ull * 1024ull;
static constexpr uint64 STAGING_PAGE_SIZE         = 4ull * 1024ull * 1024ull;

FVulkanMemoryStorage::FVulkanMemoryStorage(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Memory(VK_NULL_HANDLE)
    , MemoryOffset(0)
    , BackingBuffer(VK_NULL_HANDLE)
    , BufferOffset(0)
    , DeviceAddress(0)
    , MappedBaseAddress(nullptr)
    , Size(0)
    , AllocatorType(EVulkanAllocatorType::None)
    , StorageType(EVulkanMemoryStorageType::Unknown)
    , AllocationData()
    , AllocatorPointers()
{
}

FVulkanMemoryStorage::~FVulkanMemoryStorage()
{
    ReleaseMemory();
}

void FVulkanMemoryStorage::Swap(FVulkanMemoryStorage& Other)
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
    ::Swap(StorageType, Other.StorageType);
    ::Swap(AllocatorPointers.AsVoid, Other.AllocatorPointers.AsVoid);
}

void FVulkanMemoryStorage::ReleaseMemory()
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
            if (StorageType == EVulkanMemoryStorageType::Dedicated)
            {
                FVulkanRHI::DeferDeletion(Memory, BackingBuffer);
            }
            break;
        }
    }

    Reset();
}

void FVulkanMemoryStorage::Reset()
{
    Memory                   = VK_NULL_HANDLE;
    MemoryOffset             = 0;
    BackingBuffer            = VK_NULL_HANDLE;
    BufferOffset             = 0;
    DeviceAddress            = 0;
    MappedBaseAddress        = nullptr;
    Size                     = 0;
    AllocatorType            = EVulkanAllocatorType::None;
    StorageType              = EVulkanMemoryStorageType::Unknown;
    AllocatorPointers.AsVoid = nullptr;
}

FVulkanMemoryManager::FVulkanMemoryManager(FVulkanDevice* InDevice, uint32 InUploadMemoryTypeIndex)
    : FVulkanDeviceChild(InDevice)
    , BufferAllocator(InDevice, BUFFER_PAGE_SIZE, BUFFER_MIN_BLOCK, BUFFER_MAX_SUBALLOCATION)
    , TextureAllocator(InDevice, TEXTURE_PAGE_SIZE)
    , UploadHeapAllocator(InDevice, UPLOAD_PAGE_SIZE, UPLOAD_ALIGNMENT, UPLOAD_SMALL_THRESHOLD, UPLOAD_LARGE_THRESHOLD)
    , DynamicConstantsAllocator(InDevice, CONSTANTS_PAGE_SIZE, InUploadMemoryTypeIndex, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    , StagingBufferAllocator(InDevice, STAGING_PAGE_SIZE, InUploadMemoryTypeIndex, 0, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    , UploadMemoryTypeIndex(InUploadMemoryTypeIndex)
{
}

bool FVulkanMemoryManager::Initialize()
{
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
}

void FVulkanMemoryManager::CleanUpLinearAllocators()
{
    DynamicConstantsAllocator.CleanUp();
    StagingBufferAllocator.CleanUp();
}

bool FVulkanMemoryManager::AllocateBufferMemory(VkMemoryPropertyFlags PropertyFlags, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    return BufferAllocator.TryAllocate(PropertyFlags, UsageFlags, AllocateFlags, SizeInBytes, Alignment, OutStorage);
}

bool FVulkanMemoryManager::AllocateImageMemory(VkImage Image, VkMemoryPropertyFlags PropertyFlags, VkImageUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryStorage& OutStorage)
{
    return TextureAllocator.TryAllocate(Image, PropertyFlags, UsageFlags, AllocateFlags, OutStorage);
}

void* FVulkanMemoryManager::AllocateUploadMemory(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    return UploadHeapAllocator.Allocate(SizeInBytes, Alignment, OutStorage);
}

void* FVulkanMemoryManager::AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    return DynamicConstantsAllocator.Allocate(SizeInBytes, Alignment, OutStorage);
}

void* FVulkanMemoryManager::AllocateStagingBuffer(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    return StagingBufferAllocator.Allocate(SizeInBytes, Alignment, OutStorage);
}

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

    FVulkanStructChain AllocateInfoChain(AllocateInfo);
    AllocateInfoChain.AddNext(AllocateFlagsInfo);

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    VkResult Result = vkAllocateMemory(VulkanDevice, &AllocateInfo, nullptr, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanBuddyAllocator: vkAllocateMemory failed (Size=%llu, MemoryTypeIndex=%u)", BackingStorageSize, MemoryTypeIndex);
        return false;
    }

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

    if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanBuddyAllocator: Initialized (Size=%llu, MinBlock=%llu, MemoryTypeIndex=%u, HasSharedBuffer=%s)",
            BackingStorageSize, MinBlockBytes, MemoryTypeIndex, SharedBuffer != VK_NULL_HANDLE ? "true" : "false");
    }

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
        vkFreeMemory(VulkanDevice, DeviceMemory, nullptr);
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

bool FVulkanBuddyAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
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
    OutStorage.Reset();
    OutStorage.SetSize(BlockSize);
    OutStorage.SetMemory(DeviceMemory);
    OutStorage.SetMemoryOffset(Offset);
    OutStorage.SetStorageType(EVulkanMemoryStorageType::Suballocated);

    if (SharedBuffer != VK_NULL_HANDLE)
    {
        OutStorage.SetBackingBuffer(SharedBuffer);
        OutStorage.SetBufferOffset(Offset);
        OutStorage.SetDeviceAddress(BaseDeviceAddress ? (BaseDeviceAddress + Offset) : 0);
    }

    OutStorage.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + Offset) : nullptr);

    FVulkanBuddyAllocatorAllocationData AllocationData = {};
    AllocationData.Order  = Order;
    AllocationData.Offset = Offset;

    OutStorage.SetBuddyAllocationData(AllocationData);
    OutStorage.SetBuddyAllocator(this);

    return true;
}

void FVulkanBuddyAllocator::Deallocate(const FVulkanMemoryStorage& Storage)
{
    FVulkanRHI::DeferDeletion(this, Storage.GetBuddyAllocationData());
}

void FVulkanBuddyAllocator::RecycleAllocation(const FVulkanBuddyAllocatorAllocationData& AllocationData)
{
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

bool FVulkanMultiBuddyAllocator::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    SCOPED_LOCK(AllocatorsCS);

    for (FVulkanBuddyAllocator* Allocator : Allocators)
    {
        if (Allocator && Allocator->TryAllocate(SizeInBytes, Alignment, OutStorage))
        {
            return true;
        }
    }

    if (!CreateAllocator())
    {
        return false;
    }

    FVulkanBuddyAllocator* Allocator = Allocators[Allocators.Size() - 1];
    return Allocator && Allocator->TryAllocate(SizeInBytes, Alignment, OutStorage);
}

FVulkanPoolAllocatorPage::FVulkanPoolAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , UsedBytes(0)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , DeviceMemory(VK_NULL_HANDLE)
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

    if (DeviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(VulkanDevice, DeviceMemory, nullptr);
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

    FVulkanStructChain AllocateInfoChain(AllocateInfo);
    AllocateInfoChain.AddNext(AllocateFlagsInfo);

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    VkResult Result = vkAllocateMemory(VulkanDevice, &AllocateInfo, nullptr, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanPoolAllocatorPage: vkAllocateMemory failed (Size=%llu, MemoryTypeIndex=%u)", PageSizeBytes, MemoryTypeIndex);
        return false;
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

bool FVulkanPoolAllocatorPage::TryAllocate(uint64 SizeInBytes, uint64 InAlignment, uint32 InPageIndex, FVulkanMemoryStorage& OutStorage)
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

        OutStorage.Reset();
        OutStorage.SetSize(SizeInBytes);
        OutStorage.SetMemory(DeviceMemory);
        OutStorage.SetMemoryOffset(AlignedOffset);
        OutStorage.SetMappedBaseAddress(MappedBaseAddress ? (MappedBaseAddress + AlignedOffset) : nullptr);
        OutStorage.SetStorageType(EVulkanMemoryStorageType::Suballocated);

        FVulkanPoolAllocatorAllocationData AllocationData = {};
        AllocationData.PageIndex = InPageIndex;
        AllocationData.Offset    = AlignedOffset;
        AllocationData.Size      = SizeInBytes;

        OutStorage.SetPoolAllocationData(AllocationData);

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

        return true;
    }

    return false;
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

FVulkanPoolAllocator::FVulkanPoolAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint64 InAlignment, uint64 InMaxAllocationSize, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , Alignment(Math::Max<uint64>(InAlignment, 16ull))
    , MaxAllocationSize(InMaxAllocationSize)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
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

    FVulkanPoolAllocatorPage* NewPage = new FVulkanPoolAllocatorPage(GetDevice(), ActualPageSize, Alignment, MemoryTypeIndex, AllocateFlags);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

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

bool FVulkanPoolAllocator::TryAllocate(uint64 SizeInBytes, uint64 InAlignment, FVulkanMemoryStorage& OutStorage)
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
        if (Page && Page->TryAllocate(SizeAligned, UsedAlignment, PageIndex, OutStorage))
        {
            OutStorage.SetPoolAllocator(this);
            return true;
        }
    }

    uint32 NewPageIndex = UINT32_MAX;
    FVulkanPoolAllocatorPage* NewPage = CreatePage(SizeAligned, NewPageIndex);
    if (!NewPage)
    {
        return false;
    }

    if (!NewPage->TryAllocate(SizeAligned, UsedAlignment, NewPageIndex, OutStorage))
    {
        return false;
    }

    OutStorage.SetPoolAllocator(this);
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

void FVulkanPoolAllocator::Deallocate(const FVulkanMemoryStorage& Storage)
{
    FVulkanRHI::DeferDeletion(this, Storage.GetPoolAllocationData());
}

void FVulkanPoolAllocator::RecycleAllocation(const FVulkanPoolAllocatorAllocationData& AllocationData)
{
    if (AllocationData.PageIndex == UINT32_MAX || AllocationData.Size == 0)
    {
        return;
    }

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

FVulkanLinearAllocatorPage::FVulkanLinearAllocatorPage(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , BufferUsageFlags(InBufferUsageFlags)
    , CurrentOffset(0)
    , DeviceMemory(VK_NULL_HANDLE)
    , Buffer(VK_NULL_HANDLE)
    , MappedBaseAddress(nullptr)
{
}

FVulkanLinearAllocatorPage::~FVulkanLinearAllocatorPage()
{
    Reset();
}

bool FVulkanLinearAllocatorPage::Initialize()
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = PageSizeBytes;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

    VkMemoryAllocateFlagsInfo AllocateFlagsInfo = {};
    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = AllocateFlags;

    FVulkanStructChain AllocateInfoChain(AllocateInfo);
    AllocateInfoChain.AddNext(AllocateFlagsInfo);

    VkResult Result = vkAllocateMemory(VulkanDevice, &AllocateInfo, nullptr, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanLinearAllocatorPage: vkAllocateMemory failed");
        return false;
    }

    if (BufferUsageFlags != 0)
    {
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size        = PageSizeBytes;
        BufferCreateInfo.usage       = BufferUsageFlags;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Result = vkCreateBuffer(VulkanDevice, &BufferCreateInfo, nullptr, &Buffer);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanLinearAllocatorPage: vkCreateBuffer failed");
            Reset();
            return false;
        }

        Result = vkBindBufferMemory(VulkanDevice, Buffer, DeviceMemory, 0);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanLinearAllocatorPage: vkBindBufferMemory failed");
            Reset();
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
            VULKAN_ERROR_CRITICAL("FVulkanLinearAllocatorPage: vkMapMemory failed");
            Reset();
            return false;
        }

        MappedBaseAddress = static_cast<uint8*>(Mapped);
    }

    CurrentOffset = 0;
    return true;
}

void FVulkanLinearAllocatorPage::Reset()
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    if (MappedBaseAddress)
    {
        vkUnmapMemory(VulkanDevice, DeviceMemory);
        MappedBaseAddress = nullptr;
    }

    if (Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    if (DeviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(VulkanDevice, DeviceMemory, nullptr);
        DeviceMemory = VK_NULL_HANDLE;
    }

    CurrentOffset = 0;
}

bool FVulkanLinearAllocatorPage::TryAllocate(uint64 SizeInBytes, uint64 Alignment, uint64& OutOffset)
{
    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 AlignedOffset = Math::AlignUp<uint64>(CurrentOffset, UsedAlignment);

    if (AlignedOffset + SizeInBytes > PageSizeBytes)
    {
        return false;
    }

    OutOffset     = AlignedOffset;
    CurrentOffset = AlignedOffset + SizeInBytes;
    return true;
}

FVulkanLinearAllocator::FVulkanLinearAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes, uint32 InMemoryTypeIndex, VkMemoryAllocateFlags InAllocateFlags, VkBufferUsageFlags InBufferUsageFlags)
    : FVulkanDeviceChild(InDevice)
    , PageSizeBytes(InPageSizeBytes)
    , MemoryTypeIndex(InMemoryTypeIndex)
    , AllocateFlags(InAllocateFlags)
    , BufferUsageFlags(InBufferUsageFlags)
    , Pages()
    , FullPages()
    , PagesCS()
{
}

FVulkanLinearAllocator::~FVulkanLinearAllocator()
{
    SCOPED_LOCK(PagesCS);

    for (FVulkanLinearAllocatorPage* Page : Pages)
    {
        delete Page;
    }

    Pages.Clear();

    for (FVulkanLinearAllocatorPage* Page : FullPages)
    {
        delete Page;
    }

    FullPages.Clear();
}

FVulkanLinearAllocatorPage* FVulkanLinearAllocator::CreatePage()
{
    FVulkanLinearAllocatorPage* NewPage = new FVulkanLinearAllocatorPage(GetDevice(), PageSizeBytes, MemoryTypeIndex, AllocateFlags, BufferUsageFlags);
    if (!NewPage->Initialize())
    {
        delete NewPage;
        return nullptr;
    }

    return NewPage;
}

void FVulkanLinearAllocator::RetirePage(FVulkanLinearAllocatorPage* Page)
{
    if (Page)
    {
        Page->Reset();
    }
}

void* FVulkanLinearAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    if (SizeInBytes == 0)
    {
        return nullptr;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, 16ull);
    const uint64 SizeAligned   = Math::AlignUp<uint64>(SizeInBytes, UsedAlignment);

    SCOPED_LOCK(PagesCS);

    FVulkanLinearAllocatorPage* SelectedPage = nullptr;

    uint64 AllocationOffset = 0;
    for (int32 Index = 0; Index < Pages.Size();)
    {
        FVulkanLinearAllocatorPage* Page = Pages[Index];
        if (!Page)
        {
            Pages.RemoveAtSwap(Index);
            continue;
        }

        if (Page->TryAllocate(SizeAligned, UsedAlignment, AllocationOffset))
        {
            SelectedPage = Page;
            break;
        }

        if (Page->IsExhausted())
        {
            FullPages.Add(Page);
            Pages.RemoveAtSwap(Index);
            continue;
        }

        ++Index;
    }

    if (!SelectedPage)
    {
        SelectedPage = CreatePage();
        if (!SelectedPage)
        {
            return nullptr;
        }

        Pages.Add(SelectedPage);

        if (!SelectedPage->TryAllocate(SizeAligned, UsedAlignment, AllocationOffset))
        {
            return nullptr;
        }
    }

    OutStorage.Reset();
    OutStorage.SetMemory(SelectedPage->GetDeviceMemory());
    OutStorage.SetMemoryOffset(AllocationOffset);
    OutStorage.SetSize(SizeAligned);
    OutStorage.SetStorageType(EVulkanMemoryStorageType::Suballocated);

    if (SelectedPage->GetBuffer() != VK_NULL_HANDLE)
    {
        OutStorage.SetBackingBuffer(SelectedPage->GetBuffer());
        OutStorage.SetBufferOffset(AllocationOffset);
    }

    uint8* MappedBase = SelectedPage->GetMappedMemory();
    OutStorage.SetMappedBaseAddress(MappedBase ? (MappedBase + AllocationOffset) : nullptr);

    if (SelectedPage->IsExhausted())
    {
        for (int32 Index = 0; Index < Pages.Size(); ++Index)
        {
            if (Pages[Index] == SelectedPage)
            {
                FullPages.Add(SelectedPage);
                Pages.RemoveAtSwap(Index);
                break;
            }
        }
    }

    return OutStorage.GetMappedBaseAddress();
}

void FVulkanLinearAllocator::CleanUp()
{
    SCOPED_LOCK(PagesCS);

    for (FVulkanLinearAllocatorPage* Page : FullPages)
    {
        if (Page)
        {
            Pages.Add(Page);
        }
    }
    
    FullPages.Clear();

    for (FVulkanLinearAllocatorPage* Page : Pages)
    {
        if (Page)
        {
            Page->ResetOffset();
        }
    }
}

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

bool FVulkanBufferAllocatorPool::TryAllocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    if (SizeInBytes > MaxSuballocationSize)
    {
        return false;
    }

    return MultiBuddyAllocator.TryAllocate(SizeInBytes, Alignment, OutStorage);
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

void FVulkanBufferAllocator::ReleasePools()
{
    SCOPED_LOCK(PoolsCS);

    for (FVulkanBufferAllocatorPool* Pool : Pools)
    {
        delete Pool;
    }

    Pools.Clear();
}

bool FVulkanBufferAllocator::TryAllocate(VkMemoryPropertyFlags MemoryProperties, VkBufferUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    VkBufferCreateInfo DummyBufferInfo = {};
    DummyBufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    DummyBufferInfo.size        = SizeInBytes;
    DummyBufferInfo.usage       = UsageFlags;
    DummyBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkBuffer DummyBuffer = VK_NULL_HANDLE;
    VkResult Result = vkCreateBuffer(VulkanDevice, &DummyBufferInfo, nullptr, &DummyBuffer);
    if (VULKAN_FAILED(Result))
    {
        return false;
    }

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(VulkanDevice, DummyBuffer, &MemoryRequirements);
    vkDestroyBuffer(VulkanDevice, DummyBuffer, nullptr);

    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR_CRITICAL("FVulkanBufferAllocator: No suitable memory type for buffer");
        return false;
    }

    const uint64 UsedAlignment = Math::Max<uint64>(Alignment, MemoryRequirements.alignment);

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

        if (Pool->TryAllocate(SizeInBytes, UsedAlignment, OutStorage))
        {
            return true;
        }
    }

    FVulkanBufferAllocatorPool* NewPool = new FVulkanBufferAllocatorPool(GetDevice(), static_cast<uint32>(MemoryTypeIndex), UsageFlags, PageSizeBytes, MinBlockBytes, MaxSuballocationSize, AllocateFlags);
    if (!NewPool->Initialize())
    {
        delete NewPool;
        return false;
    }

    Pools.Add(NewPool);
    return NewPool->TryAllocate(SizeInBytes, UsedAlignment, OutStorage);
}

FVulkanTextureAllocator::FVulkanTextureAllocator(FVulkanDevice* InDevice, uint64 InDefaultPageSizeBytes)
    : FVulkanDeviceChild(InDevice)
    , DefaultPageSizeBytes(InDefaultPageSizeBytes)
    , PoolsCS()
{
    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
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

    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
    {
        if (Pools[Index])
        {
            Pools[Index]->CleanUp();
        }
    }
}

void FVulkanTextureAllocator::ReleasePools()
{
    for (uint32 Index = 0; Index < TexturePoolClassCount; ++Index)
    {
        if (Pools[Index])
        {
            delete Pools[Index];
            Pools[Index] = nullptr;
        }
    }
}

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

bool FVulkanTextureAllocator::TryAllocate(VkImage Image, VkMemoryPropertyFlags MemoryProperties, VkImageUsageFlags UsageFlags, VkMemoryAllocateFlags AllocateFlags, FVulkanMemoryStorage& OutStorage)
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    VkMemoryDedicatedRequirements DedicatedRequirements = {};
    DedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;

    VkMemoryRequirements2 MemoryRequirements2 = {};
    MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

    VkImageMemoryRequirementsInfo2 ImageRequirementsInfo = {};
    ImageRequirementsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
    ImageRequirementsInfo.image = Image;

    FVulkanStructChain RequirementsChain(MemoryRequirements2);
    RequirementsChain.AddNext(DedicatedRequirements);

    vkGetImageMemoryRequirements2(VulkanDevice, &ImageRequirementsInfo, &MemoryRequirements2);

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

        FVulkanStructChain AllocateChain(AllocateInfo);
        AllocateChain.AddNext(AllocateFlagsInfo);
        AllocateChain.AddNext(DedicatedAllocateInfo);

        VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
        VkResult Result = vkAllocateMemory(VulkanDevice, &AllocateInfo, nullptr, &DedicatedMemory);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: Dedicated vkAllocateMemory failed");
            return false;
        }

        OutStorage.Reset();
        OutStorage.SetMemory(DedicatedMemory);
        OutStorage.SetMemoryOffset(0);
        OutStorage.SetSize(MemReqs.size);
        OutStorage.SetStorageType(EVulkanMemoryStorageType::Dedicated);

        if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanTextureAllocator: Using dedicated allocation for image (Size=%llu)", MemReqs.size);
    }
        return true;
    }

    const ETexturePoolClass PoolClass = ClassifyTexture(UsageFlags, MemReqs.alignment);
    const uint32 PoolIndex = static_cast<uint32>(PoolClass);

    bool bPoolAllocated = false;
    if (PoolIndex < TexturePoolClassCount)
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
            bPoolAllocated = Pools[PoolIndex]->TryAllocate(MemReqs.size, MemReqs.alignment, OutStorage);
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

    FVulkanStructChain AllocateChain(AllocateInfo);
    AllocateChain.AddNext(AllocateFlagsInfo);

    VkDeviceMemory DedicatedMemory = VK_NULL_HANDLE;
    VkResult Result = vkAllocateMemory(VulkanDevice, &AllocateInfo, nullptr, &DedicatedMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanTextureAllocator: Fallback dedicated vkAllocateMemory failed (Size=%llu)", MemReqs.size);
        return false;
    }

    OutStorage.Reset();
    OutStorage.SetMemory(DedicatedMemory);
    OutStorage.SetMemoryOffset(0);
    OutStorage.SetSize(MemReqs.size);
    OutStorage.SetStorageType(EVulkanMemoryStorageType::Dedicated);

    if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanTextureAllocator: Pool suballocation failed, using dedicated allocation (Size=%llu)", MemReqs.size);
    }

    return true;
}

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
    const VkMemoryAllocateFlags AllocateFlags = 0;
    const VkBufferUsageFlags TransferSrcUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    const VkBufferUsageFlags UniformUsage     = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    SmallAllocator = new FVulkanMultiBuddyAllocator(GetDevice(), PageSizeBytes, DefaultAlignment, MemoryTypeIndex, AllocateFlags, TransferSrcUsage);
    if (!SmallAllocator->Initialize())
    {
        Destroy();
        return false;
    }

    static constexpr uint64 LARGE_PAGE_SIZE = 16ull * 1024ull * 1024ull;
    LargeAllocator = new FVulkanMultiBuddyAllocator(GetDevice(), LARGE_PAGE_SIZE, DefaultAlignment, MemoryTypeIndex, AllocateFlags, TransferSrcUsage);
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

void* FVulkanUploadHeapAllocator::Allocate(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    const uint64 UsedAlignment = Alignment ? Alignment : DefaultAlignment;

    if (SizeInBytes <= SmallThreshold)
    {
        if (SmallAllocator && SmallAllocator->TryAllocate(SizeInBytes, UsedAlignment, OutStorage))
        {
            return OutStorage.GetMappedBaseAddress();
        }
    }
    else
    {
        if (LargeAllocator && LargeAllocator->TryAllocate(SizeInBytes, UsedAlignment, OutStorage))
        {
            return OutStorage.GetMappedBaseAddress();
        }
    }

    return AllocateOversized(SizeInBytes, UsedAlignment, OutStorage);
}

void* FVulkanUploadHeapAllocator::AllocateOversized(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    VkDevice VulkanDevice = GetDevice()->GetVkDevice();

    const uint64 AlignedSize = Math::AlignUp<uint64>(SizeInBytes, Math::Max<uint64>(Alignment, 16ull));

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.size        = AlignedSize;
    BufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer Buffer = VK_NULL_HANDLE;
    VkResult Result = vkCreateBuffer(VulkanDevice, &BufferCreateInfo, nullptr, &Buffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkCreateBuffer failed (Size=%llu)", AlignedSize);
        return nullptr;
    }

    VkMemoryRequirements MemReqs = {};
    vkGetBufferMemoryRequirements(VulkanDevice, Buffer, &MemReqs);

    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = MemReqs.size;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;

    VkDeviceMemory DeviceMemory = VK_NULL_HANDLE;
    Result = vkAllocateMemory(VulkanDevice, &AllocateInfo, nullptr, &DeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkAllocateMemory failed (Size=%llu)", MemReqs.size);
        return nullptr;
    }

    Result = vkBindBufferMemory(VulkanDevice, Buffer, DeviceMemory, 0);
    if (VULKAN_FAILED(Result))
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        vkFreeMemory(VulkanDevice, DeviceMemory, nullptr);
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkBindBufferMemory failed");
        return nullptr;
    }

    void* MappedMemory = nullptr;
    Result = vkMapMemory(VulkanDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, &MappedMemory);
    if (VULKAN_FAILED(Result))
    {
        vkDestroyBuffer(VulkanDevice, Buffer, nullptr);
        vkFreeMemory(VulkanDevice, DeviceMemory, nullptr);
        VULKAN_ERROR_CRITICAL("FVulkanUploadHeapAllocator: Oversized vkMapMemory failed");
        return nullptr;
    }

    OutStorage.Reset();
    OutStorage.SetMemory(DeviceMemory);
    OutStorage.SetMemoryOffset(0);
    OutStorage.SetBackingBuffer(Buffer);
    OutStorage.SetBufferOffset(0);
    OutStorage.SetMappedBaseAddress(MappedMemory);
    OutStorage.SetSize(MemReqs.size);
    OutStorage.SetStorageType(EVulkanMemoryStorageType::Dedicated);

    if (CVarVulkanLogMemoryAllocations.GetValue())
    {
        VULKAN_INFO("FVulkanUploadHeapAllocator: Oversized upload allocation (Size=%llu)", MemReqs.size);
    }

    return MappedMemory;
}

void* FVulkanUploadHeapAllocator::AllocateConstants(uint64 SizeInBytes, uint64 Alignment, FVulkanMemoryStorage& OutStorage)
{
    const uint64 ConstantsAlignment = GetDevice()->GetPhysicalDevice()->GetProperties().limits.minUniformBufferOffsetAlignment;
    const uint64 UsedAlignment = Alignment ? Alignment : ConstantsAlignment;

    if (ConstantsAllocator && ConstantsAllocator->TryAllocate(SizeInBytes, UsedAlignment, OutStorage))
    {
        return OutStorage.GetMappedBaseAddress();
    }

    return nullptr;
}
