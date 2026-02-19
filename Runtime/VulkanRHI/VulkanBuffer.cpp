#include "Core/Math/Math.h"
#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandContext.h"

FVulkanBufferRHI::FVulkanBufferRHI(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc, EResourceAccess InInitialState)
    : FRHIBuffer(InBufferDesc)
    , FVulkanDeviceChild(InDevice)
    , Buffer(VK_NULL_HANDLE)
    , MemoryAllocation()
    , RequiredAlignment(0)
    , DebugName()
{
    (void)InInitialState;
}

FVulkanBufferRHI::~FVulkanBufferRHI()
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

bool FVulkanBufferRHI::Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData)
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    VkBufferCreateInfo BufferCreateInfo = {};
    BufferCreateInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferCreateInfo.pNext                 = nullptr;
    BufferCreateInfo.flags                 = 0;
    BufferCreateInfo.pQueueFamilyIndices   = nullptr;
    BufferCreateInfo.queueFamilyIndexCount = 0;
    BufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    BufferCreateInfo.size                  = Desc.Size;

    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetProperties();
    RequiredAlignment = 1u;
    
    // TODO: Look into abstracting these flags
    BufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    
    // VK_KHR_buffer_device_address (Core in 1.2)
    VkMemoryAllocateFlags AllocateFlags = 0;
    if (Desc.IsDefault())
    {
        AllocateFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    const bool bIsRayTracingSupported = GVulkanSupportsAccelerationStructures;
    if (Desc.IsVertexBuffer())
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
    if (Desc.IsIndexBuffer())
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
    if (Desc.IsConstantBuffer())
    {
        BufferCreateInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minUniformBufferOffsetAlignment);
    }
    if (Desc.IsUnorderedAccessBuffer() || Desc.IsShaderResourceBuffer())
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
    if (Desc.IsDynamic())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (Desc.IsReadBack())
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
    
    // Clear non-dedicated buffers to zero to prevent reading undefined memory from shared allocations.
    if (!InInitialData && !MemoryAllocation.bIsDedicated)
    {
        InCommandContext->StartContext();
        
        InCommandContext->TransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
        InCommandContext->GetBarrierBatcher().FlushBarriers();
        
        // Fill buffer with zeros
        InCommandContext->GetCommandBuffer()->FillBuffer(Buffer, 0, Desc.Size, 0);
        
        // Transition to initial access state
        if (InInitialAccess != EResourceAccess::CopyDest)
        {
            InCommandContext->TransitionBuffer(this, EResourceAccess::CopyDest, InInitialAccess);
        }
        
        InCommandContext->FinishContext();
        InCommandContext->GetCommandQueue().WaitForCompletion();
    }
    
    if (InInitialData)
    {
        if (Desc.IsDynamic())
        {
            // Map buffer
            void* BufferData = MemoryManager.Map(MemoryAllocation);
            if (!BufferData)
            {
                VULKAN_ERROR_CRITICAL("Failed to map buffer memory");
                return false;
            }

            // Copy over relevant data
            FMemory::Memcpy(BufferData, InInitialData, Desc.Size);
            
            // Unmap buffer
            MemoryManager.Unmap(MemoryAllocation);
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);

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

void FVulkanBufferRHI::SetDebugName(const FString& InName)
{
    VulkanDebugUtilsEXT::SetObjectName(GetDevice()->GetVkDevice(), *InName, Buffer, VK_OBJECT_TYPE_BUFFER);
    DebugName = InName;
}

FString FVulkanBufferRHI::GetDebugName() const
{
    return DebugName;
}

void* FVulkanBufferRHI::Map(uint64 Offset, uint64 Size)
{
    if (!MemoryAllocation.IsValid())
    {
        return nullptr;
    }

    if (!Desc.IsDynamic() && !Desc.IsReadBack())
    {
        VULKAN_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *GetDebugName());
        return nullptr;
    }

    CHECK(Offset <= Desc.Size);
    
    uint64 MapSize = Size;
    if (MapSize == UINT64_MAX)
    {
        MapSize = Desc.Size - Offset;
    }

    FVulkanDevice* VulkanDevice = GetDevice();

    uint8* Mapped = reinterpret_cast<uint8*>(VulkanDevice->GetMemoryManager().Map(MemoryAllocation));
    if (!Mapped)
    {
        return nullptr;
    }

    if (Desc.IsReadBack())
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

void FVulkanBufferRHI::EnableStateTracking(EResourceAccess InitialState)
{
    const VkAccessFlags2        Access = FVulkanRHI::ResourceStateToAccessFlags(InitialState);
    const VkPipelineStageFlags2 Stage  = FVulkanRHI::ResourceStateToPipelineStageFlags(InitialState);

    BufferState = MakeUniquePtr<FVulkanBufferState>(Access, Stage);
}

void FVulkanBufferRHI::DisableStateTracking(FVulkanCommandContext* CommandContext)
{
    if (!BufferState)
    {
        return;
    }

    if (CommandContext)
    {
        CommandContext->RequireBufferState(this, EResourceAccess::Common);
    }

    BufferState.Reset();
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FVulkanBufferRHI::Unmap(uint64 Offset, uint64 Size)
{
    if (!MemoryAllocation.IsValid())
    {
        return;
    }

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    MemoryManager.Unmap(MemoryAllocation);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
