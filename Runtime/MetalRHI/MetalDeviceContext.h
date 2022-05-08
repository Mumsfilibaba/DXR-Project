#pragma once
#include "MetalCore.h"

class CMetalCoreInterface;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDeviceContext

class CMetalDeviceContext 
{
protected:
	
	friend class CMetalCoreInterface;
	
	CMetalDeviceContext(CMetalCoreInterface* InCoreInterface, id<MTLDevice> InDevice);
	~CMetalDeviceContext();

public:
    
	static CMetalDeviceContext* CreateContext(CMetalCoreInterface* InCoreInterface);
	
	FORCEINLINE id<MTLDevice> GetMTLDevice() const { return Device; }
	
	FORCEINLINE id<MTLCommandQueue> GetMTLCommandQueue() const { return CommandQueue; }

	FORCEINLINE CMetalCoreInterface* GetMetalCoreInterface() const { return CoreInterface; }
	
private:
	CMetalCoreInterface* CoreInterface;
	
	id<MTLDevice>        Device;
	id<MTLCommandQueue>  CommandQueue;
};
