#pragma once
#include "VulkanDeviceObject.h"

#include "RHI/RHIResources.h"


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CVulkanBuffer> CVulkanBufferRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanBuffer

class CVulkanBuffer : public CRHIBuffer, public CVulkanDeviceObject
{
public:

	CVulkanBuffer(CVulkanDevice* InDevice, const CRHIBufferDesc& InBufferDesc);
	~CVulkanBuffer();
	
	bool Initialize();

	virtual void SetName(const String& InName) override final
	{
		CRHIResource::SetName(InName);
	}

	virtual bool IsValid() const override
	{
		return true;
	}

	FORCEINLINE VkBuffer GetVkBuffer() const
	{
		return Buffer;
	}
	
protected:
	
	VkBuffer        Buffer;
	VkDeviceMemory  DeviceMemory;
	VkDeviceAddress DeviceAddress;
    uint32          RequiredAlignment;
};
