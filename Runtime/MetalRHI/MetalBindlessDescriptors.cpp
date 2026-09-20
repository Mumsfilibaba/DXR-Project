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

static uint64 MetalCopyResourceID(id Object)
{
    if (!Object || ![Object respondsToSelector:@selector(gpuResourceID)])
    {
        return 0;
    }

    const MTLResourceID ResourceID = [Object gpuResourceID];
    uint64 Value = 0;
    static_assert(sizeof(MTLResourceID) <= sizeof(uint64), "MTLResourceID must fit in one bindless slot");
    Memory::Memcpy(&Value, &ResourceID, sizeof(MTLResourceID));
    return Value;
}

static uint64 MetalBufferGpuAddress(id<MTLBuffer> Buffer, uint64 Offset)
{
    if (!Buffer || ![Buffer respondsToSelector:@selector(gpuAddress)])
    {
        return 0;
    }

    return static_cast<uint64>([Buffer gpuAddress]) + Offset;
}

FMetalBindlessDescriptorManager::FMetalBindlessDescriptorManager(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
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

    bool bHasBindlessABI = false;
    if (@available(macOS 13.0, *))
    {
        bHasBindlessABI = true;
    }

    if (!bHasBindlessABI)
    {
        METAL_INFO("Bindless descriptors are disabled; Metal 3 gpuResourceID requires macOS 13 or later");
        return false;
    }

    HardMaxSlots = MetalBindlessHardMaxSlots;

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
    id<MTLBuffer>    Buffer   = [Device newBufferWithLength:ByteSize options:MTLResourceStorageModeShared];

    if (!Buffer)
    {
        return false;
    }

    Buffer.label = [NSString stringWithUTF8String:DebugName];
    Memory::Memzero([Buffer contents], ByteSize);

    Heap.Buffer     = Buffer;
    Heap.Mapped     = static_cast<FMetalBindlessDescriptorEntry*>([Buffer contents]);
    Heap.Capacity   = Capacity;
    Heap.NextFresh  = 0;

    Heap.Slots.Resize(static_cast<int32>(Capacity));
    Heap.Occupied.Clear();
    Heap.FreeStack.Clear();
    return true;
}

void FMetalBindlessDescriptorManager::DestroyTable(FHeap& Heap)
{
    for (FSlotState& Slot : Heap.Slots)
    {
        if (Slot.Resource)
        {
            [Slot.Resource release];
            Slot.Resource = nil;
        }
    }

    if (Heap.Buffer)
    {
        [Heap.Buffer release];
        Heap.Buffer = nil;
    }

    Heap.Mapped    = nullptr;
    Heap.Capacity  = 0;
    Heap.NextFresh = 0;

    Heap.Slots.Clear();
    Heap.Occupied.Clear();
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
    id<MTLBuffer>    NewBuffer = [Device newBufferWithLength:ByteSize options:MTLResourceStorageModeShared];

    if (!NewBuffer)
    {
        METAL_ERROR("Failed to grow the Metal bindless %s table", Heap.bSampler ? "sampler" : "resource");
        return false;
    }

    NewBuffer.label = Heap.bSampler ? @"MetalBindlessSamplerHeap" : @"MetalBindlessResourceHeap";
    Memory::Memzero([NewBuffer contents], ByteSize);

    if (Heap.Mapped && Heap.Capacity > 0)
    {
        Memory::Memcpy([NewBuffer contents], Heap.Mapped, Heap.Capacity * sizeof(FMetalBindlessDescriptorEntry));
    }

    if (Heap.Buffer)
    {
        FMetalDeviceRHI::DeferDeletion(static_cast<id<MTLResource>>(Heap.Buffer));
        [Heap.Buffer release];
    }

    Heap.Buffer   = NewBuffer;
    Heap.Mapped   = static_cast<FMetalBindlessDescriptorEntry*>([NewBuffer contents]);
    Heap.Capacity = Grown;

    Heap.Slots.Resize(static_cast<int32>(Grown));
    Heap.GrowthCount++;

    UpdateStats();
    return true;
}

