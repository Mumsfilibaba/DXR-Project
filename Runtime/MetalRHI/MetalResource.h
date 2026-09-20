#pragma once
#include "Core/Templates/Utility/NonCopyable.h"
#include "MetalRHI/MetalDeviceChild.h"

class FMetalHeap;
class FMetalLinearAllocator;
class FMetalBufferAllocator;
class FMetalTextureAllocator;
class FMetalUploadHeapAllocator;

enum class EMetalResourceStorageType : uint8
{
    Unknown              = 0,
    Standalone           = 1,
    SuballocatedHeap     = 2,
    SuballocatedResource = 3,
};

enum class EMetalAllocatorType : uint8
{
    None               = 0,
    LinearAllocator    = 1,
    UploadHeapAllocator = 2,
    BufferAllocator    = 3,
    TextureAllocator   = 4,
};

class FMetalResourceStorage : public FMetalDeviceChild, public FNonCopyable
{
public:
    explicit FMetalResourceStorage(FMetalDevice* InDevice);
    ~FMetalResourceStorage();

    void InitStandalone(id<MTLBuffer> InBuffer, uint64 InSize);
    void InitStandalone(id<MTLTexture> InTexture, uint64 InSize);
    void InitSuballocatedResource(id<MTLBuffer> InBuffer, uint64 InOffset, uint64 InSize, void* InMappedAddress, FMetalLinearAllocator* InLinearAllocator, FMetalUploadHeapAllocator* InUploadAllocator);
    void InitSuballocatedHeap(id<MTLBuffer> InBuffer, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalBufferAllocator* InBufferAllocator);
    void InitSuballocatedHeap(id<MTLTexture> InTexture, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalTextureAllocator* InTextureAllocator);

    void ReleaseResource();
    void Reset();

    FORCEINLINE bool IsValid() const
    {
        return StorageType != EMetalResourceStorageType::Unknown;
    }

    FORCEINLINE bool IsPlacedResource() const
    {
        return StorageType == EMetalResourceStorageType::SuballocatedHeap;
    }

    FORCEINLINE id<MTLBuffer> GetBuffer() const
    {
        return Buffer;
    }

    FORCEINLINE id<MTLTexture> GetTexture() const
    {
        return Texture;
    }

    FORCEINLINE FMetalHeap* GetHeap() const
    {
        return Heap;
    }

    FORCEINLINE uint64 GetResourceOffset() const
    {
        return ResourceOffset;
    }

    FORCEINLINE uint64 GetSize() const
    {
        return Size;
    }

    FORCEINLINE void* GetMappedBaseAddress() const
    {
        return MappedBaseAddress;
    }

    FORCEINLINE EMetalResourceStorageType GetStorageType() const
    {
        return StorageType;
    }

    FORCEINLINE uint32 GetHeapIndex() const
    {
        return HeapIndex;
    }

private:
    void ReleaseOwnedResource();

    union FAllocatorPointers
    {
        FMetalLinearAllocator*     LinearAllocator;
        FMetalUploadHeapAllocator* UploadHeapAllocator;
        FMetalBufferAllocator*     BufferAllocator;
        FMetalTextureAllocator*    TextureAllocator;
        void*                      AsVoid;

        FAllocatorPointers()
            : AsVoid(nullptr)
        {
        }
    } AllocatorPointers;

    id<MTLBuffer>             Buffer;
    id<MTLTexture>            Texture;
    FMetalHeap*               Heap;
    void*                     MappedBaseAddress;
    uint64                    ResourceOffset;
    uint64                    Size;
    uint32                    HeapIndex;
    EMetalResourceStorageType StorageType;
    EMetalAllocatorType       AllocatorType;
};
