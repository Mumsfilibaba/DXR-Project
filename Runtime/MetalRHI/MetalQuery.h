#pragma once
#include "Core/Containers/Array.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "RHI/RHIResources.h"

class FMetalDevice;
class FMetalQueue;

static constexpr uint32 MetalInvalidQueryIndex = UINT32_MAX;
static constexpr uint32 MetalQuerySlotCount    = 4096;

struct FMetalQueryRHI : public FRHIQuery, public FMetalDeviceChild
{
    static void ResolveQueries(TArray<FMetalQueryRHI*>& Queries);

    FMetalQueryRHI(FMetalDevice* InDevice, EQueryType InQueryType);
    virtual ~FMetalQueryRHI();

    void Resolve();

    uint64*      QueryResult;
    uint64       SubmissionValue;
    FMetalQueue* SubmittedQueue;
    uint32       SampleIndex;
    bool         bResolved;
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