void FMetalBindlessDescriptorManager::NotifyTableModified(FHeap& Heap, uint32 SlotIndex)
{
    if (!Heap.Buffer || Heap.Buffer.storageMode != MTLStorageModeManaged)
    {
        return;
    }

    const NSRange Range = NSMakeRange(SlotIndex * sizeof(FMetalBindlessDescriptorEntry), sizeof(FMetalBindlessDescriptorEntry));
    [Heap.Buffer didModifyRange:Range];
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

    Heap.Slots[static_cast<int32>(SlotIndex)].bOccupied = true;
    Heap.Occupied.Add(SlotIndex);
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
    if (Slot.Resource)
    {
        [Slot.Resource release];
        Slot.Resource = nil;
    }

    Slot.bOccupied = false;
    Slot.bWritable = false;
    Slot.bIsView   = false;

    if (Heap.Mapped)
    {
        Heap.Mapped[SlotIndex].Resource = 0;
        NotifyTableModified(Heap, SlotIndex);
    }

    for (int32 Index = Heap.Occupied.Size() - 1; Index >= 0; --Index)
    {
        if (Heap.Occupied[Index] == SlotIndex)
        {
            Heap.Occupied.RemoveAt(Index);
            break;
        }
    }

    Heap.FreeStack.Add(SlotIndex);
    UpdateStats();
}

void FMetalBindlessDescriptorManager::WriteSlot(
    FHeap& Heap,
    uint32 SlotIndex,
    FMetalBindlessDescriptorEntry Entry,
    id<MTLResource> Resource,
    bool bWritable,
    bool bIsView,
    bool bImmediate)
{
    CHECK(SlotIndex < Heap.Capacity);

    {
        TScopedLock Lock(AllocCS);
        FSlotState& Slot = Heap.Slots[static_cast<int32>(SlotIndex)];
        if (Slot.Resource != Resource)
        {
            [Resource retain];
            [Slot.Resource release];
            Slot.Resource = Resource;
        }

        Slot.bWritable = bWritable;
        Slot.bIsView   = bIsView;
    }

    if (bImmediate)
    {
        TScopedLock Lock(PendingWritesCS);
        if (Heap.Mapped)
        {
            Heap.Mapped[SlotIndex] = Entry;
            NotifyTableModified(Heap, SlotIndex);
        }
        return;
    }

    TScopedLock Lock(PendingWritesCS);
    FPendingWrite& Write = PendingWrites.Emplace();
    Write.SlotIndex = SlotIndex;
    Write.Entry     = Entry;
    Write.bSampler  = Heap.bSampler;
}

void FMetalBindlessDescriptorManager::WriteTexture(FRHIDescriptorHandle Handle, id<MTLTexture> Texture, bool bWritable, bool bIsView, bool bImmediate)
{
    if (!Handle.IsValid() || !Texture)
    {
        return;
    }

    FMetalBindlessDescriptorEntry Entry;
    Entry.Resource = MetalCopyResourceID(Texture);
    WriteSlot(GetHeap(Handle.Type), Handle.Index, Entry, Texture, bWritable, bIsView, bImmediate);
}

void FMetalBindlessDescriptorManager::WriteBuffer(FRHIDescriptorHandle Handle, id<MTLBuffer> Buffer, uint64 Offset, bool bWritable, bool bIsView, bool bHeapPlaced, bool bImmediate)
{
    UNREFERENCED_VARIABLE(bHeapPlaced);
    if (!Handle.IsValid() || !Buffer)
    {
        return;
    }

    FMetalBindlessDescriptorEntry Entry;
    Entry.Resource = MetalBufferGpuAddress(Buffer, Offset);
    WriteSlot(GetHeap(Handle.Type), Handle.Index, Entry, Buffer, bWritable, bIsView, bImmediate);
}

void FMetalBindlessDescriptorManager::WriteSampler(FRHIDescriptorHandle Handle, id<MTLSamplerState> Sampler, bool bImmediate)
{
    if (!Handle.IsValid() || !Sampler)
    {
        return;
    }

    FMetalBindlessDescriptorEntry Entry;
    Entry.Resource = MetalCopyResourceID(Sampler);
    WriteSlot(SamplerHeap, Handle.Index, Entry, nil, false, false, bImmediate);
}

void FMetalBindlessDescriptorManager::WriteAccelerationStructure(FRHIDescriptorHandle Handle, id<MTLAccelerationStructure> AccelerationStructure, bool bImmediate)
{
    if (!Handle.IsValid() || !AccelerationStructure)
    {
        return;
    }

    FMetalBindlessDescriptorEntry Entry;
    Entry.Resource = MetalCopyResourceID(AccelerationStructure);
    WriteSlot(ResourceHeap, Handle.Index, Entry, AccelerationStructure, false, true, bImmediate);
}

