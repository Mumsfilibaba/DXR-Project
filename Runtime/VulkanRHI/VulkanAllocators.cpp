#include "VulkanAllocators.h"
#include "VulkanDevice.h"
#include "Core/Misc/ConsoleManager.h"

TAutoConsoleVariable<int32> CVarMaxStagingAllocationSize(
    "Vulkan.MaxStagingAllocationSize",
    "The maximum size for a resource that uses a shared staging-buffer (MB)",
    8);


FVulkanUploadBuffer::FVulkanUploadBuffer(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , Buffer(VK_NULL_HANDLE)
    , BufferMemory(VK_NULL_HANDLE)
{
}

FVulkanUploadBuffer::~FVulkanUploadBuffer()
{
    if (VULKAN_CHECK_HANDLE(Buffer))
    {
        vkDestroyBuffer(GetDevice()->GetVkDevice(), Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(BufferMemory))
    {
        vkUnmapMemory(GetDevice()->GetVkDevice(), BufferMemory);
        GetDevice()->FreeMemory(BufferMemory);
    }
}

bool FVulkanUploadBuffer::Initialize(uint64 Size)
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    VkBufferCreateInfo BufferCreateInfo;
    FMemory::Memzero(&BufferCreateInfo);

    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = Size;
    BufferCreateInfo.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkResult Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &BufferCreateInfo, nullptr, &Buffer);
    VULKAN_CHECK_RESULT(Result, "Failed to create Buffer");

    bool bUseDedicatedAllocation = false;
    
    VkMemoryRequirements MemoryRequirements;
#if VK_KHR_get_memory_requirements2 && VK_KHR_dedicated_allocation
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
#endif
    {
        vkGetBufferMemoryRequirements(GetDevice()->GetVkDevice(), Buffer, &MemoryRequirements);
    }
    
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;    
    const int32 MemoryTypeIndex = PhysicalDevice->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    VULKAN_CHECK(MemoryTypeIndex != UINT32_MAX, "No suitable memory type");

    VkMemoryAllocateInfo AllocateInfo;
    FMemory::Memzero(&AllocateInfo);

    FVulkanStructureHelper AllocationInfoHelper(AllocateInfo);
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize  = MemoryRequirements.size;
    
#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
    FMemory::Memzero(&DedicatedAllocateInfo);
    
    DedicatedAllocateInfo.sType  = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
    DedicatedAllocateInfo.buffer = Buffer;
    
    if (bUseDedicatedAllocation && FVulkanDedicatedAllocationKHR::IsEnabled())
    {
        AllocationInfoHelper.AddNext(DedicatedAllocateInfo);
    }
#endif

    const bool bResult = GetDevice()->AllocateMemory(AllocateInfo, BufferMemory);
    VULKAN_CHECK(bResult, "Failed to allocate memory");

    Result = vkBindBufferMemory(GetDevice()->GetVkDevice(), Buffer, BufferMemory, 0);
    VULKAN_CHECK_RESULT(Result, "Failed to bind Buffer-DeviceMemory");

    Result = vkMapMemory(GetDevice()->GetVkDevice(), BufferMemory, 0, VK_WHOLE_SIZE, 0, &MappedMemory);
    VULKAN_CHECK_RESULT(Result, "Failed to map buffer-memory");
    
    if (!MappedMemory)
    {
        return false;
    }

    return true;
}

FVulkanUploadHeapAllocator::FVulkanUploadHeapAllocator(FVulkanDevice* InDevice)
    : FVulkanDeviceObject(InDevice)
    , BufferSize(0)
    , CurrentOffset(0)
    , BufferData(nullptr)
    , Buffer(nullptr)
{
}

FVulkanUploadHeapAllocator::~FVulkanUploadHeapAllocator()
{
}

FVulkanUploadAllocation FVulkanUploadHeapAllocator::Allocate(uint64 Size, uint64 Alignment)
{
    FVulkanUploadAllocation Allocation;
    
    FVulkanUploadBufferRef Buffer = new FVulkanUploadBuffer(GetDevice());
    if (!Buffer->Initialize(Size))
    {
        VULKAN_ERROR("Failed to create staging-buffer");
        return Allocation;
    }

    Allocation.Buffer = Buffer;
    Allocation.Offset = 0;
    Allocation.Memory = reinterpret_cast<uint8*>(Buffer->GetMappedMemory());
    return Allocation;
}
