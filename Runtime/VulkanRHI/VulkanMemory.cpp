#include "VulkanMemory.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<int32> CVarMemoryHeapSize(
    "VulkanRHI.MemoryHeapSize",
    "The size of each VulkanMemoryHeap (MB)",
    128);

FVulkanMemoryHeap::FVulkanMemoryHeap(FVulkanDevice* InDevice, VkMemoryAllocateFlags InAllocationFlags, uint32 InHeapIndex, uint32 InMemoryIndex)
    : FVulkanDeviceObject(InDevice)
    , SizeInBytes(0)
    , AllocationFlags(InAllocationFlags)
    , MemoryIndex(InMemoryIndex)
    , HeapIndex(InHeapIndex)
    , Head(nullptr)
    , HostMemory(nullptr)
    , DeviceMemory(VK_NULL_HANDLE)
    , MappingCount(0)
    , HeapCS()
#ifdef DEBUG_BUILD
    , AllBlocks()
#endif
{
    VULKAN_INFO("New FVulkanMemoryHeap HeapIndex=%d MemoryIndex=%d", InHeapIndex, InMemoryIndex);
}

FVulkanMemoryHeap::~FVulkanMemoryHeap()
{
    CHECK(ValidateBlock(Head));
    CHECK(ValidateNoOverlap());

#ifdef DEBUG_BUILD
    if (Head->Next != nullptr)
    {
        LOG_WARNING("Memoryleak detected, Head->Next is not nullptr");
    }

    if (Head->Previous != nullptr)
    {
        LOG_WARNING("Memoryleak detected, Head->Previous is not nullptr");
    }
#endif

    FVulkanMemoryBlock* Iterator = Head;
    while (Iterator != nullptr)
    {
        FVulkanMemoryBlock* Block = Iterator;
        Iterator = Block->Next;
        SAFE_DELETE(Block);
    }

    VkDevice VulkanDevice = GetDevice()->GetVkDevice();
    if (MappingCount > 0)
    {
        vkUnmapMemory(VulkanDevice, DeviceMemory);
        DeviceMemory = VK_NULL_HANDLE;
        HostMemory   = nullptr;
        MappingCount = 0;
    }

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    MemoryManager.FreeMemory(DeviceMemory);
}

bool FVulkanMemoryHeap::Initialize(uint64 InSizeInBytes)
{
    SCOPED_LOCK(HeapCS);

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateMemoryDedicated(DeviceMemory, AllocationFlags, InSizeInBytes, MemoryIndex))
    {
        VULKAN_ERROR("Failed to allocate memory");
        return false;
    }
    else
    {
        Head = new FVulkanMemoryBlock();
        Head->Page             = this;
        Head->TotalSizeInBytes = InSizeInBytes;
        Head->SizeInBytes      = InSizeInBytes;

        SizeInBytes = InSizeInBytes;

    #ifdef DEBUG_BUILD
        AllBlocks.Emplace(Head);
    #endif
        return true;
    }
}

