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
		vkFreeMemory(GetDevice()->GetVkDevice(), DeviceMemory, nullptr);
		DeviceMemory = VK_NULL_HANDLE;
    }
}
	
bool CVulkanBuffer::Initialize()
{
    CVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    VkBufferCreateInfo BufferCreateInfo;
    CMemory::Memzero(&BufferCreateInfo);

    BufferCreateInfo.sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                  = nullptr;
    BufferCreateInfo.flags                  = 0;
    BufferCreateInfo.pQueueFamilyIndices    = nullptr;
    BufferCreateInfo.queueFamilyIndexCount  = 0;
    BufferCreateInfo.sharingMode            = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                   = BufferDesc.SizeInBytes;

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
    if ((BufferDesc.Flags & BufferFlag_UnorderedAccess) && (BufferDesc.Flags & BufferFlag_ShaderResource))
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

    VkMemoryRequirements MemoryRequirements;
    vkGetBufferMemoryRequirements(GetDevice()->GetVkDevice(), Buffer, &MemoryRequirements);

	VkMemoryPropertyFlags MemoryProperties = 0;
	if (BufferDesc.Flags & BufferFlag_Dynamic)
	{
		MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}
	else if (BufferDesc.Flags & BufferFlag_Default)
	{
		MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}
	
    const int32 MemoryTypeIndex = PhysicalDevice->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    
    VkMemoryAllocateInfo AllocateInfo;
    CMemory::Memzero(&AllocateInfo);

    AllocateInfo.sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex    = MemoryTypeIndex;
    AllocateInfo.allocationSize     = BufferDesc.SizeInBytes;

#if VK_KHR_buffer_device_address
    VkMemoryAllocateFlagsInfo AllocateFlagsInfo;
    CMemory::Memzero(&AllocateFlagsInfo);

    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
	{
		AllocateInfo.pNext = &AllocateFlagsInfo;
	}
	else
	{
		AllocateInfo.pNext = nullptr;
	}
#else
    AllocateInfo.pNext = nullptr;
#endif

    Result = vkAllocateMemory(GetDevice()->GetVkDevice(), &AllocateInfo, nullptr, &DeviceMemory);
    VULKAN_CHECK_RESULT(Result, "Failed to allocate memory");
	
	VULKAN_INFO(String("Allocated: ") + ToString(AllocateInfo.allocationSize) + " Bytes");

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
	}
	
    return true;
#else
    return true;
#endif
}
