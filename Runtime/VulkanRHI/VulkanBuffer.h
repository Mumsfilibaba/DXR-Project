#pragma once
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanMemory.h"
#include "VulkanRHI/VulkanResourceState.h"
#include "Core/Containers/UniquePtr.h"

typedef TSharedRef<class FVulkanBuffer> FVulkanBufferRef;

class FVulkanBuffer : public FRHIBuffer, public FVulkanDeviceChild
{
public:
    static FORCEINLINE FVulkanBuffer* Cast(FRHIBuffer* Buffer)
    {
        return static_cast<FVulkanBuffer*>(Buffer);
    }

public:
    FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferInfo& InBufferDesc, EResourceAccess InInitialState = EResourceAccess::Common);
    ~FVulkanBuffer();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

    // FRHIBuffer Interface
    virtual void* GetRHINativeHandle() const override final { return reinterpret_cast<void*>(GetVkBuffer()); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;
    virtual void* Map(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;
    virtual void Unmap(uint64 Offset = 0, uint64 Size = UINT64_MAX) override final;

    // Resource State Tracking
    VkBuffer GetVkBuffer() const
    {
        return Buffer;
    }

    FVulkanBufferState* GetBufferState() const
    {
        return BufferState.Get();
    }

    void EnableResourceStateTracking(EResourceAccess InitialState);
    void DisableResourceStateTracking();

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

protected:
    VkBuffer                Buffer;
    FVulkanMemoryAllocation MemoryAllocation;
    VkDeviceSize            RequiredAlignment;
    FString                 DebugName;
    TUniquePtr<FVulkanBufferState> BufferState;
};