bool FVulkanMemoryHeap::Allocate(FVulkanMemoryAllocation& OutAllocation, VkDeviceSize InSizeInBytes, VkDeviceSize Alignment, VkDeviceSize PageGranularity)
{
    SCOPED_LOCK(HeapCS);

    uint64 Padding      = 0;
    uint64 PaddedOffset = 0;

    // Find a suitable block
    FVulkanMemoryBlock* BestFitBlock = nullptr;
    CHECK(ValidateBlock(Head));

    for (FVulkanMemoryBlock* Iterator = Head; Iterator != nullptr; Iterator = Iterator->Next)
    {
        if (!Iterator->bIsFree)
        {
            continue;
        }

        if (Iterator->SizeInBytes < InSizeInBytes)
        {
            continue;
        }

        PaddedOffset = FMath::AlignUp(Iterator->Offset, Alignment);
        if (PageGranularity > 1)
        {
            FVulkanMemoryBlock* Next     = Iterator->Next;
            FVulkanMemoryBlock* Previous = Iterator->Previous;

            if (Previous)
            {
                if (IsAliasing(Previous->Offset, Previous->TotalSizeInBytes, PaddedOffset, PageGranularity))
                {
                    PaddedOffset = FMath::AlignUp(PaddedOffset, PageGranularity);
                }
            }

            if (Next)
            {
                if (IsAliasing(PaddedOffset, InSizeInBytes, Next->Offset, PageGranularity))
                {
                    continue;
                }
            }
        }

        Padding = PaddedOffset - Iterator->Offset;
        if (Iterator->SizeInBytes >= (InSizeInBytes + Padding))
        {
            BestFitBlock = Iterator;
            break;
        }
    }

    if (BestFitBlock == nullptr)
    {
        OutAllocation.Reset();
        return false;
    }

    // Divide block
    const uint64 PaddedSizeInBytes = Padding + InSizeInBytes;
    if (BestFitBlock->SizeInBytes > PaddedSizeInBytes)
    {
        FVulkanMemoryBlock* NewBlock = new FVulkanMemoryBlock();
        NewBlock->Offset           = BestFitBlock->Offset + PaddedSizeInBytes;
        NewBlock->Page             = this;
        NewBlock->Next             = BestFitBlock->Next;
        NewBlock->Previous         = BestFitBlock;
        NewBlock->SizeInBytes      = BestFitBlock->SizeInBytes - PaddedSizeInBytes;
        NewBlock->TotalSizeInBytes = NewBlock->SizeInBytes;
        NewBlock->bIsFree          = true;

        if (BestFitBlock->Next)
        {
            BestFitBlock->Next->Previous = NewBlock;
        }

        BestFitBlock->Next = NewBlock;
        CHECK(ValidateChain());
        CHECK(ValidateBlock(NewBlock));

    #ifdef DEBUG_BUILD
        AllBlocks.Emplace(NewBlock);
    #endif
    }

    // Set new attributes of block
    BestFitBlock->SizeInBytes      = InSizeInBytes;
    BestFitBlock->TotalSizeInBytes = PaddedSizeInBytes;
    BestFitBlock->bIsFree          = false;

    CHECK(ValidateChain());
    CHECK(ValidateBlock(BestFitBlock));

    // Setup allocation
    OutAllocation.Memory = DeviceMemory;
    OutAllocation.Offset = PaddedOffset;
    OutAllocation.Block  = BestFitBlock;

    CHECK(ValidateNoOverlap());
    return true;
}

bool FVulkanMemoryHeap::Free(FVulkanMemoryAllocation& OutAllocation)
{
    SCOPED_LOCK(HeapCS);

    FVulkanMemoryBlock* Block = OutAllocation.Block;
    CHECK(Block != nullptr);
    CHECK(ValidateChain());
    CHECK(ValidateBlock(Block));
    Block->bIsFree = true;

    FVulkanMemoryBlock* Previous = Block->Previous;
    if (Previous)
    {
        if (Previous->bIsFree)
        {
            Previous->Next = Block->Next;
            if (Block->Next)
            {
                Block->Next->Previous = Previous;
            }

            Previous->SizeInBytes      += Block->TotalSizeInBytes;
            Previous->TotalSizeInBytes += Block->TotalSizeInBytes;

            SAFE_DELETE(Block);
            Block = Previous;
        }
    }

    CHECK(ValidateChain());

    FVulkanMemoryBlock* Next = Block->Next;
    if (Next)
    {
        if (Next->bIsFree)
        {
            if (Next->Next)
            {
                Next->Next->Previous = Block;
            }

            Block->Next              = Next->Next;
            Block->SizeInBytes      += Next->TotalSizeInBytes;
            Block->TotalSizeInBytes += Next->TotalSizeInBytes;

            SAFE_DELETE(Next);
        }
    }

    CHECK(ValidateChain());
    CHECK(ValidateNoOverlap());

    OutAllocation.Reset();
    return true;
}

void* FVulkanMemoryHeap::Map(const FVulkanMemoryAllocation& Allocation)
{
    SCOPED_LOCK(HeapCS);

    CHECK(Allocation.Block != nullptr);
    CHECK(ValidateBlock(Allocation.Block));

    if (MappingCount == 0)
    {
        vkMapMemory(GetDevice()->GetVkDevice(), DeviceMemory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&HostMemory));
    }

    CHECK(HostMemory != nullptr);

    MappingCount++;
    return HostMemory + Allocation.Offset;
}

