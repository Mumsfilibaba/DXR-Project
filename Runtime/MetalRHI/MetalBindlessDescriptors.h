#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalShader.h"
#include "RHI/RHITypes.h"

struct FMetalBindlessDescriptorEntry
{
    uint64 Resource = 0;
};

static_assert(sizeof(FMetalBindlessDescriptorEntry) == 8, "Bindless table stride must match spvDescriptor<T>");

static constexpr uint8 MetalNullEntryWritableBase = MSL_NUM_NULL_TEXTURE_TYPES;
static constexpr uint8 MetalNullEntryBuffer       = 2 * MSL_NUM_NULL_TEXTURE_TYPES;
static constexpr uint8 MetalNullEntrySampler      = MetalNullEntryBuffer + 1;
static constexpr uint8 MetalNullEntryCount        = MetalNullEntrySampler + 1;

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

    void WriteTexture(FRHIDescriptorHandle Handle, id<MTLTexture> Texture, bool bWritable, bool bImmediate);
    void WriteBuffer(FRHIDescriptorHandle Handle, id<MTLBuffer> Buffer, uint64 Offset, bool bHeapPlaced, bool bImmediate);
    void WriteSampler(FRHIDescriptorHandle Handle, id<MTLSamplerState> Sampler, bool bImmediate);

    void Flush();

    id<MTLBuffer> GetResourceHeapBuffer() const { return ResourceHeap.Buffer; }
    id<MTLBuffer> GetSamplerHeapBuffer() const  { return SamplerHeap.Buffer; }

private:
    struct FSlotState
    {
        uint8 NullEntry = 0;
        bool  bOccupied = false;
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
        uint32                         NumOccupied = 0;
        uint32                         DirtyBegin  = UINT32_MAX;
        uint32                         DirtyEnd    = 0;

        TArray<uint32>                 FreeStack;
        TArray<FSlotState>             Slots;

        bool                           bSampler = false;
    };

    bool CreateTable(FHeap& Heap, uint32 Capacity, const CHAR* DebugName);
    void DestroyTable(FHeap& Heap);
    bool GrowTable(FHeap& Heap);
    void FillNullEntries(FHeap& Heap, uint32 FirstSlot, uint32 NumSlots);
    void MarkDirty(FHeap& Heap, uint32 FirstSlot, uint32 NumSlots);
    void FlushDirtyRange(FHeap& Heap);
    FHeap& GetHeap(EDescriptorType Type);
    const FHeap& GetHeap(EDescriptorType Type) const;
    uint32 AllocateSlot(FHeap& Heap);
    void WriteSlot(FHeap& Heap, uint32 SlotIndex, FMetalBindlessDescriptorEntry Entry, uint8 NullEntry, bool bImmediate);
    void UpdateStats();

    FMetalBindlessDescriptorEntry NullEntries[MetalNullEntryCount];
    FHeap                         ResourceHeap;
    FHeap                         SamplerHeap;
    FCriticalSection              AllocCS;
    TArray<FPendingWrite>         PendingWrites;
    FCriticalSection              PendingWritesCS;
    bool                          bEnabled;
    uint32                        HardMaxSlots;
};
