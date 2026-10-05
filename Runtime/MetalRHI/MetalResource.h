#pragma once
#include "Core/Templates/Utility/NonCopyable.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalResidencyManager.h"

class FMetalHeap;
class FMetalRelocatable;
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

    void InitStandalone(id<MTLBuffer> InBuffer, uint64 InSize, bool bBindlessReachable);
    void InitStandalone(id<MTLTexture> InTexture, uint64 InSize, bool bBindlessReachable);
    void InitStandalone(id<MTLAccelerationStructure> InAccelerationStructure, uint64 InSize);
    void InitSuballocatedResource(id<MTLBuffer> InBuffer, uint64 InOffset, uint64 InSize, void* InMappedAddress, FMetalLinearAllocator* InLinearAllocator, FMetalUploadHeapAllocator* InUploadAllocator);
    void InitSuballocatedHeap(id<MTLBuffer> InBuffer, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalBufferAllocator* InBufferAllocator);
    void InitSuballocatedHeap(id<MTLTexture> InTexture, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalTextureAllocator* InTextureAllocator);
    void InitSuballocatedHeap(id<MTLAccelerationStructure> InAccelerationStructure, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalBufferAllocator* InBufferAllocator);

    void ReleaseResource();
    void Reset();

    void SetOwner(FMetalRelocatable* InOwner)
    {
        Owner = InOwner;
    }

    void SwapPlacement(FMetalResourceStorage& Other);

    void SetDefragPending(bool bPending)
    {
        bDefragPending = bPending;
    }

    FORCEINLINE bool IsDefragPending() const
    {
        return bDefragPending;
    }

    FORCEINLINE FMetalRelocatable* GetOwner() const
    {
        return Owner;
    }

    FORCEINLINE FMetalResidencyEntry* GetResidencyEntry() const
    {
        return ResidencyEntry;
    }

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

    FORCEINLINE id<MTLAccelerationStructure> GetAccelerationStructure() const
    {
        return AccelerationStructure;
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
    void ReleaseOwnedResource(bool bStandalone);
    void TrackStandalone(id<MTLResource> Resource, bool bBindlessReachable);

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

    id<MTLBuffer>                Buffer;
    id<MTLTexture>               Texture;
    id<MTLAccelerationStructure> AccelerationStructure;
    FMetalHeap*                  Heap;
    FMetalRelocatable*           Owner;
    FMetalResidencyEntry*        ResidencyEntry;
    FMetalResidencyEntry         StandaloneEntry;
    void*                        MappedBaseAddress;
    uint64                       ResourceOffset;
    uint64                       Size;
    uint32                       HeapIndex;
    EMetalResourceStorageType    StorageType;
    EMetalAllocatorType          AllocatorType;
    bool                         bDefragPending;
};