void FVulkanMemoryHeap::Unmap(const FVulkanMemoryAllocation& Allocation)
{
    SCOPED_LOCK(HeapCS);

    CHECK(Allocation.Block != nullptr);
    CHECK(ValidateBlock(Allocation.Block));

    MappingCount--;
    if (MappingCount == 0)
    {
        vkUnmapMemory(GetDevice()->GetVkDevice(), DeviceMemory);
    }
}

void FVulkanMemoryHeap::SetName(const FString& InName)
{
    SCOPED_LOCK(HeapCS);

    if (VULKAN_CHECK_HANDLE(DeviceMemory))
    {
        FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), DeviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY);
        DebugName = InName;
    }
}

bool FVulkanMemoryHeap::IsAliasing(VkDeviceSize FirstBlockOffset, VkDeviceSize FirstBlockSize, VkDeviceSize SecondBlockOffset, VkDeviceSize PageGranularity)
{
    CHECK(FirstBlockSize > 0);
    CHECK(PageGranularity > 0);

    VkDeviceSize FirstBlockEnd        = FirstBlockOffset + (FirstBlockSize - 1);
    VkDeviceSize FirstBlockEndPage    = FirstBlockEnd & ~(PageGranularity - 1);
    VkDeviceSize SecondBlockStart     = SecondBlockOffset;
    VkDeviceSize SecondBlockStartPage = SecondBlockStart & ~(PageGranularity - 1);
    return FirstBlockEndPage >= SecondBlockStartPage;
}

bool FVulkanMemoryHeap::ValidateBlock(FVulkanMemoryBlock* Block) const
{
    FVulkanMemoryBlock* Iterator = Head;
    while (Iterator != nullptr)
    {
        if (Iterator == Block)
        {
            return true;
        }

        Iterator = Iterator->Next;
    }

#ifdef DEBUG_BUILD
    for (FVulkanMemoryBlock* BlockIterator : AllBlocks)
    {
        if (Block == BlockIterator)
        {
            DEBUG_BREAK();
        }
    }
#endif

    return false;
}

bool FVulkanMemoryHeap::ValidateNoOverlap() const
{
    FVulkanMemoryBlock* Iterator = Head;
    while (Iterator != nullptr)
    {
        if (Iterator->Next)
        {
            if ((Iterator->Offset + Iterator->TotalSizeInBytes) > (Iterator->Next->Offset))
            {
                VULKAN_WARNING("Overlap found");
                return false;
            }
        }

        Iterator = Iterator->Next;
    }

    return true;
}

bool FVulkanMemoryHeap::ValidateChain() const
{
    TArray<FVulkanMemoryBlock*> TraversedBlocks;

    // Traverse all the blocks and put them into an array in order
    FVulkanMemoryBlock* Iterator = Head;
    FVulkanMemoryBlock* Tail     = nullptr;
    while (Iterator != nullptr)
    {
        TraversedBlocks.Emplace(Iterator);
        Tail     = Iterator;
        Iterator = Iterator->Next;
    }

    // When we have reached the tail we start going backwards and check so
    // that the order in the array is the same as when we traversed forward
    while (Tail != nullptr)
    {
        // In case this fails our chain is not valid
        if (Tail != TraversedBlocks.LastElement())
        {
            return false;
        }

        TraversedBlocks.Pop();
        Tail = Tail->Previous;
    }

    return true;
}


