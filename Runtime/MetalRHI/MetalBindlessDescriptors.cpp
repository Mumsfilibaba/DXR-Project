#include "MetalRHI/MetalBindlessDescriptors.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalRHI.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "RHI/MSLShaderBindings.h"
#include <string.h>

static TAutoConsoleVariable<bool> CVarEnableBindless(
    "MetalRHI.EnableBindless",
    "When enabled, allocates shared descriptor table buffers for SM 6.6 heap indexing. Requires Metal 3 gpuResourceID.",
    true);

static TAutoConsoleVariable<int32> CVarNumBindlessResourceDescriptors(
    "MetalRHI.NumBindlessResourceDescriptors",
    "Initial CBV/SRV/UAV slot count for the Metal bindless resource table. The table grows when this is exhausted.",
    100000);

static TAutoConsoleVariable<int32> CVarNumBindlessSamplerDescriptors(
    "MetalRHI.NumBindlessSamplerDescriptors",
    "Initial sampler slot count for the Metal bindless sampler table. The table grows when this is exhausted.",
    2048);

static constexpr uint32 MetalBindlessHardMaxSlots = 1u << 20;

static constexpr uint8 MetalNullEntryDefaultResource = MakeMSLNullTextureType(EMSLTextureDimension::Texture2D, EMSLTextureComponent::Float);

static uint64 MetalCopyResourceID(id Object)
{
    if (!Object)
    {
        return 0;
    }

    const MTLResourceID ResourceID = [Object gpuResourceID];
    uint64 Value = 0;
    static_assert(sizeof(MTLResourceID) <= sizeof(uint64), "MTLResourceID must fit in one bindless slot");
    Memory::Memcpy(&Value, &ResourceID, sizeof(MTLResourceID));
    return Value;
}

