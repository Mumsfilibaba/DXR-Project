#include "MetalRHI/MetalQuery.h"
#include "MetalRHI/MetalCapabilities.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Memory/Memory.h"

FMetalQueryRHI::FMetalQueryRHI(FMetalDevice* InDevice, EQueryType InQueryType)
    : FRHIQuery(InQueryType)
    , FMetalDeviceChild(InDevice)
    , QueryResult(static_cast<uint64*>(Memory::Malloc(GetQueryResultElementCount(InQueryType) * sizeof(uint64))))
    , SubmissionValue(0)
    , SampleIndex(MetalInvalidQueryIndex)
    , bResolved(false)
{
    Memory::Memzero(QueryResult, GetQueryResultElementCount(InQueryType) * sizeof(uint64));
}

FMetalQueryRHI::~FMetalQueryRHI()
{
    Memory::Free(QueryResult);
    QueryResult = nullptr;
}

void FMetalQueryRHI::Resolve()
{
    if (bResolved)
    {
        return;
    }

    switch (GetType())
    {
        case EQueryType::Timestamp:
        {
            GetDevice()->GetTimestampQueries().Resolve(*this);
            break;
        }

        case EQueryType::Occlusion:
        {
            GetDevice()->GetOcclusionQueries().Resolve(*this);
            break;
        }

        default:
        {
            break;
        }
    }
}

static uint32 AllocateRingSlot(TArray<uint8>& Occupied, uint32& NextSlot, FMetalDevice* Device)
{
    const uint32 SlotCount = static_cast<uint32>(Occupied.Size());
    for (uint32 Attempt = 0; Attempt < 2; ++Attempt)
    {
        for (uint32 Offset = 0; Offset < SlotCount; ++Offset)
        {
            const uint32 Index = (NextSlot + Offset) % SlotCount;
            if (Occupied[Index] == 0)
            {
                Occupied[Index] = 1;
                NextSlot = (Index + 1) % SlotCount;
                return Index;
            }
        }

        if (Attempt == 0 && Device)
        {
            Device->WaitForGPU();
        }
    }

    return MetalInvalidQueryIndex;
}

FMetalTimestampQueries::FMetalTimestampQueries(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , SampleBuffer(nil)
    , ResolveBuffer(nil)
    , DummyRenderTarget(nil)
    , Occupied()
    , NextSlot(0)
    , bCanSampleGraphics(false)
    , bCanSampleCompute(false)
    , bCanSampleBlit(false)
    , bUseSampleBarrier(false)
{
}

FMetalTimestampQueries::~FMetalTimestampQueries()
{
    Release();
}

bool FMetalTimestampQueries::Initialize()
{
    Release();

    SCOPED_AUTORELEASE_POOL();

    id<MTLDevice> DeviceHandle = GetDevice()->GetMTLDevice();
    id<MTLCounterSet> TimestampSet = nil;
    for (id<MTLCounterSet> CounterSet in DeviceHandle.counterSets)
    {
        if ([CounterSet.name isEqualToString:MTLCommonCounterSetTimestamp])
        {
            TimestampSet = CounterSet;
            break;
        }
    }

    if (!TimestampSet)
    {
        return false;
    }

    MTLCounterSampleBufferDescriptor* Descriptor = [MTLCounterSampleBufferDescriptor new];
    Descriptor.counterSet  = TimestampSet;
    Descriptor.sampleCount = MetalQuerySlotCount;
    Descriptor.storageMode = MTLStorageModePrivate;

    NSError* Error = nil;
    SampleBuffer = [DeviceHandle newCounterSampleBufferWithDescriptor:Descriptor error:&Error];
    [Descriptor release];

    if (!SampleBuffer)
    {
        const String ErrorString(Error ? [Error localizedDescription] : @"unknown error");
        METAL_ERROR("Failed to create the timestamp counter sample buffer: %s", *ErrorString);
        return false;
    }

    const uint64 ResolveBytes = uint64(MetalQuerySlotCount) * sizeof(MTLCounterResultTimestamp);
    ResolveBuffer = [DeviceHandle newBufferWithLength:ResolveBytes options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache];
    if (!ResolveBuffer)
    {
        METAL_ERROR("Failed to create the timestamp resolve buffer");
        Release();
        return false;
    }

    Memory::Memzero(ResolveBuffer.contents, ResolveBytes);
    Occupied.Resize(static_cast<int32>(MetalQuerySlotCount));
    Memory::Memzero(Occupied.Data(), Occupied.Size());
    NextSlot = 0;

    id<MTLDevice> DeviceHandleForCaps = GetDevice()->GetMTLDevice();
    bUseSampleBarrier  = [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtStageBoundary];
    bCanSampleGraphics = bUseSampleBarrier || [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtDrawBoundary];
    bCanSampleCompute  = bUseSampleBarrier || [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary];
    bCanSampleBlit     = bUseSampleBarrier || [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtBlitBoundary];

    GMetalSupportsTimestampStageBoundary    = bUseSampleBarrier;
    GMetalSupportsTimestampDrawBoundary     = [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtDrawBoundary];
    GMetalSupportsTimestampDispatchBoundary = [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtDispatchBoundary];
    GMetalSupportsTimestampBlitBoundary     = [DeviceHandleForCaps supportsCounterSampling:MTLCounterSamplingPointAtBlitBoundary];

    if (!bCanSampleGraphics && !bCanSampleCompute && !bCanSampleBlit)
    {
        METAL_ERROR("Timestamp counter set exists but no sampling point is usable");
        Release();
        return false;
    }

    if (bCanSampleGraphics && !bCanSampleBlit && !bCanSampleCompute)
    {
        MTLTextureDescriptor* DummyDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm width:1 height:1 mipmapped:NO];
        DummyDesc.usage       = MTLTextureUsageRenderTarget;
        DummyDesc.storageMode = MTLStorageModePrivate;
        DummyRenderTarget     = [DeviceHandle newTextureWithDescriptor:DummyDesc];
        if (!DummyRenderTarget)
        {
            METAL_ERROR("Failed to create a dummy render target for timestamp sampling");
            Release();
            return false;
        }
    }

    STAT_ADD(STAT_Metal_CounterSampleBufferCount, 1);
    return true;
}