FVulkanMemoryManager::FVulkanMemoryManager(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , MemoryHeaps()
    , DeviceProperties()
    , HeapSize(0)
    , NumAllocations(0)
    , ManagerCS()
{
    // Cache the DeviceProperties
    DeviceProperties = GetDevice()->GetPhysicalDevice()->GetDeviceProperties();
 
    // Calculate the HeapSize in bytes
    HeapSize = CVarMemoryHeapSize.GetValue() * 1024 * 1024;

    // List all the MemoryHeaps available
    const VkPhysicalDeviceMemoryProperties& MemoryProperties = GetDevice()->GetPhysicalDevice()->GetDeviceMemoryProperties();
    VULKAN_INFO("Current Device has the following MemoryHeaps:");
 
    for (uint32 Index = 0; Index < MemoryProperties.memoryHeapCount; Index++)
    {
        const VkMemoryHeap& MemoryHeap = MemoryProperties.memoryHeaps[Index];
        VULKAN_INFO("    MemoryHeap[%u] Size = %.2f MB", Index, static_cast<float>(MemoryHeap.size) / (1024.0f * 1024.0f));
    }
}

bool FVulkanMemoryManager::AllocateBufferMemory(VkBuffer Buffer, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, bool bForceDedicatedAllocation, FVulkanMemoryAllocation& OutAllocation)
{
    // We can force dedicated allocations
    bool bUseDedicatedAllocation = bForceDedicatedAllocation;
    
    // Check if the driver prefers us to use a dedicated allocation
    VkMemoryRequirements MemoryRequirements;
    
    // VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation (Core in 1.2)
    if (FVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VkMemoryDedicatedRequirementsKHR MemoryDedicatedRequirements;
        FMemory::Memzero(&MemoryDedicatedRequirements);
        MemoryDedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;
        
        VkMemoryRequirements2KHR MemoryRequirements2;
        FMemory::Memzero(&MemoryRequirements2);
        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

        // Add the proper pNext values in the structs
        FVulkanStructureHelper MemoryRequirements2Helper(MemoryRequirements2);
        MemoryRequirements2Helper.AddNext(MemoryDedicatedRequirements);

        VkBufferMemoryRequirementsInfo2KHR BufferMemoryRequirementsInfo;
        FMemory::Memzero(&BufferMemoryRequirementsInfo);
        BufferMemoryRequirementsInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR;
        BufferMemoryRequirementsInfo.buffer = Buffer;

        vkGetBufferMemoryRequirements2KHR(GetDevice()->GetVkDevice(), &BufferMemoryRequirementsInfo, &MemoryRequirements2);
        MemoryRequirements      = MemoryRequirements2.memoryRequirements;
        bUseDedicatedAllocation = MemoryDedicatedRequirements.requiresDedicatedAllocation != VK_FALSE || MemoryDedicatedRequirements.prefersDedicatedAllocation != VK_FALSE;
    }
    else
    {
        vkGetBufferMemoryRequirements(GetDevice()->GetVkDevice(), Buffer, &MemoryRequirements);
    }


    // Find the correct type of memory index
    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, PropertyFlags);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR("Did not find any suitable memory type");
        return false;
    }


    // Allocate memory, either pooled or dedicated
    bool bResult = false;
    if (bUseDedicatedAllocation)
    {
        VkMemoryAllocateInfo MemoryAllocateInfo;
        FMemory::Memzero(&MemoryAllocateInfo);

        FVulkanStructureHelper MemoryAllocateInfoHelper(MemoryAllocateInfo);
        MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        MemoryAllocateInfo.memoryTypeIndex = MemoryTypeIndex;
        MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
        
        VkMemoryAllocateFlagsInfo MemoryAllocateFlagsInfo;
        FMemory::Memzero(&MemoryAllocateFlagsInfo);

        MemoryAllocateInfoHelper.AddNext(MemoryAllocateFlagsInfo);
        MemoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        MemoryAllocateFlagsInfo.flags = AllocateFlags;
        
        // VK_KHR_dedicated_allocation (Core in 1.1)
        VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
        FMemory::Memzero(&DedicatedAllocateInfo);
        
        DedicatedAllocateInfo.sType  = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
        DedicatedAllocateInfo.buffer = Buffer;
        
        if (FVulkanDedicatedAllocationKHR::IsEnabled())
        {
            VULKAN_INFO("Using dedicated allocation for buffer");
            MemoryAllocateInfoHelper.AddNext(DedicatedAllocateInfo);
        }

        // Zero all parts of the block
        OutAllocation.Reset();

        // Allocate a dedicated piece of memory
        bResult = AllocateMemoryDedicated(OutAllocation.Memory, MemoryAllocateInfo);
        OutAllocation.bIsDedicated = true;
    }
    else
    {
        bResult = AllocateMemoryFromHeap(OutAllocation, AllocateFlags, MemoryRequirements.size, MemoryRequirements.alignment, MemoryTypeIndex);
        OutAllocation.bIsDedicated = false;
    }


    if (!bResult)
    {
        VULKAN_ERROR("Failed to allocte BufferMemory");
        return false;
    }


    // Bind the buffer to the allocated memory
    VkResult Result = vkBindBufferMemory(GetDevice()->GetVkDevice(), Buffer, OutAllocation.Memory, OutAllocation.Offset);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to bind BufferMemory");
        return false;
    }


    // We retrieve the device address for all buffers that supports it (TODO: Check if this has any performance impact and make this optional)
    if (AllocateFlags & VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)
    {
        // VK_KHR_buffer_device_address (Core in 1.2)
        VkBufferDeviceAddressInfo DeviceAdressInfo;
        FMemory::Memzero(&DeviceAdressInfo);

        DeviceAdressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        DeviceAdressInfo.buffer = Buffer;

        if (FVulkanBufferDeviceAddressKHR::IsEnabled())
        {
            OutAllocation.DeviceAddress = vkGetBufferDeviceAddressKHR(GetDevice()->GetVkDevice(), &DeviceAdressInfo);
            if (OutAllocation.DeviceAddress == 0)
            {
                VULKAN_ERROR("vkGetBufferDeviceAddressKHR returned nullptr");
                return false;
            }
        }
    }
    
    return true;
}

