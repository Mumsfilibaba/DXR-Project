#include "Core/Math/Math.h"
#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandContext.h"

FVulkanBuffer::FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferInfo& InBufferDesc, EResourceAccess InInitialState)
    : FRHIBuffer(InBufferDesc)
    , FVulkanDeviceChild(InDevice)
    , Buffer(VK_NULL_HANDLE)
    , MemoryAllocation()
    , RequiredAlignment(0)
    , DebugName()
{
    EnableResourceStateTracking(InInitialState);
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

bool FVulkanBuffer::Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData)
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = Info.Size;

    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetProperties();
    RequiredAlignment = 1u;
    
    // TODO: Look into abstracting these flags
    BufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    // VK_KHR_buffer_device_address (Core in 1.2)
    VkMemoryAllocateFlags AllocateFlags = 0;
    if (Info.IsDefault())
    {
        AllocateFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    const bool bIsRayTracingSupported = GVulkanSupportsAccelerationStructures;
    if (Info.IsVertexBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    #if VK_KHR_acceleration_structure
        if (bIsRayTracingSupported)
        {
            BufferCreateInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }
    #endif

        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, 1LLU);
    }
    if (Info.IsIndexBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    #if VK_KHR_acceleration_structure
        if (bIsRayTracingSupported)
        {
            BufferCreateInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }
    #endif

        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, 1LLU);
    }
    if (Info.IsConstantBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minUniformBufferOffsetAlignment);
    }
    if (Info.IsUnorderedAccessBuffer() || Info.IsShaderResourceBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minStorageBufferOffsetAlignment);
    }
    
    // Setup the proper size
    BufferCreateInfo.size = Math::AlignUp(BufferCreateInfo.size, RequiredAlignment);

    VkResult Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &BufferCreateInfo, nullptr, &Buffer);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR_CRITICAL("Failed to create Buffer");
        return false;
    }
    
    // NOTE: We might need to call:
    //   vkInvalidateMappedMemoryRanges before reading (host <- device)
    //   vkFlushMappedMemoryRanges after writing(device <- host), if you ever write.
    VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (Info.IsDynamic())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (Info.IsReadBack())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    
    // Allocate memory based on the buffer
    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(Buffer, MemoryProperties, AllocateFlags, GVulkanForceDedicatedBufferAllocations, MemoryAllocation))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate buffer memory");
        return false;
    }
    
    if (InInitialData)
    {
        if (Info.IsDynamic())
        {
            // Map buffer
            void* BufferData = MemoryManager.Map(MemoryAllocation);
            if (!BufferData)
            {
                VULKAN_ERROR_CRITICAL("Failed to map buffer memory");
                return false;
            }

            // Copy over relevant data
            FMemory::Memcpy(BufferData, InInitialData, Info.Size);
            
            // Unmap buffer
            MemoryManager.Unmap(MemoryAllocation);
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Info.Size), InInitialData);

            // NOTE: Transfer to the initial state
            if (InInitialAccess != EResourceAccess::CopyDest)
            {
                InCommandContext->TransitionBuffer(this, EResourceAccess::CopyDest, InInitialAccess);
            }

            InCommandContext->FinishContext();
        }
    }
    
    return true;
}

void FVulkanBuffer::SetDebugName(const FString& InName)
{
    VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, Buffer, VK_OBJECT_TYPE_BUFFER);
    DebugName = InName;
}

FString FVulkanBuffer::GetDebugName() const
{
    return DebugName;
}

void* FVulkanBuffer::Map(uint64 Offset, uint64 Size)
{
    if (!MemoryAllocation.IsValid())
    {
        return nullptr;
    }

    if (!Info.IsDynamic() && !Info.IsReadBack())
    {
        VULKAN_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *GetDebugName());
        return nullptr;
    }

    CHECK(Offset <= Info.Size);
    uint64 MapSize = Size;
    if (MapSize == UINT64_MAX)
    {
        MapSize = Info.Size - Offset;
    }

    FVulkanDevice* VulkanDevice = GetDevice();
    FVulkanMemoryManager& MemoryManager = VulkanDevice->GetMemoryManager();
    uint8* Mapped = reinterpret_cast<uint8*>(MemoryManager.Map(MemoryAllocation));
    if (!Mapped)
    {
        return nullptr;
    }

    if (Info.IsReadBack())
    {
        VkMappedMemoryRange Range = {};
        Range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.pNext  = nullptr;
        Range.memory = MemoryAllocation.Memory;
        Range.offset = MemoryAllocation.Offset + Offset;
        Range.size   = MapSize;
        vkInvalidateMappedMemoryRanges(VulkanDevice->GetVkDevice(), 1, &Range);
    }

    return Mapped + Offset;
}

void FVulkanBuffer::EnableResourceStateTracking(EResourceAccess InitialState)
{
    if (!BufferState)
    {
        BufferState = MakeUniquePtr<FVulkanBufferState>();
    }

    FVulkanBufferState::FBufferState State;
    State.Access = ConvertResourceStateToAccessFlags(InitialState);
    State.Stage  = ConvertResourceStateToPipelineStageFlags(InitialState);
    BufferState->Enable(State);
}

void FVulkanBuffer::DisableResourceStateTracking()
{
    BufferState.Reset();
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FVulkanBuffer::Unmap(uint64 Offset, uint64 Size)
{
    if (!MemoryAllocation.IsValid())
    {
        return;
    }

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    MemoryManager.Unmap(MemoryAllocation);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
