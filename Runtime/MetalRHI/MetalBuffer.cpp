#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalRHI.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalBufferRHI::FMetalBufferRHI(FMetalDevice* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FMetalDeviceChild(InDevice)
    , Buffer(nil)
    , ResourceStorage(InDevice)
    , LastWriteValue(0)
{
}

FMetalBufferRHI::~FMetalBufferRHI()
{
    ResourceStorage.ReleaseResource();
    Buffer = nil;
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

        if (FMetalQueue* CopyQueue = GetDevice()->GetQueue(EMetalQueueType::Copy))
        {
            CopyQueue->WaitForCompletion();
        }
    }

    uint8* Contents = static_cast<uint8*>([BufferHandle contents]);
    return Contents ? (Contents + ResourceStorage.GetResourceOffset() + Offset) : nullptr;
}

void FMetalBufferRHI::Unmap(uint64 Offset, uint64 Size)
{
    // Shared storage stays coherent with the GPU, so nothing has to be flushed back.
}

bool FMetalBufferRHI::Initialize(ERHIResourceState InInitialAccess, const void* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    const uint64 AlignedSize = Math::AlignUp(Desc.Size, MetalRHI::GetMTLBufferAlignment(Desc));
    const MTLResourceOptions Options = MetalRHI::GetMTLBufferResourceOptions(Desc);

    bool bAllocated = false;
    if (Desc.IsDynamic() || Desc.IsTransient())
    {
        FMetalQueue* Queue = GetDevice()->GetQueue(EMetalQueueType::Direct);
        bAllocated = GetDevice()->GetUploadHeapAllocator()->Allocate(AlignedSize, MetalRHI::GetMTLBufferAlignment(Desc), Queue, ResourceStorage) != nullptr;
    }
    else
    {
        bAllocated = GetDevice()->GetBufferAllocator()->TryAllocate(AlignedSize, MetalRHI::GetMTLBufferAlignment(Desc), Options, ResourceStorage);
    }

    if (!bAllocated)
    {
        METAL_ERROR("Failed to allocate a %llu byte buffer", AlignedSize);
        return false;
    }

    Buffer = ResourceStorage.GetBuffer();
    id<MTLBuffer> NewBuffer = Buffer;
    if (!NewBuffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte buffer", AlignedSize);
        return false;
    }

    if (!InInitialData)
    {
        return true;
    }

    if (NewBuffer.storageMode == MTLStorageModeShared)
    {
        Memory::Memcpy(static_cast<uint8*>(NewBuffer.contents) + ResourceStorage.GetResourceOffset(), InInitialData, Desc.Size);
        return true;
    }

    FMetalUploadBatch UploadBatch(GetDevice());
    if (!UploadBatch.IsValid())
    {
        return false;
    }

    FMetalResourceStorage StagingStorage(GetDevice());
    if (!UploadBatch.CreateStagingBuffer(Desc.Size, StagingStorage))
    {
        return false;
    }

    Memory::Memcpy(StagingStorage.GetMappedBaseAddress(), InInitialData, Desc.Size);

    [UploadBatch.GetBlitEncoder() copyFromBuffer:StagingStorage.GetBuffer()
                                    sourceOffset:StagingStorage.GetResourceOffset()
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