bool FVulkanMemoryManager::AllocateImageMemory(VkImage Image, VkMemoryPropertyFlags PropertyFlags, VkMemoryAllocateFlags AllocateFlags, bool bForceDedicatedAllocation, FVulkanMemoryAllocation& OutAllocation)
{
    // We can force dedicated allocations
    bool bUseDedicatedAllocation = bForceDedicatedAllocation;
    
    // Check if the driver prefers us to use a dedicated allocation
    VkMemoryRequirements MemoryRequirements;

    // VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation (Core in 1.2)
    if (FVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VkMemoryRequirements2KHR MemoryRequirements2;
        FMemory::Memzero(&MemoryRequirements2);

        FVulkanStructureHelper MemoryRequirements2Helper(MemoryRequirements2);
        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;

        VkMemoryDedicatedRequirementsKHR MemoryDedicatedRequirements;
        FMemory::Memzero(&MemoryDedicatedRequirements);

        // Add the proper pNext values in the structs
        MemoryRequirements2Helper.AddNext(MemoryDedicatedRequirements);
        MemoryDedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;

        VkImageMemoryRequirementsInfo2KHR ImageMemoryRequirementsInfo;
        FMemory::Memzero(&ImageMemoryRequirementsInfo);
        
        ImageMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
        ImageMemoryRequirementsInfo.image = Image;
        
        vkGetImageMemoryRequirements2KHR(GetDevice()->GetVkDevice(), &ImageMemoryRequirementsInfo, &MemoryRequirements2);
        MemoryRequirements      = MemoryRequirements2.memoryRequirements;
        bUseDedicatedAllocation = MemoryDedicatedRequirements.requiresDedicatedAllocation != VK_FALSE || MemoryDedicatedRequirements.prefersDedicatedAllocation != VK_FALSE;
    }
    else
    {
        vkGetImageMemoryRequirements(GetDevice()->GetVkDevice(), Image, &MemoryRequirements);
    }


    // Find the correct type of memory index
    const int32 MemoryTypeIndex = GetDevice()->GetPhysicalDevice()->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, PropertyFlags);
    if (MemoryTypeIndex == TNumericLimits<int32>::Max())
    {
        VULKAN_ERROR("No suitable memory type");
        return false;
    }


    // Allocate memory, either pooled or dedicated
    bool bResult = false;
    if (bUseDedicatedAllocation)
    {
        VkMemoryAllocateInfo MemoryAllocateInfo;
        FMemory::Memzero(&MemoryAllocateInfo);

        FVulkanStructureHelper MemoryAllocateInfoHelper(MemoryAllocateInfo);
        MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        MemoryAllocateInfo.memoryTypeIndex = MemoryTypeIndex;
        MemoryAllocateInfo.allocationSize  = MemoryRequirements.size;
        
        VkMemoryAllocateFlagsInfo MemoryAllocateFlagsInfo;
        FMemory::Memzero(&MemoryAllocateFlagsInfo);

        MemoryAllocateInfoHelper.AddNext(MemoryAllocateFlagsInfo);
        MemoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        MemoryAllocateFlagsInfo.flags = AllocateFlags;

        // VK_KHR_dedicated_allocation (Core in 1.1)
        VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
        FMemory::Memzero(&DedicatedAllocateInfo);

        DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
        DedicatedAllocateInfo.image = Image;

        if (FVulkanDedicatedAllocationKHR::IsEnabled())
        {
            VULKAN_INFO("Using dedicated allocation for Image");
            MemoryAllocateInfoHelper.AddNext(DedicatedAllocateInfo);
        }
        
        // Zero all parts of the block
        OutAllocation.Reset();

        // Allocate a dedicated piece of memory
        bResult = AllocateMemoryDedicated(OutAllocation.Memory, MemoryAllocateInfo);
        OutAllocation.bIsDedicated = true;
    }
    else
    {
        bResult = AllocateMemoryFromHeap(OutAllocation, AllocateFlags, MemoryRequirements.size, MemoryRequirements.alignment, MemoryTypeIndex);
        OutAllocation.bIsDedicated = false;
    }

    if (!bResult)
    {
        VULKAN_ERROR("Failed to allocte ImageMemory");
        return false;
    }


    // Lastly bind the memory to this resource
    VkResult Result = vkBindImageMemory(GetDevice()->GetVkDevice(), Image, OutAllocation.Memory, OutAllocation.Offset);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to bind ImageMemory");
        return false;
    }

    return true;
}

