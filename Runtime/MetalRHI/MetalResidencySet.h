#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "MetalRHI/MetalCore.h"
#include "MetalRHI/MetalDeviceChild.h"

class FMetalDevice;

class FMetalResidencySet : public FMetalDeviceChild, public FNonCopyAndNonMovable
{
public:
    FMetalResidencySet(FMetalDevice* InDevice);
    virtual ~FMetalResidencySet();

    void Initialize();
    void Release();

    void AddHeap(id<MTLHeap> Heap);
    void RemoveHeap(id<MTLHeap> Heap);

    void Add(id<MTLResource> Resource, bool bBindlessReachable);
    void Remove(id<MTLResource> Resource);
    void CommitIfDirty();

    template<typename EncoderType>
    uint64 DeclareForEncoder(EncoderType Encoder);

    uint64 GetFallbackGeneration() const
    {
        return FallbackGeneration.Load();
    }

    bool UsesEncoderFallback() const
    {
        return bUseEncoderFallback;
    }

private:
    id                       ResidencySet;
    TArray<id<MTLHeap>>      FallbackHeaps;
    TArray<id<MTLResource>>  FallbackResources;
    TMap<const void*, int32> FallbackIndices;
    FCriticalSection         CriticalSection;
    TAtomicInt<uint64>       FallbackGeneration;
    TAtomicInt<int32>        bDirty;
    bool                     bUseEncoderFallback;
};
