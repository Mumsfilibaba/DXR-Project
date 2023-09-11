#include "VulkanRHI.h"
#include "VulkanBuffer.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanCommandContext.h"
#include "Core/Math/Math.h"
#include "Core/Templates/NumericLimits.h"

FVulkanBuffer::FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FVulkanDeviceObject(InDevice)
    , Buffer(VK_NULL_HANDLE)
    , DeviceMemory(VK_NULL_HANDLE)
{
}

FVulkanBuffer::~FVulkanBuffer()
{
    if (VULKAN_CHECK_HANDLE(Buffer))
    {
        vkDestroyBuffer(GetDevice()->GetVkDevice(), Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    if (VULKAN_CHECK_HANDLE(DeviceMemory))
    {
        GetDevice()->FreeMemory(DeviceMemory);
    }
}

bool FVulkanBuffer::Initialize(EResourceAccess InInitialAccess, const void* InInitialData)
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
    BufferCreateInfo.size                  = Desc.Size;

    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetDeviceProperties();
    RequiredAlignment = 1u;
    
    // TODO: Look into abstracting these flags
    BufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
#if VK_KHR_buffer_device_address
    if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }
#endif

    if (Desc.IsVertexBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        RequiredAlignment = FMath::Max<VkDeviceSize>(RequiredAlignment, 1LLU);
    }
    if (Desc.IsIndexBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        RequiredAlignment = FMath::Max<VkDeviceSize>(RequiredAlignment, 1LLU);
    }
    if (Desc.IsConstantBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        RequiredAlignment = FMath::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minUniformBufferOffsetAlignment);
    }
    if (Desc.IsUnorderedAccess() || Desc.IsShaderResource())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        RequiredAlignment = FMath::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minStorageBufferOffsetAlignment);
    }
    
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
    
    VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (Desc.IsDynamic())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (Desc.IsReadBack())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    
    const int32 MemoryTypeIndex = PhysicalDevice->FindMemoryTypeIndex(MemoryRequirements.memoryTypeBits, MemoryProperties);
    VULKAN_CHECK(MemoryTypeIndex != TNumericLimits<int32>::Max(), "No suitable memory type");

    VkMemoryAllocateInfo AllocateInfo;
    FMemory::Memzero(&AllocateInfo);

    FVulkanStructureHelper AllocationInfoHelper(AllocateInfo);
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize  = MemoryRequirements.size;
    
#if VK_KHR_buffer_device_address
    VkMemoryAllocateFlagsInfo AllocateFlagsInfo;
    FMemory::Memzero(&AllocateFlagsInfo);

    AllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    AllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        AllocationInfoHelper.AddNext(AllocateFlagsInfo);
    }
#endif
    
#if VK_KHR_dedicated_allocation
    VkMemoryDedicatedAllocateInfoKHR DedicatedAllocateInfo;
    FMemory::Memzero(&DedicatedAllocateInfo);
    
    DedicatedAllocateInfo.sType  = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
    DedicatedAllocateInfo.buffer = Buffer;
    
    if (bUseDedicatedAllocation && FVulkanDedicatedAllocationKHR::IsEnabled())
    {
        VULKAN_INFO("Using dedicated allocation for buffer");
        AllocationInfoHelper.AddNext(DedicatedAllocateInfo);
    }
#endif

    const bool bResult = GetDevice()->AllocateMemory(AllocateInfo, DeviceMemory);
    VULKAN_CHECK(bResult, "Failed to allocate memory");

    Result = vkBindBufferMemory(GetDevice()->GetVkDevice(), Buffer, DeviceMemory, 0);
    VULKAN_CHECK_RESULT(Result, "Failed to bind Buffer-DeviceMemory");

#if VK_KHR_buffer_device_address
    VkBufferDeviceAddressInfo DeviceAdressInfo;
    FMemory::Memzero(&DeviceAdressInfo);

    DeviceAdressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    DeviceAdressInfo.buffer = Buffer;

    if (GetDevice()->IsExtensionEnabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))
    {
        DeviceAddress = vkGetBufferDeviceAddressKHR(GetDevice()->GetVkDevice(), &DeviceAdressInfo);
        VULKAN_CHECK(DeviceAddress != 0, "vkGetBufferDeviceAddressKHR returned nullptr");
    }
#endif

    if (InInitialData)
    {
        if (Desc.IsDynamic())
        {
            void* BufferData = nullptr;
            VkDevice NativeDevice = GetDevice()->GetVkDevice();
            
            // Map buffer
            Result = vkMapMemory(NativeDevice, DeviceMemory, 0, VK_WHOLE_SIZE, 0, &BufferData);
            if (VULKAN_FAILED(Result) || !BufferData)
            {
                VULKAN_ERROR("Failed to map buffer memory");
                return false;
            }

            // Copy over relevant data
            FMemory::Memcpy(BufferData, InInitialData, Desc.Size);
            
            // Unmap buffer
            vkUnmapMemory(NativeDevice, DeviceMemory);
        }
        else
        {
            FVulkanCommandContext* Context = FVulkanRHI::GetRHI()->ObtainCommandContext();
            Context->RHIStartContext();

            Context->RHITransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            
            Context->RHIUpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);

            // NOTE: Transfer to the initial state
            if (InInitialAccess != EResourceAccess::CopyDest)
            {
                Context->RHITransitionBuffer(this, EResourceAccess::CopyDest, InInitialAccess);
            }

            Context->RHIFinishContext();
        }
    }
    
    return true;
}

void FVulkanBuffer::SetName(const FString& InName)
{
    FVulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), InName.GetCString(), Buffer, VK_OBJECT_TYPE_BUFFER);
    DebugName = InName;
}

FString FVulkanBuffer::GetName() const
{
    return DebugName;
}
