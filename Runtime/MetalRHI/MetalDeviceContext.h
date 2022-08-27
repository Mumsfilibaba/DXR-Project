#pragma once
#include "MetalCore.h"

class FMetalCoreInterface;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalDeviceContext

class FMetalDeviceContext 
{
protected:
	
	friend class FMetalCoreInterface;
	
	FMetalDeviceContext(FMetalCoreInterface* InCoreInterface, id<MTLDevice> InDevice);
	~FMetalDeviceContext();

public:
    
	static FMetalDeviceContext* CreateContext(FMetalCoreInterface* InCoreInterface);
	
	FORCEINLINE id<MTLDevice> GetMTLDevice() const { return Device; }
	
	FORCEINLINE id<MTLCommandQueue> GetMTLCommandQueue() const { return CommandQueue; }

	FORCEINLINE FMetalCoreInterface* GetMetalCoreInterface() const { return CoreInterface; }
	
private:
	FMetalCoreInterface* CoreInterface;
	
	id<MTLDevice>        Device;
	id<MTLCommandQueue>  CommandQueue;
};
