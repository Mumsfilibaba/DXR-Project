#pragma once
#include "MetalRHI/MetalCore.h"

class FMetalDevice
{
public:
    FMetalDevice();
    ~FMetalDevice();

    bool Initialize();

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
