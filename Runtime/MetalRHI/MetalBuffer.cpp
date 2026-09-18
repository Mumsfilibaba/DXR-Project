#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalRHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalBufferRHI::FMetalBufferRHI(FMetalDevice* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FMetalDeviceChild(InDevice)
    , Buffer(nil)
    , LastWriteValue(0)
{
}

FMetalBufferRHI::~FMetalBufferRHI()
{
    if (Buffer)
    {
        FMetalDeviceRHI::DeferDeletion(Buffer);
        [Buffer release];
        Buffer = nil;
    }
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
    UNREFERENCED_VARIABLE(Size);
    CHECK(Offset <= Desc.Size);

    id<MTLBuffer> BufferHandle = GetMTLBuffer();
    if (!BufferHandle)
    {
        return nullptr;
    }

    if (!MetalRHI::IsMTLBufferMappable(Desc))
    {
        String DebugNameStr;
        GetDebugName(DebugNameStr);
        METAL_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *DebugNameStr);
        return nullptr;
    }

    if (Desc.IsReadBack())
    {
        FMetalQueue* Queue = GetDevice()->GetQueue();
        if (LastWriteValue > 0)
        {
            Queue->WaitForValue(LastWriteValue);
        }
        else
        {
            Queue->WaitForCompletion();
        }
    }

    uint8* Contents = static_cast<uint8*>([BufferHandle contents]);
    return Contents ? (Contents + Offset) : nullptr;
}

void FMetalBufferRHI::Unmap(uint64 Offset, uint64 Size)
{
    // Shared storage stays coherent with the GPU, so nothing has to be flushed back.
}

bool FMetalBufferRHI::Initialize(ERHIResourceState InInitialAccess, const void* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    const uint64 AlignedSize = Math::AlignUp(Desc.Size, MetalRHI::GetMTLBufferAlignment(Desc));

    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();
    CHECK(DeviceHandle != nil);

    id<MTLBuffer> NewBuffer = [DeviceHandle newBufferWithLength:AlignedSize options:MetalRHI::GetMTLBufferResourceOptions(Desc)];
    if (!NewBuffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte buffer", AlignedSize);
        return false;
    }

    SetMTLBuffer(NewBuffer);
    [NewBuffer release];

    if (!InInitialData)
    {
        return true;
    }

    if (NewBuffer.storageMode == MTLStorageModeShared)
    {
        Memory::Memcpy(NewBuffer.contents, InInitialData, Desc.Size);
        return true;
    }

    FMetalUploadBatch UploadBatch(GetDevice());
    if (!UploadBatch.IsValid())
    {
        return false;
    }

    id<MTLBuffer> StagingBuffer = UploadBatch.CreateStagingBuffer(Desc.Size);
    if (!StagingBuffer)
    {
        return false;
    }

    Memory::Memcpy(StagingBuffer.contents, InInitialData, Desc.Size);

    [UploadBatch.GetBlitEncoder() copyFromBuffer:StagingBuffer
                                    sourceOffset:0
                                        toBuffer:NewBuffer
                               destinationOffset:0
                                            size:Desc.Size];

    LastWriteValue = UploadBatch.Submit();
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
