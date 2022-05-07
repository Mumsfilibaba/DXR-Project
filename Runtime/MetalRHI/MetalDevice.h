#pragma once
#include <Metal/Metal.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDevice

class CMetalDevice 
{
protected:
	
	CMetalDevice(id<MTLDevice> InDevice)
		: Device(InDevice)
	{ }

public:

	~CMetalDevice()
	{
		if (Device)
		{
			[Device release];
		}
	}

	static CMetalDevice* CreateMetalDevice()
	{
		id<MTLDevice> Device = MTLCreateSystemDefaultDevice();
		return dbg_new CMetalDevice(Device);
	}
	
	FORCEINLINE id<MTLDevice> GetDevice() const { return Device; }
	
private:
	id<MTLDevice> Device;
};