bool FVulkanMemoryManager::AllocateMemoryDedicated(VkDeviceMemory& OutDeviceMemory, const VkMemoryAllocateInfo& AllocateInfo)
{
    VkResult Result = vkAllocateMemory(GetDevice()->GetVkDevice(), &AllocateInfo, nullptr, &OutDeviceMemory);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("vkAllocateMemory failed");
        return false;
    }
    else
    {
        NumAllocations++;
    }

    VULKAN_INFO("[AllocateMemory] Allocated=%d Bytes, NumAllocations = %d/%d", AllocateInfo.allocationSize, NumAllocations.Load(), DeviceProperties.limits.maxMemoryAllocationCount);
    if (static_cast<uint32>(NumAllocations.Load()) > DeviceProperties.limits.maxMemoryAllocationCount)
    {
        VULKAN_WARNING("Too many allocations");
    }

    return true;
}

bool FVulkanMemoryManager::AllocateMemoryDedicated(VkDeviceMemory& OutDeviceMemory, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint32 MemoryIndex)
{
    VkMemoryAllocateInfo MemoryAllocateInfo;
    FMemory::Memzero(&MemoryAllocateInfo);

    FVulkanStructureHelper MemoryAllocateInfoHelper(MemoryAllocateInfo);
    MemoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    MemoryAllocateInfo.memoryTypeIndex = MemoryIndex;
    MemoryAllocateInfo.allocationSize  = SizeInBytes;
    
    VkMemoryAllocateFlagsInfo MemoryAllocateFlagsInfo;
    FMemory::Memzero(&MemoryAllocateFlagsInfo);

    MemoryAllocateInfoHelper.AddNext(MemoryAllocateFlagsInfo);
    MemoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    MemoryAllocateFlagsInfo.flags = AllocateFlags;
    return AllocateMemoryDedicated(OutDeviceMemory, MemoryAllocateInfo);
}

