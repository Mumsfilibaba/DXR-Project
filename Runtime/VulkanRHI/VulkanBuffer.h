#pragma once
#include "VulkanDeviceObject.h"
#include "VulkanRefCounted.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FVulkanBuffer> FVulkanBufferRef;

class FVulkanBuffer : public FRHIBuffer, public FVulkanDeviceObject, public FVulkanRefCounted
{
public:
    FVulkanBuffer(FVulkanDevice* InDevice, const FRHIBufferDesc& InBufferDesc);
    ~FVulkanBuffer();
    
    bool Initialize(EResourceAccess InInitialState, const void* InInitialData);

    virtual int32 AddRef() override final { return FVulkanRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FVulkanRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FVulkanRefCounted::GetRefCount(); }

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FVulkanBuffer*>(this)); }
    
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetVkBuffer()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetName(const FString& InName) override final;

    virtual FString GetName() const override final;

    FORCEINLINE VkBuffer GetVkBuffer() const
    {
        return Buffer;
    }
    
protected:
    VkBuffer        Buffer;
    VkDeviceMemory  DeviceMemory;
    VkDeviceAddress DeviceAddress;
    VkDeviceSize    RequiredAlignment;
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