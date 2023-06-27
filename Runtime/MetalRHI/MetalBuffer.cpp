#include "MetalBuffer.h"
#include "MetalDeviceContext.h"

FMetalBuffer::FMetalBuffer(FMetalDeviceContext* DeviceContext, const FRHIBufferDesc& InDesc)
    : FRHIBuffer(InDesc)
    , FMetalObject(DeviceContext)
    , FMetalRefCounted()
    , Buffer(nil)
{
}

FMetalBuffer::~FMetalBuffer()
{
    NSSafeRelease(Buffer);
}

bool FMetalBuffer::Initialize(EResourceAccess InInitialAccess, const void* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();
    
    MTLResourceOptions ResourceOptions = MTLResourceHazardTrackingModeDefault;
    if (Desc.IsDynamic())
    {
        ResourceOptions |= MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache;
    }
    else
    {
        ResourceOptions |= MTLResourceStorageModePrivate | MTLResourceCPUCacheModeWriteCombined;
    }
    
    const uint64 Alignment   = Desc.IsConstantBuffer() ? kConstantBufferAlignment : kBufferAlignment;
    const uint64 AlignedSize = FMath::AlignUp(Desc.Size, Alignment);
    
    id<MTLDevice> Device       = GetDeviceContext()->GetMTLDevice();
    id<MTLBuffer> NewMTLBuffer = [Device newBufferWithLength:AlignedSize options:ResourceOptions];
    if (!NewMTLBuffer)
    {
        return false;
    }
    
    // Set the buffer handle
    SetMTLBuffer(NewMTLBuffer);
    
    // Upload the data
    if (InInitialData)
    {
        if (Desc.IsDynamic())
        {
            FMemory::Memcpy(NewMTLBuffer.contents, InInitialData, Desc.Size);
        }
        else
        {
            @autoreleasepool
            {
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:Desc.Size options:MTLResourceCPUCacheModeDefaultCache];
                FMemory::Memcpy(StagingBuffer.contents, InInitialData, Desc.Size);
                
                id<MTLCommandQueue>       CommandQueue  = GetDeviceContext()->GetMTLCommandQueue();
                id<MTLCommandBuffer>      CommandBuffer = [CommandQueue commandBuffer];
                id<MTLBlitCommandEncoder> CopyEncoder   = [CommandBuffer blitCommandEncoder];
                
                [CopyEncoder copyFromBuffer:StagingBuffer
                               sourceOffset:0
                                   toBuffer:NewMTLBuffer
                          destinationOffset:0
                                       size:Desc.Size];
                
                [CopyEncoder endEncoding];

                // TODO: we do not want to wait here
                [CommandBuffer commit];
                [CommandBuffer waitUntilCompleted];
            
                [StagingBuffer release];
            }
        }
    }

    return true;
}

void FMetalBuffer::SetName(const FString& InName)
{
    @autoreleasepool
    {
        id<MTLBuffer> BufferHandle = GetMTLBuffer();
        if (BufferHandle)
        {
            BufferHandle.label = InName.GetNSString();
        }
    }
}

FString FMetalBuffer::GetName() const
{
    FString Result;
    
    @autoreleasepool
    {
        id<MTLBuffer> BufferHandle = GetMTLBuffer();
        if (BufferHandle)
        {
            Result = FString(BufferHandle.label);
        }
    }
    
    return Result;
}
