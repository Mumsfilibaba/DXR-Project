#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanRefCounted.h"
#include "Core/Containers/SharedRef.h"

typedef TSharedRef<class FVulkanUploadBuffer> FVulkanUploadBufferRef;

struct FVulkanUploadAllocation
{
    FVulkanUploadBufferRef Buffer;
    uint8*                 Memory = nullptr;
    VkDeviceSize           Offset = 0;
};

class FVulkanUploadBuffer : public FVulkanDeviceObject, public FVulkanRefCounted
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
    VkBuffer       Buffer;
    VkDeviceMemory BufferMemory;
    uint8*         MappedMemory;
};

class FVulkanUploadHeapAllocator : public FVulkanDeviceObject
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