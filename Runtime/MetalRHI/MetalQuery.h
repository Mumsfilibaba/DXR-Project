#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "RHI/RHIResources.h"

class FMetalDevice;
class FMetalQueue;

static constexpr uint32 MetalInvalidQueryIndex = UINT32_MAX;
static constexpr uint32 MetalQuerySlotCount    = 4096;
static constexpr uint32 MetalCounterPairCount  = 2048;

struct FMetalQueryRHI : public FRHIQuery, public FMetalDeviceChild
{
    static void ResolveQueries(TArray<FMetalQueryRHI*>& Queries);

    FMetalQueryRHI(FMetalDevice* InDevice, EQueryType InQueryType);
    virtual ~FMetalQueryRHI();

    void Resolve();

    uint64*        QueryResult;
    uint64         SubmissionValue;
    FMetalQueue*   SubmittedQueue;
    uint32         SampleIndex;
    TArray<uint32> SamplePairs;
    bool           bResolved;
};

class FMetalCounterPairs : public FMetalDeviceChild
{
public:
    FMetalCounterPairs(FMetalDevice* InDevice);
    ~FMetalCounterPairs();

    bool Initialize(NSString* CounterSetName);
    void Release();

    uint32  AllocatePair();
    void    FreePair(uint32 Pair);
    NSData* ResolvePair(uint32 Pair) const;

    id<MTLCounterSampleBuffer> GetSampleBuffer() const
    {
        return SampleBuffer;
    }

    bool IsAvailable() const
    {
        return SampleBuffer != nil;
    }

private:
    id<MTLCounterSampleBuffer> SampleBuffer;
    TArray<uint8>              Occupied;
    uint32                     NextSlot;
    mutable FCriticalSection   SlotsCS;
};

class FMetalStatisticQueries : public FMetalDeviceChild
{
public:
    FMetalStatisticQueries(FMetalDevice* InDevice);
    ~FMetalStatisticQueries();

    bool Initialize();
    void Release();

    void   Begin(FMetalQueryRHI& Query);
    uint32 BeginPair(FMetalQueryRHI& Query);
    void   Cancel(FMetalQueryRHI& Query);
    void   Resolve(FMetalQueryRHI& Query);

    id<MTLCounterSampleBuffer> GetSampleBuffer() const
    {
        return Pairs.GetSampleBuffer();
    }

    bool IsAvailable() const
    {
        return Pairs.IsAvailable();
    }

private:
    void FreePairs(FMetalQueryRHI& Query);

    FMetalCounterPairs Pairs;
};

class FMetalStageUtilizationQueries : public FMetalDeviceChild
{
public:
    FMetalStageUtilizationQueries(FMetalDevice* InDevice);
    ~FMetalStageUtilizationQueries();

    bool Initialize();
    void Release();

    uint32 BeginPair();
    void   EndFrame(FMetalQueue& Queue);

    id<MTLCounterSampleBuffer> GetSampleBuffer() const
    {
        return Pairs.GetSampleBuffer();
    }

    bool IsAvailable() const
    {
        return Pairs.IsAvailable();
    }

private:
    struct FFrame
    {
        TArray<uint32> Pairs;
        uint64         ResolveValue = 0;
    };

    void ResolveFrame(const FFrame& Frame);
    void FreeFrame(FFrame& Frame);

    FMetalCounterPairs Pairs;
    FFrame             CurrentFrame;
    TArray<FFrame>     PendingFrames;
    FCriticalSection   FramesCS;
};

class FMetalTimestampQueries : public FMetalDeviceChild
{
public:
    FMetalTimestampQueries(FMetalDevice* InDevice);
    ~FMetalTimestampQueries();

    bool Initialize();
    void Release();

    bool Allocate(FMetalQueryRHI& Query);
    void Cancel(FMetalQueryRHI& Query);
    void EncodeResolve(id<MTLCommandBuffer> CommandBuffer, TArray<FMetalQueryRHI*>& Queries);
    void Resolve(FMetalQueryRHI& Query);

    id<MTLCounterSampleBuffer> GetSampleBuffer() const
    {
        return SampleBuffer;
    }

    bool IsAvailable() const
    {
        return SampleBuffer != nil;
    }

private:
    void FreeSlot(uint32 Index);

    id<MTLCounterSampleBuffer> SampleBuffer;
    id<MTLBuffer>              ResolveBuffer;
    TArray<uint8>              Occupied;
    uint32                     NextSlot;
};

class FMetalOcclusionQueries : public FMetalDeviceChild
{
public:
    FMetalOcclusionQueries(FMetalDevice* InDevice);
    ~FMetalOcclusionQueries();

    bool Initialize();
    void Release();

    bool Allocate(FMetalQueryRHI& Query);
    void Cancel(FMetalQueryRHI& Query);
    void Resolve(FMetalQueryRHI& Query);

    id<MTLBuffer> GetBuffer() const
    {
        return VisibilityBuffer;
    }

private:
    void FreeSlot(uint32 Index);

    id<MTLBuffer> VisibilityBuffer;
    TArray<uint8> Occupied;
    uint32        NextSlot;
};
