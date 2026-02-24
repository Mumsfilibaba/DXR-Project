#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanMemory.h"
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
        return Buffer;
    }

    VkBuffer GetBindVkBuffer() const
    {
        return (TransientBuffer != VK_NULL_HANDLE) ? TransientBuffer : Buffer;
    }

    VkDeviceSize GetBindOffset() const
    {
        return TransientOffset;
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
        return MemoryAllocation.Memory;
    }

    VkDeviceAddress GetDeviceAddress() const
    {
        return MemoryAllocation.DeviceAddress;
    }

    VkDeviceSize GetRequiredAlignment() const
    {
        return RequiredAlignment;
    }

    FVulkanBufferState&       GetTrackedState()       { return TrackedState; }
    const FVulkanBufferState& GetTrackedState() const { return TrackedState; }

protected:
    VkBuffer                Buffer;
    FVulkanMemoryAllocation MemoryAllocation;
    VkDeviceSize            RequiredAlignment;
    FVulkanBufferState      TrackedState;
    FString                 DebugName;

    VkBuffer     TransientBuffer = VK_NULL_HANDLE;
    VkDeviceSize TransientOffset = 0;
    VkDeviceSize TransientRange  = 0;
};
