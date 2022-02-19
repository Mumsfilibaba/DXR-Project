#include "VulkanPhysicalDevice.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanPhysicalDevice

TSharedRef<CVulkanPhysicalDevice> CVulkanPhysicalDevice::QueryAdapter(CVulkanDriverInstance* InInstance, const SVulkanPhysicalDeviceDesc& AdapterDesc)
{
	TSharedRef<CVulkanPhysicalDevice> NewPhyscialDevice = dbg_new CVulkanPhysicalDevice(InInstance);
	if (NewPhyscialDevice && NewPhyscialDevice->Initialize(AdapterDesc))
	{
		return NewPhyscialDevice;
	}
	else
	{
		return nullptr;
	}
}

CVulkanPhysicalDevice::CVulkanPhysicalDevice(CVulkanDriverInstance* InInstance)
	: Instance(InInstance)
	, PhysicalDevice(VK_NULL_HANDLE)
{
}

CVulkanPhysicalDevice::~CVulkanPhysicalDevice()
{
}

bool CVulkanPhysicalDevice::Initialize(const SVulkanPhysicalDeviceDesc& AdapterDesc)
{
	return true;
}
