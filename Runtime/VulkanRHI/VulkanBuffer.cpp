#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"

#include "Core/Math/Math.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanBuffer

CVulkanBuffer::CVulkanBuffer(CVulkanDevice* InDevice, const CRHIBufferDesc& InBufferDesc)
    : CRHIBuffer(InBufferDesc)
    , CVulkanDeviceObject(InDevice)
    , Buffer(VK_NULL_HANDLE)
    , DeviceMemory(VK_NULL_HANDLE)
{
}

CVulkanBuffer::~CVulkanBuffer()
{
    if (VULKAN_CHECK_HANDLE(Buffer))
    {
        vkDestroyBuffer(GetDevice()->GetVkDevice(), Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(DeviceMemory))
    {
        GetDevice()->FreeMemory(&DeviceMemory);
    }
}
    
bool CVulkanBuffer::Initialize()
{
    CVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    VkBufferCreateInfo BufferCreateInfo;
    CMemory::Memzero(&BufferCreateInfo);

    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = BufferDesc.SizeInBytes;

    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetDeviceProperties();;
    const VkPhysicalDeviceLimits&     DeviceLimits     = DeviceProperties.limits;
    RequiredAlignment = 1u;
    
    BufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
#if VK_KHR_buffer_device_address
    if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }
#endif

    if (BufferDesc.Flags & BufferFlag_VertexBuffer)
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        RequiredAlignment = NMath::Max<uint32>(RequiredAlignment, 1LLU);
    }
    if (BufferDesc.Flags & BufferFlag_IndexBuffer)
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        RequiredAlignment = NMath::Max<uint32>(RequiredAlignment, 1LLU);
    }
    if (BufferDesc.Flags & BufferFlag_ConstantBuffer)
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        RequiredAlignment = NMath::Max<uint32>(RequiredAlignment, DeviceLimits.minUniformBufferOffsetAlignment);
    }
    if ((BufferDesc.Flags & BufferFlag_UAV) && (BufferDesc.Flags & BufferFlag_SRV))
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        RequiredAlignment = NMath::Max<uint32>(RequiredAlignment, DeviceLimits.minStorageBufferOffsetAlignment);
    }
    /*if (BufferDesc.Flags & )
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        RequiredAlignment = NMath::Max<uint32>(RequiredAlignment, 1LLU);
    }*/
    
    VkResult Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &BufferCreateInfo, nullptr, &Buffer);
    VULKAN_CHECK_RESULT(Result, "Failed to create Buffer");

    bool bUseDedicatedAllocation = false;
    
    VkMemoryRequirements MemoryRequirements;
    if (CVulkanDedicatedAllocationKHR::IsEnabled())
    {
#if (VK_KHR_get_memory_requirements2) && (VK_KHR_dedicated_allocation)
        VkMemoryDedicatedRequirementsKHR MemoryDedicatedRequirements;
        CMemory::Memzero(&MemoryDedicatedRequirements);
        
        MemoryDedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;
        MemoryDedicatedRequirements.pNext = nullptr;
        
        VkBufferMemoryRequirementsInfo2KHR BufferMemoryRequirementsInfo;
        BufferMemoryRequirementsInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2_KHR;
        BufferMemoryRequirementsInfo.pNext  = nullptr;
        BufferMemoryRequirementsInfo.buffer = Buffer;
        
        VkMemoryRequirements2KHR MemoryRequirements2;
        CMemory::Memzero(&MemoryRequirements2);
        
        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;
        MemoryRequirements2.pNext = &MemoryDedicatedRequirements;

        vkGetBufferMemoryRequirements2KHR(GetDevice()->GetVkDevice(), &BufferMemoryRequirementsInfo, &MemoryRequirements2);
        MemoryRequirements = MemoryRequirements2.memoryRequirements;
        
        bUseDedicatedAllocation = (MemoryDedicatedRequirements.requiresDedicatedAllocation != VK_FALSE) || (MemoryDedicatedRequirements.prefersDedicatedAllocation != VK_FALSE);
#endif
    }
    else
    {
        vkGetBufferMemoryRequirements(GetDevice()->GetVkDevice(), Buffer, &MemoryRequirements);
    }
    
    VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (BufferDesc.IsDynamic())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (BufferDesc.IsReadBack())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    
    const int32 MemoryTypeIndex = PhysicalDevice->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    VULKAN_CHECK(MemoryTypeIndex != UINT32_MAX, "No suitable memory type");

    VkMemoryAllocateInfo AllocateInfo;
    CMemory::Memzero(&AllocateInfo);

    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize  = MemoryRequirements.size;

    CVulkanStructureHelper AllocationInfoHelper(AllocateInfo);
    
#if VK_KHR_buffer_device_address
    VkMemoryAllocateFlagsInfo AllocateFlagsInfo;
    CMemory::Memzero(&AllocateFlagsInfo);

    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        AllocationInfoHelper.AddNext(AllocateFlagsInfo);
    }
#endif
    
#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
    CMemory::Memzero(&DedicatedAllocateInfo);
    
    DedicatedAllocateInfo.sType  = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
    DedicatedAllocateInfo.buffer = Buffer;
    
    if (bUseDedicatedAllocation && CVulkanDedicatedAllocationKHR::IsEnabled())
    {
        AllocationInfoHelper.AddNext(DedicatedAllocateInfo);
    }
#endif

    const bool bResult = GetDevice()->AllocateMemory(AllocateInfo, &DeviceMemory);
    VULKAN_CHECK(bResult, "Failed to allocate memory");

    Result = vkBindBufferMemory(GetDevice()->GetVkDevice(), Buffer, DeviceMemory, 0);
    VULKAN_CHECK_RESULT(Result, "Failed to bind Buffer-DeviceMemory");

#if VK_KHR_buffer_device_address
    VkBufferDeviceAddressInfo DeviceAdressInfo;
    CMemory::Memzero(&DeviceAdressInfo);

    DeviceAdressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    DeviceAdressInfo.pNext  = nullptr;
    DeviceAdressInfo.buffer = Buffer;

    if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        DeviceAddress = vkGetBufferDeviceAddressKHR(GetDevice()->GetVkDevice(), &DeviceAdressInfo);
        VULKAN_CHECK(DeviceAddress == 0, "vkGetBufferDeviceAddressKHR returned nullptr");
    }
#endif

    return true;
}
