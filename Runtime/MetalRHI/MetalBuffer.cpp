#include "MetalRHI/MetalBuffer.h"
#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalQueue.h"
#include "MetalRHI/MetalUploadBatch.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalViews.h"
#include "Core/Threading/ScopedLock.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

FMetalBufferRHI::FMetalBufferRHI(FMetalDevice* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FMetalDeviceChild(InDevice)
    , ResourceStorage(InDevice)
    , BindlessHandle()
    , RelocationListeners()
    , RelocationListenersCS()
{
}

FMetalBufferRHI::~FMetalBufferRHI()
{
    {
        TScopedLock Lock(RelocationListenersCS);
        for (FMetalView* View : RelocationListeners)
        {
            View->OnBufferReleased();
        }

        RelocationListeners.Clear();
    }

    if (BindlessHandle.IsValid())
    {
        if (FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
        {
            BindlessManager->Free(BindlessHandle);
        }

        BindlessHandle = FRHIDescriptorHandle();
    }

    ResourceStorage.ReleaseResource();
}

void* FMetalBufferRHI::GetRHINativeResource() const
{
    return reinterpret_cast<void*>(GetMTLBuffer());
}

FRHIDescriptorHandle FMetalBufferRHI::GetBindlessHandle() const
{
    if (BindlessHandle.IsValid())
    {
        return BindlessHandle;
    }

    if (!Desc.IsConstantBuffer())
    {
        CHECK(false);
        return FRHIDescriptorHandle();
    }

    FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager();

    if (!BindlessManager || !BindlessManager->IsEnabled())
    {
        return FRHIDescriptorHandle();
    }

    BindlessHandle = BindlessManager->Allocate(EDescriptorType::ConstantBuffer);

    if (!BindlessHandle.IsValid())
    {
        return FRHIDescriptorHandle();
    }

    BindlessManager->WriteBuffer(BindlessHandle, GetMTLBuffer(), ResourceStorage.GetResourceOffset(), ResourceStorage.IsPlacedResource(), true);
    return BindlessHandle;
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

    if (!Desc.IsDynamic() && !Desc.IsReadBack() && !Desc.IsTransient())
    {
        String DebugNameStr;
        GetDebugName(DebugNameStr);
        METAL_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *DebugNameStr);
        return nullptr;
    }

    uint8* Contents = static_cast<uint8*>([BufferHandle contents]);
    return Contents ? (Contents + GetMetalBindOffset() + Offset) : nullptr;
}

void FMetalBufferRHI::Unmap(uint64 Offset, uint64 Size)
{
    if (id<MTLBuffer> BufferHandle = GetMTLBuffer())
    {
        MetalRHI::FlushCPUWrite(BufferHandle, GetMetalBindOffset() + Offset, Math::Min(Size, Desc.Size - Offset));
    }
}

bool FMetalBufferRHI::RelocateTransientStorage(uint64 SizeInBytes, const void* SourceData, FMetalQueue* Queue)
{
    FMetalLinearAllocator* DynamicConstantsAllocator = GetDevice()->GetDynamicConstantsAllocator();

    if (!DynamicConstantsAllocator || !SourceData || SizeInBytes == 0)
    {
        return false;
    }

    const uint64 Alignment   = MetalRHI::GetMTLBufferAlignment(Desc);
    const uint64 AlignedSize = Math::AlignUp(SizeInBytes, Alignment);

    ResourceStorage.ReleaseResource();

    void* Mapped = DynamicConstantsAllocator->Allocate(AlignedSize, Alignment, Queue, ResourceStorage);

    if (!Mapped)
    {
        METAL_ERROR("Failed to allocate %llu bytes of dynamic constant memory", AlignedSize);
        return false;
    }

    Memory::Memcpy(Mapped, SourceData, SizeInBytes);

    if (BindlessHandle.IsValid())
    {
        if (FMetalBindlessDescriptorManager* BindlessManager = GetDevice()->GetBindlessDescriptorManager())
        {
            BindlessManager->Free(BindlessHandle);
        }

        BindlessHandle = FRHIDescriptorHandle();
    }

    TScopedLock Lock(RelocationListenersCS);
    for (FMetalView* View : RelocationListeners)
    {
        View->OnBufferRelocated();
    }

    return true;
}

void FMetalBufferRHI::AddRelocationListener(FMetalView* View)
{
    CHECK(View != nullptr);

    TScopedLock Lock(RelocationListenersCS);
    RelocationListeners.AddUnique(View);
}

void FMetalBufferRHI::RemoveRelocationListener(FMetalView* View)
{
    TScopedLock Lock(RelocationListenersCS);
    RelocationListeners.Remove(View);
}

bool FMetalBufferRHI::Initialize(ERHIResourceState InInitialAccess, const void* InInitialData)
{
    SCOPED_AUTORELEASE_POOL();

    const uint64            AlignedSize = Math::AlignUp(Desc.Size, MetalRHI::GetMTLBufferAlignment(Desc));
    const EMetalMemoryClass MemoryClass = MetalRHI::GetMetalMemoryClass(Desc);

    const bool bFillInPlace = InInitialData && MemoryClass == EMetalMemoryClass::GPUOnly && MetalRHI::HasUnifiedMemory();
    const MTLResourceOptions Options = MetalRHI::GetMTLResourceOptions(bFillInPlace ? EMetalMemoryClass::Upload : MemoryClass);

    bool bAllocated = false;

    if (Desc.IsTransient())
    {
        FMetalQueue* Queue = GetDevice()->GetQueue(EMetalQueueType::Direct);
        bAllocated = GetDevice()->GetUploadHeapAllocator()->Allocate(AlignedSize, MetalRHI::GetMTLBufferAlignment(Desc), Queue, ResourceStorage) != nullptr;
    }
    else
    {
        const bool bBindlessReachable = Desc.IsConstantBuffer() || Desc.IsShaderResourceBuffer() || Desc.IsUnorderedAccessBuffer();
        bAllocated = GetDevice()->GetBufferAllocator()->TryAllocate(AlignedSize, MetalRHI::GetMTLBufferAlignment(Desc), Options, bBindlessReachable, ResourceStorage);
    }

    if (!bAllocated)
    {
        METAL_ERROR("Failed to allocate a %llu byte buffer", AlignedSize);
        return false;
    }

    id<MTLBuffer> NewBuffer = ResourceStorage.GetBuffer();

    if (!NewBuffer)
    {
        METAL_ERROR("Failed to allocate a %llu byte buffer", AlignedSize);
        return false;
    }

    if (!InInitialData)
    {
        return true;
    }

    if (NewBuffer.storageMode != MTLStorageModePrivate)
    {
        Memory::Memcpy(static_cast<uint8*>(NewBuffer.contents) + GetMetalBindOffset(), InInitialData, Desc.Size);
        MetalRHI::FlushCPUWrite(NewBuffer, GetMetalBindOffset(), Desc.Size);
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
                               destinationOffset:GetMetalBindOffset()
                                            size:Desc.Size];

    UploadBatch.Submit();
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