void FMetalTimestampQueries::Release()
{
    if (SampleBuffer)
    {
        STAT_SUBTRACT(STAT_Metal_CounterSampleBufferCount, 1);
        [SampleBuffer release];
        SampleBuffer = nil;
    }

    [ResolveBuffer release];
    ResolveBuffer = nil;

    [DummyRenderTarget release];
    DummyRenderTarget = nil;

    Occupied.Clear();
    NextSlot           = 0;
    bCanSampleGraphics = false;
    bCanSampleCompute  = false;
    bCanSampleBlit     = false;
    bUseSampleBarrier  = false;
}

bool FMetalTimestampQueries::Allocate(FMetalQueryRHI& Query)
{
    if (!SampleBuffer)
    {
        return false;
    }

    const uint32 Index = AllocateRingSlot(Occupied, NextSlot, GetDevice());
    if (Index == MetalInvalidQueryIndex)
    {
        METAL_ERROR("Failed to allocate a timestamp query slot");
        return false;
    }

    Query.SampleIndex     = Index;
    Query.SubmissionValue = 0;
    Query.bResolved       = false;
    *Query.QueryResult    = 0;
    STAT_ADD(STAT_Metal_TimestampSlotsInFlight, 1);
    return true;
}

void FMetalTimestampQueries::Cancel(FMetalQueryRHI& Query)
{
    FreeSlot(Query.SampleIndex);
    Query.SampleIndex     = MetalInvalidQueryIndex;
    Query.SubmissionValue = 0;
}

void FMetalTimestampQueries::EncodeResolve(id<MTLCommandBuffer> CommandBuffer, TArray<FMetalQueryRHI*>& Queries)
{
    if (!SampleBuffer || !ResolveBuffer || !CommandBuffer)
    {
        return;
    }

    bool bHasTimestamp = false;
    for (FMetalQueryRHI* Query : Queries)
    {
        if (Query && Query->GetType() == EQueryType::Timestamp && Query->SampleIndex != MetalInvalidQueryIndex)
        {
            bHasTimestamp = true;
            break;
        }
    }

    if (!bHasTimestamp)
    {
        return;
    }

    id<MTLBlitCommandEncoder> Blit = [CommandBuffer blitCommandEncoder];
    if (!Blit)
    {
        METAL_ERROR("Failed to create a blit encoder to resolve timestamps");
        return;
    }

    for (FMetalQueryRHI* Query : Queries)
    {
        if (!Query || Query->GetType() != EQueryType::Timestamp || Query->SampleIndex == MetalInvalidQueryIndex)
        {
            continue;
        }

        const NSUInteger Offset = NSUInteger(Query->SampleIndex) * sizeof(MTLCounterResultTimestamp);
        [Blit resolveCounters:SampleBuffer
                      inRange:NSMakeRange(Query->SampleIndex, 1)
            destinationBuffer:ResolveBuffer
          destinationOffset:Offset];
    }

    [Blit endEncoding];
}