bool FVulkanMemoryManager::AllocateMemoryFromHeap(FVulkanMemoryAllocation& OutAllocation, VkMemoryAllocateFlags AllocateFlags, uint64 SizeInBytes, uint64 Alignment, uint32 InMemoryIndex)
{
    CHECK(SizeInBytes > 0);

    SCOPED_LOCK(ManagerCS);

    // Check if this size will be possible with this allocator
    VkDeviceSize AlignedSize = FMath::AlignUp(SizeInBytes, Alignment);
    if (AlignedSize >= HeapSize)
    {
        OutAllocation.Reset();
        return false;
    }

    if (!MemoryHeaps.IsEmpty())
    {
        for (FVulkanMemoryHeap* MemoryPage : MemoryHeaps)
        {
            CHECK(MemoryPage != nullptr);
            if (MemoryPage->GetMemoryIndex() == InMemoryIndex && MemoryPage->GetAllocationFlags() == AllocateFlags)
            {
                // Try and allocate otherwise we continue the search
                if (MemoryPage->Allocate(OutAllocation, SizeInBytes, Alignment, DeviceProperties.limits.bufferImageGranularity))
                {
                    return true;
                }
                else
                {
                    continue;
                }
            }
        }
    }

    FVulkanMemoryHeap* NewMemoryPage = new FVulkanMemoryHeap(GetDevice(), AllocateFlags, static_cast<uint32>(MemoryHeaps.Size()), InMemoryIndex);
    if (!NewMemoryPage->Initialize(HeapSize))
    {
        OutAllocation.Reset();
        SAFE_DELETE(NewMemoryPage);
        return false;
    }
    else
    {
        MemoryHeaps.Emplace(NewMemoryPage);
    }

    return NewMemoryPage->Allocate(OutAllocation, SizeInBytes, Alignment, DeviceProperties.limits.bufferImageGranularity);
}

bool FVulkanMemoryManager::Free(FVulkanMemoryAllocation& OutAllocation)
{
    SCOPED_LOCK(ManagerCS);

    FVulkanMemoryBlock* Block = OutAllocation.Block;
    if (Block)
    {
        CHECK(OutAllocation.bIsDedicated == false);
        
        FVulkanMemoryHeap* Page = Block->Page;
        CHECK(Page != nullptr);
        
        // Remove an empty heap
        const bool bResult = Page->Free(OutAllocation);
        if (Page->IsEmpty())
        {
            for (auto it = MemoryHeaps.begin(); it != MemoryHeaps.end(); it++)
            {
                if (*it == Page)
                {
                    MemoryHeaps.RemoveAt(it.GetIndex());
                    break;
                }
            }

            SAFE_DELETE(Page);
        }
        
        return bResult;
    }
    else
    {
        CHECK(OutAllocation.bIsDedicated == true);
        
        FreeMemory(OutAllocation.Memory);
        OutAllocation.Reset();
        return true;
    }
    
    return false;
}

void FVulkanMemoryManager::FreeMemory(VkDeviceMemory& OutDeviceMemory)
{
    vkFreeMemory(GetDevice()->GetVkDevice(), OutDeviceMemory, nullptr);
    OutDeviceMemory = VK_NULL_HANDLE;
    NumAllocations--;
    
    VULKAN_INFO("[FreeMemory] NumAllocations = %d/%d", NumAllocations.Load(), DeviceProperties.limits.maxMemoryAllocationCount);
}

void FVulkanMemoryManager::ReleaseMemoryHeaps()
{
    for (FVulkanMemoryHeap* MemoryPage : MemoryHeaps)
    {
        SAFE_DELETE(MemoryPage);
    }

    MemoryHeaps.Clear();
}

void* FVulkanMemoryManager::Map(const FVulkanMemoryAllocation& Allocation)
{
    SCOPED_LOCK(ManagerCS);

    if (!Allocation.bIsDedicated)
    {
        FVulkanMemoryBlock* Block = Allocation.Block;
        CHECK(Block != nullptr);

        FVulkanMemoryHeap* Page = Block->Page;
        CHECK(Page != nullptr);
        return Page->Map(Allocation);
    }
    else
    {
        // TODO: Mapping dedicated allocations are not ref-counted
        void* MappedMemory = nullptr;
        vkMapMemory(GetDevice()->GetVkDevice(), Allocation.Memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&MappedMemory));
        return MappedMemory;
    }
}

void FVulkanMemoryManager::Unmap(const FVulkanMemoryAllocation& Allocation)
{
    SCOPED_LOCK(ManagerCS);

    if (!Allocation.bIsDedicated)
    {
        FVulkanMemoryBlock* Block = Allocation.Block;
        CHECK(Block != nullptr);

        FVulkanMemoryHeap* Page = Block->Page;
        CHECK(Page != nullptr);
        return Page->Unmap(Allocation);
    }
    else
    {
        // TODO: Mapping dedicated allocations are not ref-counted
        vkUnmapMemory(GetDevice()->GetVkDevice(), Allocation.Memory);
    }
}
