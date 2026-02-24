#pragma once
#include "Core/Containers/SharedRef.h"
#include "VulkanRHI/VulkanMemory.h"
#include "VulkanRHI/VulkanRefCounted.h"

typedef TSharedRef<class FVulkanUploadBuffer> FVulkanUploadBufferRef;

struct FVulkanUploadAllocation
{
    FVulkanUploadAllocation()
        : Buffer(nullptr)
        , Memory(nullptr)
        , Offset(0)
    {
    }

    FVulkanUploadBufferRef Buffer;
    uint8*                 Memory;
    VkDeviceSize           Offset;
};

class FVulkanUploadBuffer : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanUploadBuffer(FVulkanDevice* InDevice);
    ~FVulkanUploadBuffer();

    bool Initialize(uint64 Size);

    VkBuffer GetVkBuffer() const
    {
        return Buffer;
    }

    uint8* GetMappedMemory() const
    {
        return MappedMemory;
    }

private:
    VkBuffer                Buffer;
    FVulkanMemoryAllocation MemoryAllocation;
    uint8*                  MappedMemory;
};

class FVulkanUploadHeapAllocator : public FVulkanDeviceChild
{
public:
    FVulkanUploadHeapAllocator(FVulkanDevice* InDevice);
    virtual ~FVulkanUploadHeapAllocator();

    FVulkanUploadAllocation Allocate(uint64 Size, uint64 Alignment);

private:
    VkDeviceSize           BufferSize;
    VkDeviceSize           CurrentOffset;
    FVulkanUploadBufferRef Buffer;
    FCriticalSection       CriticalSection;
};

struct FVulkanDynamicConstantsAllocation
{
    VkBuffer     Buffer = VK_NULL_HANDLE;
    VkDeviceSize Offset = 0;
    void*        MappedMemory = nullptr;
};

class FVulkanDynamicConstantsAllocator : public FVulkanDeviceChild
{
public:
    FVulkanDynamicConstantsAllocator(FVulkanDevice* InDevice, uint64 InPageSizeBytes);
    ~FVulkanDynamicConstantsAllocator();

    FVulkanDynamicConstantsAllocation Allocate(uint64 SizeInBytes);

private:
    bool AllocateNewPage();

    uint64       PageSizeBytes;
    uint64       MinAlignment;
    VkDeviceSize CurrentOffset;

    VkBuffer                Buffer;
    FVulkanMemoryAllocation MemoryAllocation;
    uint8*                  MappedMemory;
};
