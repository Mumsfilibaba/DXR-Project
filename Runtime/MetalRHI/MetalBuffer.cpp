#include "MetalBuffer.h"
#include "MetalDeviceContext.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalBuffer::FMetalBuffer(FMetalDeviceContext* DeviceContext, const FRHIBufferInfo& InBufferInfo)
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
    
    id<MTLDevice> Device = GetDeviceContext()->GetMTLDevice();
    CHECK(Device != nil);
    
    id<MTLBuffer> NewBuffer = [Device newBufferWithLength:AlignedSize options:ResourceOptions];
    if (!NewBuffer)
    {
        return false;
    }
    
    // Set the buffer handle
    SetMTLBuffer(NewBuffer);
    
    // Upload the data
    if (InInitialData)
    {
        if (Desc.IsDynamic())
        {
            FMemory::Memcpy(NewBuffer.contents, InInitialData, Desc.Size);
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
                                   toBuffer:NewBuffer
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

void FMetalBuffer::SetDebugName(const FString& InName)
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

FString FMetalBuffer::GetDebugName() const
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

ENABLE_UNREFERENCED_VARIABLE_WARNING