void FMetalBindlessDescriptorManager::Flush()
{
    TArray<FPendingWrite> LocalWrites;
    {
        TScopedLock Lock(PendingWritesCS);
        if (PendingWrites.IsEmpty())
        {
            return;
        }

        LocalWrites = Move(PendingWrites);
        PendingWrites.Clear();
    }

    for (const FPendingWrite& Write : LocalWrites)
    {
        FHeap& Heap = Write.bSampler ? SamplerHeap : ResourceHeap;
        if (!Heap.Mapped || Write.SlotIndex >= Heap.Capacity)
        {
            continue;
        }

        Heap.Mapped[Write.SlotIndex] = Write.Entry;
        NotifyTableModified(Heap, Write.SlotIndex);
    }

    UpdateStats();
}

void FMetalBindlessDescriptorManager::BindHeaps(FMetalCommandContext& Context, EShaderVisibility::Type Stage, uint8 ResourceSlot, uint8 SamplerSlot)
{
    if (!bEnabled)
    {
        return;
    }

    if (Stage == EShaderVisibility::Compute)
    {
        id<MTLComputeCommandEncoder> Encoder = Context.GetComputeEncoder();
        if (!Encoder)
        {
            return;
        }

        if (ResourceSlot != FMetalPipelineBindingLayout::InvalidSlot && ResourceHeap.Buffer)
        {
            Context.DeclareResident(ResourceHeap.Buffer, true, false);
            [Encoder setBuffer:ResourceHeap.Buffer offset:0 atIndex:ResourceSlot];
        }

        if (SamplerSlot != FMetalPipelineBindingLayout::InvalidSlot && SamplerHeap.Buffer)
        {
            Context.DeclareResident(SamplerHeap.Buffer, true, false);
            [Encoder setBuffer:SamplerHeap.Buffer offset:0 atIndex:SamplerSlot];
        }

        return;
    }

    if (Context.GetGraphicsEncoder() == nil)
    {
        return;
    }

    if (ResourceSlot != FMetalPipelineBindingLayout::InvalidSlot && ResourceHeap.Buffer)
    {
        Context.DeclareResident(ResourceHeap.Buffer, true, false);
        Context.SetGraphicsBuffer(Stage, ResourceHeap.Buffer, 0, ResourceSlot);
    }

    if (SamplerSlot != FMetalPipelineBindingLayout::InvalidSlot && SamplerHeap.Buffer)
    {
        Context.DeclareResident(SamplerHeap.Buffer, true, false);
        Context.SetGraphicsBuffer(Stage, SamplerHeap.Buffer, 0, SamplerSlot);
    }
}

void FMetalBindlessDescriptorManager::DeclareResidency(FMetalCommandContext& Context)
{
    if (!bEnabled)
    {
        return;
    }

    auto DeclareHeap = [&](FHeap& Heap)
    {
        TScopedLock Lock(AllocCS);
        for (uint32 SlotIndex : Heap.Occupied)
        {
            const FSlotState& Slot = Heap.Slots[static_cast<int32>(SlotIndex)];
            if (Slot.Resource)
            {
                Context.DeclareResident(Slot.Resource, !Slot.bWritable, Slot.bIsView);
            }
        }
    };

    DeclareHeap(ResourceHeap);
    DeclareHeap(SamplerHeap);
}

uint64 FMetalBindlessDescriptorManager::ReadResourceEntry(uint32 Index) const
{
    if (!ResourceHeap.Mapped || Index >= ResourceHeap.Capacity)
    {
        return 0;
    }

    return ResourceHeap.Mapped[Index].Resource;
}

uint64 FMetalBindlessDescriptorManager::ReadSamplerEntry(uint32 Index) const
{
    if (!SamplerHeap.Mapped || Index >= SamplerHeap.Capacity)
    {
        return 0;
    }

    return SamplerHeap.Mapped[Index].Resource;
}

void FMetalBindlessDescriptorManager::UpdateStats()
{
#if METAL_ENABLE_STATS
    STAT_SET(STAT_Metal_BindlessResourceSlots, ResourceHeap.Occupied.Size());
    STAT_SET(STAT_Metal_BindlessSamplerSlots, SamplerHeap.Occupied.Size());
    STAT_SET(STAT_Metal_BindlessTableBytes,
        static_cast<int64>(ResourceHeap.Capacity + SamplerHeap.Capacity) * static_cast<int64>(sizeof(FMetalBindlessDescriptorEntry)));
    STAT_SET(STAT_Metal_BindlessPendingWrites, PendingWrites.Size());
    STAT_SET(STAT_Metal_BindlessGrowthCount, ResourceHeap.GrowthCount + SamplerHeap.GrowthCount);
#endif
}
