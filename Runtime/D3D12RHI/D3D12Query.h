#pragma once
#include "Core/Containers/Queue.h"
#include "Core/Platform/CriticalSection.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12Resource.h"
#include "D3D12RHI/D3D12Fence.h"

#define D3D12_INVALID_QUERY_INDEX (-1)

class FD3D12Queue;
class FD3D12QueryHeap;
class FD3D12QueryHeapManager;
class FD3D12QueryAllocator;

typedef TSharedRef<struct FD3D12QueryRHI> FD3D12QueryRHIRef;

enum class ED3D12QueryType : uint8
{
    CommandListBegin,
    CommandListEnd,
    Timestamp,
    Occlusion,
    PipelineStatistics,
};

#if D3D12_SUPPORT_PIPELINE_STATISTICS1
extern D3D12RHI_API D3D12_MESH_SHADER_TIER GD3D12MeshShaderTier;
#endif

#if D3D12_SUPPORT_PIPELINE_STATISTICS1
NODISCARD FORCEINLINE bool SupportsPipelineStatistics1()
{
    return GD3D12MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
}
#endif

NODISCARD FORCEINLINE D3D12_QUERY_HEAP_TYPE GetPipelineStatsHeapType()
{
#if D3D12_SUPPORT_PIPELINE_STATISTICS1
    if (SupportsPipelineStatistics1())
    {
        return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1;
    }
#endif

    return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
}

NODISCARD FORCEINLINE D3D12_QUERY_TYPE GetPipelineStatsQueryType()
{
#if D3D12_SUPPORT_PIPELINE_STATISTICS1
    if (SupportsPipelineStatistics1())
    {
        return D3D12_QUERY_TYPE_PIPELINE_STATISTICS1;
    }
#endif

    return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
}

struct FD3D12Query
{
    FD3D12Query() = default;

    FD3D12Query(FD3D12QueryHeap* InHeap, int32 InQueryIndex, uint64* InResultTarget, ED3D12QueryType InType)
        : QueryHeap(InHeap)
        , ResultTarget(InResultTarget)
        , Type(InType)
        , QueryIndex(InQueryIndex)
    {
    }

    void CopyResult(void* Dst) const;

    bool IsValid() const
    {
        return QueryHeap != nullptr && QueryIndex != D3D12_INVALID_QUERY_INDEX;
    }

    operator bool() const
    {
        return IsValid();
    }

    FD3D12QueryHeap* QueryHeap    = nullptr;
    uint64*          ResultTarget = nullptr;
    ED3D12QueryType  Type         = ED3D12QueryType::Timestamp;
    int32            QueryIndex   = D3D12_INVALID_QUERY_INDEX;
};

struct FD3D12QueryRHI : public FRHIQuery, public FD3D12DeviceChild
{
    FD3D12QueryRHI(FD3D12Device* InDevice, EQueryType InQueryType);
    virtual ~FD3D12QueryRHI();

    FD3D12Query          CurrentQuery;
    FD3D12FenceSyncPoint SyncPoint;
    uint64*              QueryResult;
};

struct FD3D12QueryRange
{
    FD3D12QueryRange() = default;

    FD3D12QueryRange(FD3D12QueryHeap* InHeap, int32 InStartIndex, int32 InCount)
        : Heap(InHeap)
        , StartIndex(InStartIndex)
        , Count(InCount)
    {
    }

    bool IsValid() const
    {
        return Heap != nullptr;
    }

    operator bool() const
    {
        return IsValid();
    }

    FD3D12QueryHeap* Heap       = nullptr;
    int32            StartIndex = 0;
    int32            Count      = 0;
};

class FD3D12QueryHeap : public FD3D12DeviceChild
{
public:
    FD3D12QueryHeap(FD3D12Device* InDevice, D3D12_QUERY_HEAP_TYPE InHeapType, int32 InNumQueries);
    ~FD3D12QueryHeap();

    bool Initialize();
    void SetDebugName(const String& InName);

    uint64 GetQuerySize() const
    {
        switch (QueryHeapType)
        {
            case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:            return sizeof(uint64);
            case D3D12_QUERY_HEAP_TYPE_OCCLUSION:            return sizeof(uint64);
            case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:  return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
        #if D3D12_SUPPORT_PIPELINE_STATISTICS1
            case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1: return sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS1);
        #endif

            default:
                return sizeof(uint64);
        }
    }

    FD3D12ResidencyHandle* GetResidencyHandle()
    {
        return &ResidencyHandle;
    }

    FD3D12Resource*  GetReadbackResource() const { return ReadbackResource.Get(); }
    uint64*          GetReadbackData()     const { return ReadbackData; }
    ID3D12QueryHeap* GetD3D12QueryHeap()   const { return QueryHeap.Get(); }

    const D3D12_QUERY_TYPE      QueryType;
    const D3D12_QUERY_HEAP_TYPE QueryHeapType;
    const int32                 NumQueries;

private:
    TComPtr<ID3D12QueryHeap> QueryHeap;
    FD3D12ResourceRef        ReadbackResource;
    FD3D12ResidencyHandle    ResidencyHandle;
    uint64*                  ReadbackData;
};

class FD3D12QueryAllocator : public FD3D12DeviceChild
{
public:
    FD3D12QueryAllocator(FD3D12Device* InDevice, D3D12_QUERY_HEAP_TYPE InHeapType);
    ~FD3D12QueryAllocator();

    bool Allocate(FD3D12Query& OutQuery, uint64* ResultTarget, ED3D12QueryType InType);
    void Reset(TArray<FD3D12QueryRange>& OutRanges);

private:
    TArray<FD3D12QueryRange> Ranges;
    D3D12_QUERY_HEAP_TYPE    HeapType;
};

class FD3D12QueryHeapManager : public FD3D12DeviceChild
{
public:
    FD3D12QueryHeapManager(FD3D12Device* InDevice, D3D12_QUERY_HEAP_TYPE InHeapType, int32 InQueriesPerHeap);
    ~FD3D12QueryHeapManager();

    FD3D12QueryHeap* ObtainHeap();
    void RecycleHeap(FD3D12QueryHeap* Heap);

private:
    D3D12_QUERY_HEAP_TYPE    HeapType;
    int32                    QueriesPerHeap;
    TQueue<FD3D12QueryHeap*> AvailableHeaps;
    TArray<FD3D12QueryHeap*> AllHeaps;
    FCriticalSection         HeapsCS;
};
