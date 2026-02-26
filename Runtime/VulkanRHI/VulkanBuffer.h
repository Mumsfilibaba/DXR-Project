#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanMemoryManager.h"
#include "VulkanRHI/VulkanResourceState.h"

typedef TSharedRef<class FVulkanBuffer> FVulkanBufferRef;

class FVulkanBuffer : public FRHIBuffer, public FVulkanDeviceChild
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
        if (TransientBuffer != VK_NULL_HANDLE)
        {
            return TransientBuffer;
        }

        return GetVkBuffer();
    }

    VkDeviceSize GetBindOffset() const
    {
        if (TransientBuffer != VK_NULL_HANDLE)
        {
            return TransientOffset;
        }

        return MemoryStorage.GetBufferOffset();
    }

    VkDeviceSize GetBindRange() const
    {
        return (TransientBuffer != VK_NULL_HANDLE) ? TransientRange : Info.Size;
    }

    void SetTransientAllocation(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange)
    {
        TransientBuffer = InBuffer;
        TransientOffset = InOffset;
        TransientRange  = InRange;
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

    const FVulkanMemoryStorage& GetMemoryStorage() const
    {
        return MemoryStorage;
    }

    FVulkanBufferState&       GetTrackedState()       { return TrackedState; }
    const FVulkanBufferState& GetTrackedState() const { return TrackedState; }

protected:
    VkBuffer             OwnedBuffer;
    FVulkanMemoryStorage MemoryStorage;
    VkDeviceSize         RequiredAlignment;
    FVulkanBufferState   TrackedState;
    FString              DebugName;
    VkBuffer             TransientBuffer;
    VkDeviceSize         TransientOffset;
    VkDeviceSize         TransientRange;
};
