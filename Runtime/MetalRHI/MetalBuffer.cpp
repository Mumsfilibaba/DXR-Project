#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalDevice.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalBufferRHI::FMetalBufferRHI(FMetalDevice* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FMetalDeviceChild(InDevice)
    , Buffer(nil)
{
}

FMetalBufferRHI::~FMetalBufferRHI()
{
    [Buffer release];
}

void* FMetalBufferRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetMTLBuffer());
}

FRHIDescriptorHandle FMetalBufferRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void* FMetalBufferRHI::Map(uint64 Offset, uint64 Size)
{
    id<MTLBuffer> BufferHandle = GetMTLBuffer();
    if (!BufferHandle)
    {
        return nullptr;
    }

    // Only shared-storage (CPU-visible) buffers can be mapped directly.
    if (BufferHandle.storageMode != MTLStorageModeShared)
    {
        return nullptr;
    }

    uint8* Contents = static_cast<uint8*>([BufferHandle contents]);
    return Contents ? (Contents + Offset) : nullptr;
}

void FMetalBufferRHI::Unmap(uint64 Offset, uint64 Size)
{
}

bool FMetalBufferRHI::Initialize(EResourceAccess InInitialAccess, const void* InInitialData)
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
    const uint64 AlignedSize = Math::AlignUp(Desc.Size, Alignment);
    
    id<MTLDevice> Device = GetDevice()->GetMTLDevice();
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
            Memory::Memcpy(NewBuffer.contents, InInitialData, Desc.Size);
        }
        else
        {
            @autoreleasepool
            {
                id<MTLBuffer> StagingBuffer = [Device newBufferWithLength:Desc.Size options:MTLResourceCPUCacheModeDefaultCache];
                Memory::Memcpy(StagingBuffer.contents, InInitialData, Desc.Size);
                
                id<MTLCommandQueue>       CommandQueue  = GetDevice()->GetMTLCommandQueue();
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

void FMetalBufferRHI::SetDebugName(const String& InName)
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

void FMetalBufferRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName.Clear();

    @autoreleasepool
    {
        id<MTLBuffer> BufferHandle = GetMTLBuffer();
        if (BufferHandle)
        {
            OutDebugName = String(BufferHandle.label);
        }
    }
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
