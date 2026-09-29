#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalHeap.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalStats.h"

FMetalResourceStorage::FMetalResourceStorage(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , AllocatorPointers()
    , Buffer(nil)
    , Texture(nil)
    , Heap(nullptr)
    , MappedBaseAddress(nullptr)
    , ResourceOffset(0)
    , Size(0)
    , HeapIndex(UINT32_MAX)
    , StorageType(EMetalResourceStorageType::Unknown)
    , AllocatorType(EMetalAllocatorType::None)
{
}

FMetalResourceStorage::~FMetalResourceStorage()
{
    ReleaseResource();
}

void FMetalResourceStorage::InitStandalone(id<MTLBuffer> InBuffer, uint64 InSize, bool bBindlessReachable)
{
    Reset();
    Buffer      = InBuffer;
    Size        = InSize;
    StorageType = EMetalResourceStorageType::Standalone;

    GetDevice()->GetResidencySet().Add(InBuffer, bBindlessReachable);

    STAT_ADD(STAT_Metal_StandaloneBufferBytes, [InBuffer allocatedSize]);
    STAT_ADD(STAT_Metal_StandaloneBuffers, 1);
}

void FMetalResourceStorage::InitStandalone(id<MTLTexture> InTexture, uint64 InSize, bool bBindlessReachable)
{
    Reset();
    Texture     = InTexture;
    Size        = InSize;
    StorageType = EMetalResourceStorageType::Standalone;

    GetDevice()->GetResidencySet().Add(InTexture, bBindlessReachable);

    STAT_ADD(STAT_Metal_StandaloneTextureBytes, [InTexture allocatedSize]);
    STAT_ADD(STAT_Metal_StandaloneTextures, 1);
}

void FMetalResourceStorage::InitSuballocatedResource(id<MTLBuffer> InBuffer, uint64 InOffset, uint64 InSize, void* InMappedAddress, FMetalLinearAllocator* InLinearAllocator, FMetalUploadHeapAllocator* InUploadAllocator)
{
    Reset();
    Buffer            = InBuffer;
    ResourceOffset    = InOffset;
    Size              = InSize;
    MappedBaseAddress = InMappedAddress;
    StorageType       = EMetalResourceStorageType::SuballocatedResource;

    if (InLinearAllocator)
    {
        AllocatorPointers.LinearAllocator = InLinearAllocator;
        AllocatorType = EMetalAllocatorType::LinearAllocator;
    }
    else
    {
        AllocatorPointers.UploadHeapAllocator = InUploadAllocator;
        AllocatorType = EMetalAllocatorType::UploadHeapAllocator;
    }
}

void FMetalResourceStorage::InitSuballocatedHeap(id<MTLBuffer> InBuffer, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalBufferAllocator* InBufferAllocator)
{
    Reset();
    Buffer                            = InBuffer;
    Heap                              = InHeap;
    ResourceOffset                    = InOffset;
    Size                              = InSize;
    HeapIndex                         = InHeapIndex;
    StorageType                       = EMetalResourceStorageType::SuballocatedHeap;
    AllocatorType                     = EMetalAllocatorType::BufferAllocator;
    AllocatorPointers.BufferAllocator = InBufferAllocator;
}

void FMetalResourceStorage::InitSuballocatedHeap(id<MTLTexture> InTexture, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalTextureAllocator* InTextureAllocator)
{
    Reset();
    Texture                            = InTexture;
    Heap                               = InHeap;
    ResourceOffset                     = InOffset;
    Size                               = InSize;
    HeapIndex                          = InHeapIndex;
    StorageType                        = EMetalResourceStorageType::SuballocatedHeap;
    AllocatorType                      = EMetalAllocatorType::TextureAllocator;
    AllocatorPointers.TextureAllocator = InTextureAllocator;
}

void FMetalResourceStorage::ReleaseOwnedResource(bool bStandalone)
{
    const auto Defer = [bStandalone](id<MTLResource> Resource)
    {
        if (bStandalone)
        {
            FMetalDeviceRHI::DeferDeletion(FMetalDeferredObject::EType::StandaloneResource, Resource);
        }
        else
        {
            FMetalDeviceRHI::DeferDeletion(Resource);
        }

        [Resource release];
    };

    if (Buffer)
    {
        Defer(Buffer);
        Buffer = nil;
    }

    if (Texture)
    {
        Defer(Texture);
        Texture = nil;
    }
}

void FMetalResourceStorage::ReleaseResource()
{
    if (!IsValid())
    {
        return;
    }

    if (StorageType == EMetalResourceStorageType::SuballocatedHeap)
    {
        ReleaseOwnedResource(false);

        if (AllocatorType == EMetalAllocatorType::BufferAllocator && AllocatorPointers.BufferAllocator)
        {
            AllocatorPointers.BufferAllocator->Deallocate(*this);
        }
        else if (AllocatorType == EMetalAllocatorType::TextureAllocator && AllocatorPointers.TextureAllocator)
        {
            AllocatorPointers.TextureAllocator->Deallocate(*this);
        }
    }
    else if (StorageType == EMetalResourceStorageType::Standalone)
    {
        if (Buffer)
        {
            STAT_SUBTRACT(STAT_Metal_StandaloneBufferBytes, [Buffer allocatedSize]);
            STAT_SUBTRACT(STAT_Metal_StandaloneBuffers, 1);
        }

        if (Texture)
        {
            STAT_SUBTRACT(STAT_Metal_StandaloneTextureBytes, [Texture allocatedSize]);
            STAT_SUBTRACT(STAT_Metal_StandaloneTextures, 1);
        }

        ReleaseOwnedResource(true);
    }

    Reset();
}

void FMetalResourceStorage::Reset()
{
    Buffer            = nil;
    Texture           = nil;
    Heap              = nullptr;
    MappedBaseAddress = nullptr;
    ResourceOffset    = 0;
    Size              = 0;
    HeapIndex         = UINT32_MAX;
    StorageType       = EMetalResourceStorageType::Unknown;
    AllocatorType     = EMetalAllocatorType::None;
    AllocatorPointers.AsVoid = nullptr;
}