void FMetalTimestampQueries::Resolve(FMetalQueryRHI& Query)
{
    if (Query.SampleIndex == MetalInvalidQueryIndex)
    {
        return;
    }

    uint64 Timestamp = 0;
    if (ResolveBuffer)
    {
        const MTLCounterResultTimestamp* Results = static_cast<const MTLCounterResultTimestamp*>(ResolveBuffer.contents);
        Timestamp = Results[Query.SampleIndex].timestamp;
    }

    if (Timestamp == 0 && SampleBuffer)
    {
        NSData* Data = [SampleBuffer resolveCounterRange:NSMakeRange(Query.SampleIndex, 1)];
        if (Data && Data.length >= sizeof(MTLCounterResultTimestamp))
        {
            const MTLCounterResultTimestamp* Result = static_cast<const MTLCounterResultTimestamp*>(Data.bytes);
            Timestamp = Result->timestamp;
        }
    }

    *Query.QueryResult = Timestamp;
    Query.bResolved    = true;

    FreeSlot(Query.SampleIndex);
    Query.SampleIndex = MetalInvalidQueryIndex;
}

void FMetalTimestampQueries::FreeSlot(uint32 Index)
{
    if (Index < static_cast<uint32>(Occupied.Size()) && Occupied[Index])
    {
        Occupied[Index] = 0;
        STAT_SUBTRACT(STAT_Metal_TimestampSlotsInFlight, 1);
    }
}

FMetalOcclusionQueries::FMetalOcclusionQueries(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , VisibilityBuffer(nil)
    , Occupied()
    , NextSlot(0)
{
}

FMetalOcclusionQueries::~FMetalOcclusionQueries()
{
    Release();
}

bool FMetalOcclusionQueries::Initialize()
{
    Release();

    const uint64 ByteSize = uint64(MetalQuerySlotCount) * sizeof(uint64);
    VisibilityBuffer = [GetDevice()->GetMTLDevice() newBufferWithLength:ByteSize options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache];
    if (!VisibilityBuffer)
    {
        METAL_ERROR("Failed to create the occlusion visibility result buffer");
        return false;
    }

    Memory::Memzero(VisibilityBuffer.contents, ByteSize);
    Occupied.Resize(static_cast<int32>(MetalQuerySlotCount));
    Memory::Memzero(Occupied.Data(), Occupied.Size());
    NextSlot = 0;
    return true;
}

void FMetalOcclusionQueries::Release()
{
    [VisibilityBuffer release];
    VisibilityBuffer = nil;
    Occupied.Clear();
    NextSlot = 0;
}

bool FMetalOcclusionQueries::Allocate(FMetalQueryRHI& Query)
{
    if (!VisibilityBuffer)
    {
        return false;
    }

    const uint32 Index = AllocateRingSlot(Occupied, NextSlot, GetDevice());
    if (Index == MetalInvalidQueryIndex)
    {
        METAL_ERROR("Failed to allocate an occlusion query slot");
        return false;
    }

    uint64* Slots = static_cast<uint64*>(VisibilityBuffer.contents);
    Slots[Index] = 0;

    Query.SampleIndex     = Index;
    Query.SubmissionValue = 0;
    Query.bResolved       = false;
    *Query.QueryResult    = 0;
    STAT_ADD(STAT_Metal_OcclusionSlotsInFlight, 1);
    return true;
}

void FMetalOcclusionQueries::Cancel(FMetalQueryRHI& Query)
{
    FreeSlot(Query.SampleIndex);
    Query.SampleIndex     = MetalInvalidQueryIndex;
    Query.SubmissionValue = 0;
}

void FMetalOcclusionQueries::Resolve(FMetalQueryRHI& Query)
{
    if (Query.SampleIndex == MetalInvalidQueryIndex || !VisibilityBuffer)
    {
        return;
    }

    const uint64* Slots = static_cast<const uint64*>(VisibilityBuffer.contents);
    *Query.QueryResult  = Slots[Query.SampleIndex];
    Query.bResolved     = true;

    FreeSlot(Query.SampleIndex);
    Query.SampleIndex = MetalInvalidQueryIndex;
}

void FMetalOcclusionQueries::FreeSlot(uint32 Index)
{
    if (Index < static_cast<uint32>(Occupied.Size()) && Occupied[Index])
    {
        Occupied[Index] = 0;
        STAT_SUBTRACT(STAT_Metal_OcclusionSlotsInFlight, 1);
    }
}

void ResolveMetalQueries(TArray<FMetalQueryRHI*>& Queries)
{
    for (FMetalQueryRHI* Query : Queries)
    {
        if (Query)
        {
            Query->Resolve();
        }
    }

    Queries.Clear();
}
