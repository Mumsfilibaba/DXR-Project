#pragma once
#include "VulkanMemory.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanBuffer> FVulkanBufferRef;

class FVulkanBuffer : public FRHIBuffer, public FVulkanDeviceChild
{
public:
    static FVulkanBuffer* ResourceCast(FRHIBuffer* Buffer);

public:
    FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferInfo& InBufferDesc);
    ~FVulkanBuffer();

    bool Initialize(FVulkanCommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData);

public:

    // FRHIBuffer Interface
    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FVulkanBuffer*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetVkBuffer()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }
    
    virtual void SetDebugName(const FString& InName) override final;
    virtual FString GetDebugName() const override final;

public:

    VkBuffer GetVkBuffer() const
    {
        return Buffer;
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

protected:
    VkBuffer                Buffer;
    FVulkanMemoryAllocation MemoryAllocation;
    VkDeviceSize            RequiredAlignment;
    FString                 DebugName;
};