FMetalBindlessDescriptorManager::FMetalBindlessDescriptorManager(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , NullEntries()
    , ResourceHeap()
    , SamplerHeap()
    , AllocCS()
    , PendingWrites()
    , PendingWritesCS()
    , bEnabled(false)
    , HardMaxSlots(0)
{
    ResourceHeap.bSampler = false;
    SamplerHeap.bSampler  = true;
}

FMetalBindlessDescriptorManager::~FMetalBindlessDescriptorManager()
{
    DestroyTable(ResourceHeap);
    DestroyTable(SamplerHeap);
}

bool FMetalBindlessDescriptorManager::Initialize()
{
    id<MTLDevice> Device = GetDevice()->GetMTLDevice();

    if (!Device || !CVarEnableBindless.GetValue())
    {
        return false;
    }

    if (!MetalRHI::SupportsMetal3() || GMetalFeatures.ArgumentBuffersTier != MTLArgumentBuffersTier2)
    {
        METAL_INFO("Bindless descriptors are disabled; they require a Metal 3 GPU with Tier 2 argument buffers");
        return false;
    }

    HardMaxSlots = MetalBindlessHardMaxSlots;

    const FMetalDefaultResources& Defaults = GetDevice()->GetDefaultResources();
    for (uint8 NullTextureType = 0; NullTextureType < MSL_NUM_NULL_TEXTURE_TYPES; ++NullTextureType)
    {
        NullEntries[NullTextureType].Resource                              = MetalCopyResourceID(Defaults.GetNullTexture(NullTextureType));
        NullEntries[MetalNullEntryWritableBase + NullTextureType].Resource = MetalCopyResourceID(Defaults.GetNullRWTexture(NullTextureType));
    }

    NullEntries[MetalNullEntryBuffer].Resource  = static_cast<uint64>([Defaults.NullBuffer gpuAddress]);
    NullEntries[MetalNullEntrySampler].Resource = MetalCopyResourceID(Defaults.DefaultSampler);

    const uint32 ResourceCount = static_cast<uint32>(Math::Max<int32>(1, CVarNumBindlessResourceDescriptors.GetValue()));
    const uint32 SamplerCount  = static_cast<uint32>(Math::Max<int32>(1, CVarNumBindlessSamplerDescriptors.GetValue()));

    if (!CreateTable(ResourceHeap, Math::Min(ResourceCount, HardMaxSlots), "MetalBindlessResourceHeap") ||
        !CreateTable(SamplerHeap, Math::Min(SamplerCount, HardMaxSlots), "MetalBindlessSamplerHeap"))
    {
        DestroyTable(ResourceHeap);
        DestroyTable(SamplerHeap);
        METAL_ERROR("Failed to create Metal bindless descriptor tables");
        return false;
    }

    bEnabled = true;

    UpdateStats();
    METAL_INFO("Bindless descriptor tables ready. Resources=%u Samplers=%u", ResourceHeap.Capacity, SamplerHeap.Capacity);
    return true;
}

bool FMetalBindlessDescriptorManager::CreateTable(FHeap& Heap, uint32 Capacity, const CHAR* DebugName)
{
    DestroyTable(Heap);

    id<MTLDevice>    Device   = GetDevice()->GetMTLDevice();
    const NSUInteger ByteSize = static_cast<NSUInteger>(Capacity) * sizeof(FMetalBindlessDescriptorEntry);
    id<MTLBuffer>    Buffer   = [Device newBufferWithLength:ByteSize options:MetalRHI::GetMTLResourceOptions(EMetalMemoryClass::CPUWriteGPURead)];

    if (!Buffer)
    {
        return false;
    }

    Buffer.label = [NSString stringWithUTF8String:DebugName];

    Heap.Buffer     = Buffer;
    Heap.Mapped     = static_cast<FMetalBindlessDescriptorEntry*>([Buffer contents]);
    Heap.Capacity   = Capacity;
    Heap.NextFresh  = 0;

    FillNullEntries(Heap, 0, Capacity);
    MarkDirty(Heap, 0, Capacity);

    Heap.Slots.Resize(static_cast<int32>(Capacity));
    Heap.NumOccupied = 0;
    Heap.FreeStack.Clear();
    return true;
}

void FMetalBindlessDescriptorManager::FillNullEntries(FHeap& Heap, uint32 FirstSlot, uint32 NumSlots)
{
    static_assert(sizeof(FMetalBindlessDescriptorEntry) == 8, "memset_pattern8 writes one entry per pattern");

    const FMetalBindlessDescriptorEntry& Pattern = NullEntries[Heap.bSampler ? MetalNullEntrySampler : MetalNullEntryDefaultResource];
    memset_pattern8(Heap.Mapped + FirstSlot, &Pattern, static_cast<size_t>(NumSlots) * sizeof(FMetalBindlessDescriptorEntry));
}

void FMetalBindlessDescriptorManager::DestroyTable(FHeap& Heap)
{
    if (Heap.Buffer)
    {
        [Heap.Buffer release];
        Heap.Buffer = nil;
    }

    Heap.Mapped     = nullptr;
    Heap.Capacity   = 0;
    Heap.NextFresh  = 0;
    Heap.DirtyBegin = UINT32_MAX;
    Heap.DirtyEnd   = 0;

    Heap.Slots.Clear();
    Heap.NumOccupied = 0;
    Heap.FreeStack.Clear();
}

bool FMetalBindlessDescriptorManager::GrowTable(FHeap& Heap)
{
    const uint32 Grown = Math::Min(HardMaxSlots, Math::Max(Heap.Capacity * 2u, Heap.Capacity + 64u));

    if (Grown <= Heap.Capacity)
    {
        METAL_ERROR("Bindless %s table exhausted at %u slots", Heap.bSampler ? "sampler" : "resource", Heap.Capacity);
        return false;
    }

    METAL_INFO("Growing Metal bindless %s table from %u to %u slots", Heap.bSampler ? "sampler" : "resource", Heap.Capacity, Grown);

    id<MTLDevice>    Device    = GetDevice()->GetMTLDevice();
    const NSUInteger ByteSize  = static_cast<NSUInteger>(Grown) * sizeof(FMetalBindlessDescriptorEntry);
    id<MTLBuffer>    NewBuffer = [Device newBufferWithLength:ByteSize options:MetalRHI::GetMTLResourceOptions(EMetalMemoryClass::CPUWriteGPURead)];

    if (!NewBuffer)
    {
        METAL_ERROR("Failed to grow the Metal bindless %s table", Heap.bSampler ? "sampler" : "resource");
        return false;
    }

    NewBuffer.label = Heap.bSampler ? @"MetalBindlessSamplerHeap" : @"MetalBindlessResourceHeap";

    TScopedLock Lock(PendingWritesCS);

    if (Heap.Mapped && Heap.Capacity > 0)
    {
        Memory::Memcpy([NewBuffer contents], Heap.Mapped, Heap.Capacity * sizeof(FMetalBindlessDescriptorEntry));
    }

    if (Heap.Buffer)
    {
        FMetalDeviceRHI::DeferDeletion(static_cast<id<MTLResource>>(Heap.Buffer));
        [Heap.Buffer release];
    }

    const uint32 OldCapacity = Heap.Capacity;
    Heap.Buffer   = NewBuffer;
    Heap.Mapped   = static_cast<FMetalBindlessDescriptorEntry*>([NewBuffer contents]);
    Heap.Capacity = Grown;

    FillNullEntries(Heap, OldCapacity, Grown - OldCapacity);
    Heap.DirtyBegin = UINT32_MAX;
    Heap.DirtyEnd   = 0;
    MarkDirty(Heap, 0, Grown);

    Heap.Slots.Resize(static_cast<int32>(Grown));
    Heap.GrowthCount++;

    UpdateStats();
    return true;
}

void FMetalBindlessDescriptorManager::MarkDirty(FHeap& Heap, uint32 FirstSlot, uint32 NumSlots)
{
    Heap.DirtyBegin = Math::Min(Heap.DirtyBegin, FirstSlot);
    Heap.DirtyEnd   = Math::Max(Heap.DirtyEnd, FirstSlot + NumSlots);
}

void FMetalBindlessDescriptorManager::FlushDirtyRange(FHeap& Heap)
{
    if (Heap.DirtyEnd <= Heap.DirtyBegin)
    {
        return;
    }

    MetalRHI::FlushCPUWrite(Heap.Buffer, Heap.DirtyBegin * sizeof(FMetalBindlessDescriptorEntry), (Heap.DirtyEnd - Heap.DirtyBegin) * sizeof(FMetalBindlessDescriptorEntry));
    Heap.DirtyBegin = UINT32_MAX;
    Heap.DirtyEnd   = 0;
}

FMetalBindlessDescriptorManager::FHeap& FMetalBindlessDescriptorManager::GetHeap(EDescriptorType Type)
{
    return (Type == EDescriptorType::Sampler) ? SamplerHeap : ResourceHeap;
}

const FMetalBindlessDescriptorManager::FHeap& FMetalBindlessDescriptorManager::GetHeap(EDescriptorType Type) const
{
    return (Type == EDescriptorType::Sampler) ? SamplerHeap : ResourceHeap;
}

uint32 FMetalBindlessDescriptorManager::AllocateSlot(FHeap& Heap)
{
    TScopedLock Lock(AllocCS);

    uint32 SlotIndex = FRHIDescriptorHandle::InvalidHandle;

    if (!Heap.FreeStack.IsEmpty())
    {
        SlotIndex = Heap.FreeStack.Last();
        Heap.FreeStack.Pop();
    }
    else
    {
        if (Heap.NextFresh >= Heap.Capacity && !GrowTable(Heap))
        {
            return FRHIDescriptorHandle::InvalidHandle;
        }

        SlotIndex = Heap.NextFresh++;
    }

    FSlotState& Slot = Heap.Slots[static_cast<int32>(SlotIndex)];
    Slot.bOccupied = true;
    Slot.NullEntry = Heap.bSampler ? MetalNullEntrySampler : MetalNullEntryDefaultResource;
    Heap.NumOccupied++;
    return SlotIndex;
}

FRHIDescriptorHandle FMetalBindlessDescriptorManager::Allocate(EDescriptorType InType)
{
    if (!bEnabled || InType == EDescriptorType::Unknown)
    {
        return FRHIDescriptorHandle();
    }

    FHeap& Heap = GetHeap(InType);
    const uint32 SlotIndex = AllocateSlot(Heap);

    if (SlotIndex == FRHIDescriptorHandle::InvalidHandle)
    {
        return FRHIDescriptorHandle();
    }

    UpdateStats();
    return FRHIDescriptorHandle(InType, SlotIndex);
}

void FMetalBindlessDescriptorManager::Free(FRHIDescriptorHandle Handle)
{
    if (!bEnabled || !Handle.IsValid())
    {
        return;
    }

    FMetalDeviceRHI::DeferDeletion(this, Handle);
}

void FMetalBindlessDescriptorManager::RecycleSlot(FRHIDescriptorHandle Handle)
{
    if (!Handle.IsValid())
    {
        return;
    }

    FHeap& Heap = GetHeap(Handle.Type);
    const uint32 SlotIndex = Handle.Index;
    CHECK(SlotIndex < Heap.Capacity);

    TScopedLock Lock(AllocCS);
    FSlotState& Slot = Heap.Slots[static_cast<int32>(SlotIndex)];
    CHECK(Slot.bOccupied);
    Slot.bOccupied = false;

    if (Heap.Mapped)
    {
        TScopedLock WriteLock(PendingWritesCS);
        Heap.Mapped[SlotIndex] = NullEntries[Slot.NullEntry];
        MarkDirty(Heap, SlotIndex, 1);
    }

    Heap.NumOccupied--;
    Heap.FreeStack.Add(SlotIndex);
    UpdateStats();
}

void FMetalBindlessDescriptorManager::WriteSlot(
    FHeap& Heap,
    uint32 SlotIndex,
    FMetalBindlessDescriptorEntry Entry,
    uint8 NullEntry,
    bool bImmediate)
{
    CHECK(SlotIndex < Heap.Capacity);

    {
        TScopedLock Lock(AllocCS);
        Heap.Slots[static_cast<int32>(SlotIndex)].NullEntry = NullEntry;
    }

    if (bImmediate)
    {
        TScopedLock Lock(PendingWritesCS);

        if (Heap.Mapped)
        {
            Heap.Mapped[SlotIndex] = Entry;
            MarkDirty(Heap, SlotIndex, 1);
        }

        return;
    }

    TScopedLock Lock(PendingWritesCS);
    FPendingWrite& Write = PendingWrites.Emplace();
    Write.SlotIndex = SlotIndex;
    Write.Entry     = Entry;
    Write.bSampler  = Heap.bSampler;
}

void FMetalBindlessDescriptorManager::WriteTexture(FRHIDescriptorHandle Handle, id<MTLTexture> Texture, bool bWritable, bool bImmediate)
{
    if (!Handle.IsValid())
    {
        return;
    }

    const uint8 NullTextureType = Texture
        ? MetalRHI::GetNullTextureType(Texture.textureType, Texture.pixelFormat)
        : MetalNullEntryDefaultResource;
    const uint8 NullEntry       = bWritable ? MetalNullEntryWritableBase + NullTextureType : NullTextureType;

    const FMetalBindlessDescriptorEntry Entry = Texture
        ? FMetalBindlessDescriptorEntry{ MetalCopyResourceID(Texture) }
        : NullEntries[NullEntry];
    WriteSlot(GetHeap(Handle.Type), Handle.Index, Entry, NullEntry, bImmediate);
}

void FMetalBindlessDescriptorManager::WriteBuffer(FRHIDescriptorHandle Handle, id<MTLBuffer> Buffer, uint64 Offset, bool bHeapPlaced, bool bImmediate)
{
    if (!Handle.IsValid())
    {
        return;
    }

    const FMetalBindlessDescriptorEntry Entry = Buffer
        ? FMetalBindlessDescriptorEntry{ static_cast<uint64>([Buffer gpuAddress]) + (bHeapPlaced ? 0 : Offset) }
        : NullEntries[MetalNullEntryBuffer];
    WriteSlot(GetHeap(Handle.Type), Handle.Index, Entry, MetalNullEntryBuffer, bImmediate);
}

void FMetalBindlessDescriptorManager::WriteSampler(FRHIDescriptorHandle Handle, id<MTLSamplerState> Sampler, bool bImmediate)
{
    if (!Handle.IsValid())
    {
        return;
    }

    const FMetalBindlessDescriptorEntry Entry = Sampler
        ? FMetalBindlessDescriptorEntry{ MetalCopyResourceID(Sampler) }
        : NullEntries[MetalNullEntrySampler];
    WriteSlot(SamplerHeap, Handle.Index, Entry, MetalNullEntrySampler, bImmediate);
}

void FMetalBindlessDescriptorManager::Flush()
{
    TScopedLock Lock(PendingWritesCS);

    if (!PendingWrites.IsEmpty())
    {
        for (const FPendingWrite& Write : PendingWrites)
        {
            FHeap& Heap = Write.bSampler ? SamplerHeap : ResourceHeap;

            if (!Heap.Mapped || Write.SlotIndex >= Heap.Capacity)
            {
                continue;
            }

            Heap.Mapped[Write.SlotIndex] = Write.Entry;
            MarkDirty(Heap, Write.SlotIndex, 1);
        }

        PendingWrites.Clear();
        UpdateStats();
    }

    FlushDirtyRange(ResourceHeap);
    FlushDirtyRange(SamplerHeap);
}

void FMetalBindlessDescriptorManager::UpdateStats()
{
#if METAL_ENABLE_STATS
    STAT_SET(STAT_Metal_BindlessResourceSlots, ResourceHeap.NumOccupied);
    STAT_SET(STAT_Metal_BindlessSamplerSlots, SamplerHeap.NumOccupied);
    STAT_SET(STAT_Metal_BindlessTableBytes,
        static_cast<int64>(ResourceHeap.Capacity + SamplerHeap.Capacity) * static_cast<int64>(sizeof(FMetalBindlessDescriptorEntry)));
    STAT_SET(STAT_Metal_BindlessPendingWrites, PendingWrites.Size());
    STAT_SET(STAT_Metal_BindlessGrowthCount, ResourceHeap.GrowthCount + SamplerHeap.GrowthCount);
#endif
}
