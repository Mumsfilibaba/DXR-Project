#include "VulkanAllocators.h"
#include "VulkanDevice.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"

static TAutoConsoleVariable<int32> CVarMaxStagingAllocationSize(
    "VulkanRHI.MaxStagingAllocationSize",
    "The maximum size for a resource that uses a shared staging-buffer (MB)",
    8);


FVulkanUploadBuffer::FVulkanUploadBuffer(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , Buffer(VK_NULL_HANDLE)
    , MemoryAllocation()
{
}

FVulkanUploadBuffer::~FVulkanUploadBuffer()
{
    if (VULKAN_CHECK_HANDLE(Buffer))
    {
        vkDestroyBuffer(GetDevice()->GetVkDevice(), Buffer, nullptr);
        Buffer = VK_NULL_HANDLE;
    }

    if (MemoryAllocation.IsValid())
    {
        FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
        MemoryManager.Unmap(MemoryAllocation);
        MemoryManager.Free(MemoryAllocation);
    }
}

bool FVulkanUploadBuffer::Initialize(uint64 Size)
{
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
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to create UploadBuffer");
        return false;
    }
    
    const VkMemoryAllocateFlags AllocateFlags    = 0;
    const VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    FVulkanMemoryManager& MemoryManager = GetDevice()->GetMemoryManager();
    if (!MemoryManager.AllocateBufferMemory(Buffer, MemoryProperties, AllocateFlags, GVulkanForceDedicatedAllocations, MemoryAllocation))
    {
        VULKAN_ERROR("Failed to allocate BufferMemory");
        return false;
    }

    void* BufferData = MemoryManager.Map(MemoryAllocation);
    if (VULKAN_FAILED(Result))
    {
        VULKAN_ERROR("Failed to map BufferMemory");
        return false;
    }

    if (!BufferData)
    {
        return false;
    }

    MappedMemory = reinterpret_cast<uint8*>(BufferData);
    return true;
}


FVulkanUploadHeapAllocator::FVulkanUploadHeapAllocator(FVulkanDevice* InDevice)
    : FVulkanDeviceChild(InDevice)
    , BufferSize(0)
    , CurrentOffset(0)
    , Buffer(nullptr)
{
}

FVulkanUploadHeapAllocator::~FVulkanUploadHeapAllocator()
{
}

FVulkanUploadAllocation FVulkanUploadHeapAllocator::Allocate(uint64 Size, uint64 Alignment)
{
    FVulkanUploadAllocation Allocation;
    
    // Make sure the size is properly aligned
    Size = FMath::AlignUp<uint64>(Size, Alignment);

    // Maximum size for a upload buffer
    const uint64 MaxUploadSize = static_cast<uint64>(CVarMaxStagingAllocationSize.GetValue()) * 1024 * 1024;
    if (Size < MaxUploadSize)
    {
        // Lock the buffer and all variable within
        TScopedLock Lock(CriticalSection);

        uint64 Offset    = FMath::AlignUp<uint64>(CurrentOffset, Alignment);
        uint64 NewOffset = Offset + Size;
        if (NewOffset >= BufferSize)
        {
            // Allocate a new 
            FVulkanUploadBufferRef NewBuffer = new FVulkanUploadBuffer(GetDevice());
            if (!NewBuffer->Initialize(MaxUploadSize))
            {
                VULKAN_ERROR("Failed to create staging-buffer");
                return Allocation;
            }
            else
            {
                Buffer = NewBuffer;
            }

            CHECK(Size <= MaxUploadSize);

            BufferSize = MaxUploadSize;
            Offset     = 0;
            NewOffset  = Offset + Size;
        }

        Allocation.Buffer = Buffer;
        Allocation.Offset = Offset;
        Allocation.Memory = Buffer->GetMappedMemory() + Offset;
        CurrentOffset = NewOffset;
    }
    else
    {
        // Allocate a new 
        FVulkanUploadBufferRef NewBuffer = new FVulkanUploadBuffer(GetDevice());
        if (!NewBuffer->Initialize(Size))
        {
            VULKAN_ERROR("Failed to create staging-buffer");
            return Allocation;
        }

        Allocation.Buffer = NewBuffer;
        Allocation.Offset = 0;
        Allocation.Memory = NewBuffer->GetMappedMemory();
    }

    return Allocation;
}

void FVulkanUploadHeapAllocator::Release()
{
    TScopedLock Lock(CriticalSection);

    Buffer.Reset();
    
    BufferSize    = 0;
    CurrentOffset = 0;
}
