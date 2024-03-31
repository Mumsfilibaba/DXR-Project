#pragma once
#include "VulkanMemory.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanBuffer> FVulkanBufferRef;

class FVulkanBuffer : public FRHIBuffer, public FVulkanDeviceChild
{
public:
    FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc);
    ~FVulkanBuffer();
    
    bool Initialize(EResourceAccess InInitialAccess, const void* InInitialData);

    virtual void* GetRHIBaseBuffer()         override final { return reinterpret_cast<void*>(static_cast<FVulkanBuffer*>(this)); }
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetVkBuffer()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final;
    virtual FString GetName() const override final;

    VkBuffer GetVkBuffer() const
    {
        return Buffer;
    }

    VkDeviceMemory GetVkDeviceMemory() const
    {
        return MemoryAllocation.Memory;
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


inline FVulkanBuffer* GetVulkanBuffer(FRHIBuffer* Buffer)
{
    return static_cast<FVulkanBuffer*>(Buffer);
}

inline VkBuffer GetVkBuffer(FRHIBuffer* Buffer)
{
    FVulkanBuffer* VulkanBuffer = GetVulkanBuffer(Buffer);
    return VulkanBuffer ? VulkanBuffer->GetVkBuffer() : VK_NULL_HANDLE;
}
