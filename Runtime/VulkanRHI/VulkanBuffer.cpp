#include "Core/Math/Math.h"
#include "Core/Templates/NumericLimits.h"
#include "VulkanRHI/VulkanRHI.h"
#include "VulkanRHI/VulkanBuffer.h"
#include "VulkanRHI/VulkanDevice.h"
#include "VulkanRHI/VulkanCommandContext.h"
#include "RHI/RHIStats.h"

FVulkanBufferRHI::FVulkanBufferRHI(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FVulkanResource(InDevice)
    , OwnedBuffer(VK_NULL_HANDLE)
    , RequiredAlignment(0)
    , DebugName()
{
}

void* FVulkanBufferRHI::GetRHINativeHandle() const
{
    return reinterpret_cast<void*>(GetVkBuffer());
}

FRHIDescriptorHandle FVulkanBufferRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

FVulkanBufferRHI::~FVulkanBufferRHI()
{
#if VULKAN_ENABLE_STATS
    const int64 AllocatedSize = static_cast<int64>(MemoryStorage.GetSize());
    if (AllocatedSize > 0)
    {
        if (Desc.IsVertexBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_VertexBufferMemory, AllocatedSize);
        }
        else if (Desc.IsIndexBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_IndexBufferMemory, AllocatedSize);
        }
        else if (Desc.IsConstantBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_ConstantBufferMemory, AllocatedSize);
        }
        else if (Desc.IsShaderResourceBuffer() || Desc.IsUnorderedAccessBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_StructuredBufferMemory, AllocatedSize);
        }
        else
        {
            STAT_SUBTRACT(STAT_RHI_MiscBufferMemory, AllocatedSize);
        }

        if (Desc.IsReadBack())
        {
            STAT_SUBTRACT(STAT_RHI_ReadbackMemory, AllocatedSize);
        }
        if (Desc.IsDynamic() || Desc.IsTransient())
        {
            STAT_SUBTRACT(STAT_RHI_UploadMemory, AllocatedSize);
        }
    }
#endif

    if (OwnedBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(GetDevice()->GetVkDevice(), OwnedBuffer, nullptr);
        OwnedBuffer = VK_NULL_HANDLE;
    }
}

