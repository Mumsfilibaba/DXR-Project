#pragma once
#include "MetalRHI/MetalCore.h"

class FMetalDevice;
class FMetalQueue;
class FMetalResourceStorage;
struct FMetalCommands;

class FMetalUploadBatch
{
public:
    explicit FMetalUploadBatch(FMetalDevice* InDevice);
    ~FMetalUploadBatch();

    bool CreateStagingBuffer(uint64 Size, FMetalResourceStorage& OutStorage);
    uint64 Submit();

    bool IsValid() const
    {
        return BlitEncoder != nil;
    }

    id<MTLBlitCommandEncoder> GetBlitEncoder() const
    {
        return BlitEncoder;
    }

private:
    FMetalDevice*             Device;
    FMetalQueue*              Queue;
    FMetalCommands*           Commands;
    id<MTLBlitCommandEncoder> BlitEncoder;
};
