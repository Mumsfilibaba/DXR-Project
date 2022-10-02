#pragma once
#include "MetalCore.h"

class FMetalInterface;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalDeviceContext

class FMetalDeviceContext 
{
private:
	friend class FMetalInterface;
	
	FMetalDeviceContext(FMetalInterface* InCoreInterface, id<MTLDevice> InDevice);
	~FMetalDeviceContext();

public:
	static FMetalDeviceContext* CreateContext(FMetalInterface* InCoreInterface);
	
	FORCEINLINE id<MTLDevice>       GetMTLDevice() const { return Device; }
	
	FORCEINLINE id<MTLCommandQueue> GetMTLCommandQueue() const { return CommandQueue; }

	FORCEINLINE FMetalInterface*    GetMetalInterface() const { return CoreInterface; }
	
private:
	FMetalInterface* CoreInterface;

	id<MTLDevice>        Device;
	id<MTLCommandQueue>  CommandQueue;
};
