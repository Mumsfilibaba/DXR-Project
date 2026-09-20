#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalHeap.h"
#include "MetalRHI/MetalRHI.h"

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

void FMetalResourceStorage::InitStandalone(id<MTLBuffer> InBuffer, uint64 InSize)
{
    Reset();
    Buffer      = InBuffer;
    Size        = InSize;
    StorageType = EMetalResourceStorageType::Standalone;
}

void FMetalResourceStorage::InitStandalone(id<MTLTexture> InTexture, uint64 InSize)
{
    Reset();
    Texture     = InTexture;
    Size        = InSize;
    StorageType = EMetalResourceStorageType::Standalone;
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

void FMetalResourceStorage::ReleaseOwnedResource()
{
    if (Buffer)
    {
        FMetalDeviceRHI::DeferDeletion(Buffer);
        [Buffer release];
        Buffer = nil;
    }

    if (Texture)
    {
        FMetalDeviceRHI::DeferDeletion(Texture);
        [Texture release];
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
        ReleaseOwnedResource();

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
        ReleaseOwnedResource();
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
