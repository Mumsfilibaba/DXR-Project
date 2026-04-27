#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResource.h"
#include "VulkanRHI/VulkanResourceState.h"

typedef TSharedRef<class FVulkanBufferRHI> FVulkanBufferRHIRef;

class FVulkanBufferRHI : public FRHIBuffer, public FVulkanResource
{
public:
    FVulkanBufferRHI(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc);
    ~FVulkanBufferRHI();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    // FRHIBuffer Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final;
    
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX)   override final;
    virtual void  Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    FVulkanBufferState&       GetBufferState()       { return BufferState; }
    const FVulkanBufferState& GetBufferState() const { return BufferState; }

    bool IsSuballocated() const
    {
        return MemoryStorage.IsSuballocated() && OwnedBuffer == VK_NULL_HANDLE;
    }

    VkBuffer GetVkBuffer() const
    {
        if (OwnedBuffer != VK_NULL_HANDLE)
        {
            return OwnedBuffer;
        }

        return MemoryStorage.GetBackingBuffer();
    }

    VkBuffer GetBindVkBuffer() const
    {
        return GetVkBuffer();
    }

    VkDeviceSize GetBindOffset() const
    {
        return MemoryStorage.GetBufferOffset();
    }

    VkDeviceSize GetBindRange() const
    {
        return Desc.Size;
    }

    VkDeviceMemory GetVkDeviceMemory() const
    {
        return MemoryStorage.GetMemory();
    }

    VkDeviceAddress GetDeviceAddress() const
    {
        return MemoryStorage.GetDeviceAddress();
    }

    VkDeviceSize GetRequiredAlignment() const
    {
        return RequiredAlignment;
    }

protected:
    VkBuffer           OwnedBuffer;
    VkDeviceSize       RequiredAlignment;
    FVulkanBufferState BufferState;
    FString            DebugName;
};
