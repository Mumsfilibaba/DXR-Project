#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalShader.h"
#include "RHI/RHITypes.h"

class FMetalCommandContext;

/** @brief One SPIRV-Cross `spvDescriptor<T>` slot. */
struct FMetalBindlessDescriptorEntry
{
    uint64 Resource = 0;
};

static_assert(sizeof(FMetalBindlessDescriptorEntry) == 8, "Bindless table stride must match spvDescriptor<T>");

class METALRHI_API FMetalBindlessDescriptorManager : public FMetalDeviceChild
{
public:
    explicit FMetalBindlessDescriptorManager(FMetalDevice* InDevice);
    ~FMetalBindlessDescriptorManager();

    bool Initialize();

    NODISCARD bool IsEnabled() const
    {
        return bEnabled;
    }

    NODISCARD FRHIDescriptorHandle Allocate(EDescriptorType InType);

    void Free(FRHIDescriptorHandle Handle);

    void RecycleSlot(FRHIDescriptorHandle Handle);

    void WriteTexture(FRHIDescriptorHandle Handle, id<MTLTexture> Texture, bool bWritable, bool bIsView, bool bImmediate);
    void WriteBuffer(FRHIDescriptorHandle Handle, id<MTLBuffer> Buffer, uint64 Offset, bool bWritable, bool bIsView, bool bHeapPlaced, bool bImmediate);
    void WriteSampler(FRHIDescriptorHandle Handle, id<MTLSamplerState> Sampler, bool bImmediate);
    void WriteAccelerationStructure(FRHIDescriptorHandle Handle, id<MTLAccelerationStructure> AccelerationStructure, bool bImmediate);

    void Flush();
    void BindHeaps(FMetalCommandContext& Context, EShaderVisibility::Type Stage, uint8 ResourceSlot, uint8 SamplerSlot);
    void DeclareResidency(FMetalCommandContext& Context);

    NODISCARD uint64 ReadResourceEntry(uint32 Index) const;
    NODISCARD uint64 ReadSamplerEntry(uint32 Index) const;

private:
    struct FSlotState
    {
        id<MTLResource> Resource  = nil;
        bool            bOccupied = false;
        bool            bWritable = false;
        bool            bIsView   = false;
    };

    struct FPendingWrite
    {
        uint32                        SlotIndex = 0;
        FMetalBindlessDescriptorEntry Entry;
        bool                          bSampler  = false;
    };

    struct FHeap
    {
        id<MTLBuffer>                  Buffer      = nil;
        FMetalBindlessDescriptorEntry* Mapped      = nullptr;
        uint32                         Capacity    = 0;
        uint32                         NextFresh   = 0;
        uint32                         GrowthCount = 0;

        TArray<uint32>                 FreeStack;
        TArray<FSlotState>             Slots;
        TArray<uint32>                 Occupied;

        bool                           bSampler = false;
    };

    bool CreateTable(FHeap& Heap, uint32 Capacity, const CHAR* DebugName);
    void DestroyTable(FHeap& Heap);
    bool GrowTable(FHeap& Heap);
    void NotifyTableModified(FHeap& Heap, uint32 SlotIndex);
    FHeap& GetHeap(EDescriptorType Type);
    const FHeap& GetHeap(EDescriptorType Type) const;
    uint32 AllocateSlot(FHeap& Heap);
    void WriteSlot(FHeap& Heap, uint32 SlotIndex, FMetalBindlessDescriptorEntry Entry, id<MTLResource> Resource, bool bWritable, bool bIsView, bool bImmediate);
    void UpdateStats();

    FHeap                 ResourceHeap;
    FHeap                 SamplerHeap;
    FCriticalSection      AllocCS;
    TArray<FPendingWrite> PendingWrites;
    FCriticalSection      PendingWritesCS;
    bool                  bEnabled;
    uint32                HardMaxSlots;
};