bool FVulkanBufferRHI::Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData)
{
    FVulkanPhysicalDevice* PhysicalDevice = GetDevice()->GetPhysicalDevice();

    const VkPhysicalDeviceProperties& DeviceProperties = PhysicalDevice->GetProperties();
    RequiredAlignment = 1u;

    VkBufferUsageFlags UsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkMemoryAllocateFlags AllocateFlags = 0;
    if (Desc.IsDefault())
    {
        AllocateFlags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        UsageFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }

    const bool bIsRayTracingSupported = GVulkanSupportsAccelerationStructures;
    if (Desc.IsVertexBuffer())
    {
        UsageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    #if VK_KHR_acceleration_structure
        if (bIsRayTracingSupported)
        {
            UsageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }
    #endif

        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, 1LLU);
    }
    if (Desc.IsIndexBuffer())
    {
        UsageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    #if VK_KHR_acceleration_structure
        if (bIsRayTracingSupported)
        {
            UsageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }
    #endif

        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, 1LLU);
    }
    if (Desc.IsConstantBuffer())
    {
        UsageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minUniformBufferOffsetAlignment);
    }
    if (Desc.IsUnorderedAccessBuffer() || Desc.IsShaderResourceBuffer())
    {
        UsageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minStorageBufferOffsetAlignment);
    }
    if (Desc.IsShaderResourceBuffer())
    {
        UsageFlags |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minTexelBufferOffsetAlignment);
    }
    if (Desc.IsUnorderedAccessBuffer())
    {
        UsageFlags |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        RequiredAlignment = Math::Max<VkDeviceSize>(RequiredAlignment, DeviceProperties.limits.minTexelBufferOffsetAlignment);
    }

    const VkDeviceSize AlignedSize = Math::AlignUp(static_cast<VkDeviceSize>(Desc.Size), RequiredAlignment);

    VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (Desc.IsDynamic() || Desc.IsTransient())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else if (Desc.IsReadBack())
    {
        MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(MemoryProperties, UsageFlags, AllocateFlags, AlignedSize, RequiredAlignment, MemoryStorage))
    {
        VULKAN_ERROR_CRITICAL("Failed to allocate buffer memory (Size=%llu, UsageFlags=0x%x)", AlignedSize, UsageFlags);
        return false;
    }

    if (!MemoryStorage.IsSuballocated())
    {
        VkBufferCreateInfo BufferCreateInfo = {};
        BufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferCreateInfo.size        = AlignedSize;
        BufferCreateInfo.usage       = UsageFlags;
        BufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult Result = vkCreateBuffer(GetDevice()->GetVkDevice(), &BufferCreateInfo, nullptr, &OwnedBuffer);
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("Failed to create dedicated buffer");
            return false;
        }

        Result = vkBindBufferMemory(GetDevice()->GetVkDevice(), OwnedBuffer, MemoryStorage.GetMemory(), MemoryStorage.GetMemoryOffset());
        if (VULKAN_FAILED(Result))
        {
            VULKAN_ERROR_CRITICAL("Failed to bind dedicated buffer memory");
            return false;
        }

        if (AllocateFlags & VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo AddressInfo = {};
            AddressInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            AddressInfo.buffer = OwnedBuffer;
            MemoryStorage.SetDeviceAddress(vkGetBufferDeviceAddress(GetDevice()->GetVkDevice(), &AddressInfo));
        }
    }

    BufferState.SetState(
        FVulkanRHI::ResourceStateToAccessFlags(InInitialAccess),
        FVulkanRHI::ResourceStateToPipelineStageFlags(InInitialAccess));

    if (InInitialData)
    {
        if (Desc.IsDynamic() || Desc.IsTransient())
        {
            void* BufferData = MemoryStorage.GetMappedBaseAddress();
            if (!BufferData)
            {
                VULKAN_ERROR_CRITICAL("Failed to get mapped memory for buffer");
                return false;
            }

            FMemory::Memcpy(BufferData, InInitialData, Desc.Size);
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBufferState(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);

            if (InInitialAccess != EResourceAccess::CopyDest)
            {
                InCommandContext->TransitionBufferState(this, EResourceAccess::CopyDest, InInitialAccess);
            }

            InCommandContext->FinishContext();
        }
    }
    
    return true;
}

void FVulkanBufferRHI::SetDebugName(const FString& InName)
{
    VkBuffer BufferHandle = GetVkBuffer();
    if (BufferHandle != VK_NULL_HANDLE)
    {
        VulkanSetObjectName(GetDevice()->GetVkDevice(), *InName, BufferHandle, VK_OBJECT_TYPE_BUFFER);
    }

    DebugName = InName;
}

void FVulkanBufferRHI::GetDebugName(FString& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FVulkanBufferRHI::Map(uint64 Offset, uint64 Size)
{
    if (!MemoryStorage.IsValid())
    {
        return nullptr;
    }

    if (!Desc.IsDynamic() && !Desc.IsReadBack() && !Desc.IsTransient())
    {
        FString DebugNameStr;
        GetDebugName(DebugNameStr);
        VULKAN_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *DebugNameStr);
        return nullptr;
    }

    uint8* Mapped = static_cast<uint8*>(MemoryStorage.GetMappedBaseAddress());
    if (!Mapped)
    {
        return nullptr;
    }

    CHECK(Offset <= Desc.Size);
    uint64 MapSize = Size;
    if (MapSize == UINT64_MAX)
    {
        MapSize = Desc.Size - Offset;
    }

    if (Desc.IsReadBack())
    {
        VkMappedMemoryRange Range = {};
        Range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.pNext  = nullptr;
        Range.memory = MemoryStorage.GetMemory();
        Range.offset = MemoryStorage.GetMemoryOffset() + Offset;
        Range.size   = MapSize;
        vkInvalidateMappedMemoryRanges(GetDevice()->GetVkDevice(), 1, &Range);
    }

    return Mapped + Offset;
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

void FVulkanBufferRHI::Unmap(uint64 Offset, uint64 Size)
{
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
