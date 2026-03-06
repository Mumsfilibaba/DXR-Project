#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanResource.h"
#include "VulkanRHI/VulkanResourceState.h"

typedef TSharedRef<class FVulkanBuffer> FVulkanBufferRef;

class FVulkanBuffer : public FRHIBuffer, public FVulkanGenericResource
{
public:
    static FORCEINLINE FVulkanBuffer* Cast(FRHIBuffer* Buffer)
    {
        return static_cast<FVulkanBuffer*>(Buffer);
    }

public:
    FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferInfo& InBufferDesc);
    ~FVulkanBuffer();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    // FRHIBuffer Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkBuffer()); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;
    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;

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
        return Info.Size;
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

    bool IsSuballocated() const
    {
        return MemoryStorage.IsSuballocated() && OwnedBuffer == VK_NULL_HANDLE;
    }

    FVulkanBufferState&       GetBufferState()       { return TrackedState; }
    const FVulkanBufferState& GetBufferState() const { return TrackedState; }

protected:
    VkBuffer             OwnedBuffer;
    VkDeviceSize         RequiredAlignment;
    FVulkanBufferState   TrackedState;
    FString              DebugName;
};
