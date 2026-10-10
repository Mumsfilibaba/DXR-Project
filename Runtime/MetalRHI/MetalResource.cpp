#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalAllocators.h"
#include "MetalRHI/MetalHeap.h"
#include "MetalRHI/MetalResidencySet.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Templates/Utility/Swap.h"

FMetalResourceStorage::FMetalResourceStorage(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , AllocatorPointers()
    , Buffer(nil)
    , Texture(nil)
    , AccelerationStructure(nil)
    , Heap(nullptr)
    , Owner(nullptr)
    , ResidencyEntry(nullptr)
    , StandaloneEntry()
    , MappedBaseAddress(nullptr)
    , ResourceOffset(0)
    , Size(0)
    , HeapIndex(UINT32_MAX)
    , StorageType(EMetalResourceStorageType::Unknown)
    , AllocatorType(EMetalAllocatorType::None)
    , bDefragPending(false)
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

    TrackStandalone(InBuffer, bBindlessReachable);

    STAT_ADD(STAT_Metal_StandaloneBufferBytes, [InBuffer allocatedSize]);
    STAT_ADD(STAT_Metal_StandaloneBuffers, 1);
}

void FMetalResourceStorage::InitStandalone(id<MTLTexture> InTexture, uint64 InSize, bool bBindlessReachable)
{
    Reset();
    Texture     = InTexture;
    Size        = InSize;
    StorageType = EMetalResourceStorageType::Standalone;

    TrackStandalone(InTexture, bBindlessReachable);

    STAT_ADD(STAT_Metal_StandaloneTextureBytes, [InTexture allocatedSize]);
    STAT_ADD(STAT_Metal_StandaloneTextures, 1);
}

void FMetalResourceStorage::InitStandalone(id<MTLAccelerationStructure> InAccelerationStructure, uint64 InSize)
{
    Reset();
    AccelerationStructure = InAccelerationStructure;
    Size                  = InSize;
    StorageType           = EMetalResourceStorageType::Standalone;

    TrackStandalone(InAccelerationStructure, true);

    STAT_ADD(STAT_Metal_StandaloneAccelerationStructureBytes, [InAccelerationStructure allocatedSize]);
    STAT_ADD(STAT_Metal_StandaloneAccelerationStructures, 1);
}

void FMetalResourceStorage::TrackStandalone(id<MTLResource> Resource, bool bBindlessReachable)
{
    GetDevice()->TrackCPUVisibleBytes(Resource.storageMode, static_cast<int64>(Resource.allocatedSize));

    if (Resource.storageMode != MTLStorageModePrivate)
    {
        GetDevice()->GetResidencySet().Add(Resource, bBindlessReachable);
        return;
    }

    StandaloneEntry.Allocation         = Resource;
    StandaloneEntry.SizeInBytes        = [Resource allocatedSize];
    StandaloneEntry.bIsHeap            = false;
    StandaloneEntry.bBindlessReachable = bBindlessReachable;
    ResidencyEntry                     = &StandaloneEntry;

    GetDevice()->GetResidencyManager().BeginTracking(StandaloneEntry);
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
    ResidencyEntry                    = &InHeap->GetResidencyEntry();
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
    ResidencyEntry                     = &InHeap->GetResidencyEntry();
    ResourceOffset                     = InOffset;
    Size                               = InSize;
    HeapIndex                          = InHeapIndex;
    StorageType                        = EMetalResourceStorageType::SuballocatedHeap;
    AllocatorType                      = EMetalAllocatorType::TextureAllocator;
    AllocatorPointers.TextureAllocator = InTextureAllocator;
}

void FMetalResourceStorage::InitSuballocatedHeap(id<MTLAccelerationStructure> InAccelerationStructure, FMetalHeap* InHeap, uint64 InOffset, uint64 InSize, uint32 InHeapIndex, FMetalBufferAllocator* InBufferAllocator)
{
    Reset();
    AccelerationStructure             = InAccelerationStructure;
    Heap                              = InHeap;
    ResidencyEntry                    = &InHeap->GetResidencyEntry();
    ResourceOffset                    = InOffset;
    Size                              = InSize;
    HeapIndex                         = InHeapIndex;
    StorageType                       = EMetalResourceStorageType::SuballocatedHeap;
    AllocatorType                     = EMetalAllocatorType::BufferAllocator;
    AllocatorPointers.BufferAllocator = InBufferAllocator;
}

void FMetalResourceStorage::SwapPlacement(FMetalResourceStorage& Other)
{
    CHECK(IsPlacedResource() && Other.IsPlacedResource());
    CHECK(AllocatorType == Other.AllocatorType);
    CHECK(!AccelerationStructure && !Other.AccelerationStructure);

    ::Swap(AllocatorPointers, Other.AllocatorPointers);
    ::Swap(Buffer, Other.Buffer);
    ::Swap(Texture, Other.Texture);
    ::Swap(Heap, Other.Heap);
    ::Swap(ResidencyEntry, Other.ResidencyEntry);
    ::Swap(MappedBaseAddress, Other.MappedBaseAddress);
    ::Swap(ResourceOffset, Other.ResourceOffset);
    ::Swap(Size, Other.Size);
    ::Swap(HeapIndex, Other.HeapIndex);

    bDefragPending       = false;
    Other.bDefragPending = false;

    if (AllocatorType == EMetalAllocatorType::BufferAllocator)
    {
        AllocatorPointers.BufferAllocator->RetargetAllocation(*this);
        Other.AllocatorPointers.BufferAllocator->RetargetAllocation(Other);
    }
    else
    {
        AllocatorPointers.TextureAllocator->RetargetAllocation(*this);
        Other.AllocatorPointers.TextureAllocator->RetargetAllocation(Other);
    }
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

    if (AccelerationStructure)
    {
        Defer(AccelerationStructure);
        AccelerationStructure = nil;
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
        if (bDefragPending)
        {
            GetDevice()->CancelDefragMove(*this);
        }

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
        if (ResidencyEntry == &StandaloneEntry)
        {
            GetDevice()->GetResidencyManager().EndTracking(StandaloneEntry, false);
        }

        if (id<MTLResource> Resource = Buffer ? id<MTLResource>(Buffer) : (Texture ? id<MTLResource>(Texture) : id<MTLResource>(AccelerationStructure)))
        {
            GetDevice()->TrackCPUVisibleBytes(Resource.storageMode, -static_cast<int64>(Resource.allocatedSize));
        }

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

        if (AccelerationStructure)
        {
            STAT_SUBTRACT(STAT_Metal_StandaloneAccelerationStructureBytes, [AccelerationStructure allocatedSize]);
            STAT_SUBTRACT(STAT_Metal_StandaloneAccelerationStructures, 1);
        }

        ReleaseOwnedResource(true);
    }

    Reset();
}

void FMetalResourceStorage::Reset()
{
    CHECK(!StandaloneEntry.bTracked);

    Buffer                   = nil;
    Texture                  = nil;
    AccelerationStructure    = nil;
    Heap                     = nullptr;
    ResidencyEntry           = nullptr;
    MappedBaseAddress        = nullptr;
    ResourceOffset           = 0;
    Size                     = 0;
    HeapIndex                = UINT32_MAX;
    StorageType              = EMetalResourceStorageType::Unknown;
    AllocatorType            = EMetalAllocatorType::None;
    bDefragPending           = false;
    AllocatorPointers.AsVoid = nullptr;

    StandaloneEntry.Allocation    = nil;
    StandaloneEntry.SizeInBytes   = 0;
    StandaloneEntry.LastUsedFrame = 0;
    StandaloneEntry.BindlessPins.Store(0);
}
