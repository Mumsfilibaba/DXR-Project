#pragma once
#include "MetalCore.h"

class FMetalInterface;

class FMetalDeviceContext 
{
private:
    friend class FMetalInterface;
    
    FMetalDeviceContext(id<MTLDevice> InDevice);
    ~FMetalDeviceContext();

public:
    static FMetalDeviceContext* CreateContext();
    
    id<MTLDevice> GetMTLDevice() const
    {
        return Device;
    }

    id<MTLCommandQueue> GetMTLCommandQueue() const
    {
        return CommandQueue;
    }

private:
    id<MTLDevice>       Device;
    id<MTLCommandQueue> CommandQueue;
};
