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
    , MemoryAllocation()
    , RequiredAlignment(0)
    , DebugName()
{
}

FVulkanBuffer::~FVulkanBuffer()
{
    FVulkanDevice* VulkanDevice = GetDevice();
    if (VULKAN_CHECK_HANDLE(Buffer))
    {
        vkDestroyBuffer(VulkanDevice->GetVkDevice(), Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    FVulkanMemoryManager& MemoryManager = VulkanDevice->GetMemoryManager();
    MemoryManager.Free(MemoryAllocation);
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
    if (FVulkanBufferDeviceAddressKHR::IsEnabled())
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
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create Buffer");
        return false;
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
    
    
    // Allocate memory based on the buffer
    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(Buffer, MemoryProperties, false, MemoryAllocation))
    {
        VULKAN_ERROR("Failed to allocate buffer memory");
        return false;
    }
    
    if (InInitialData)
    {
        if (Desc.IsDynamic())
        {
            // Map buffer
            void* BufferData = MemoryManager.Map(MemoryAllocation);
            if (!BufferData)
            {
                VULKAN_ERROR("Failed to map buffer memory");
                return false;
            }

            // Copy over relevant data
            FMemory::Memcpy(BufferData, InInitialData, Desc.Size);
            
            // Unmap buffer
            MemoryManager.Unmap(MemoryAllocation);
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
