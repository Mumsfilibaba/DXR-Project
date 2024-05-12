#pragma once
#include "D3D12Resource.h"
#include "D3D12CommandList.h"
#include "RHI/RHIResources.h"

#define D3D12_INVALID_QUERY_INDEX (-1)

class FD3D12QueryHeap;
class FD3D12QueryHeapManager;
class FD3D12CommandContext;
class FD3D12Queue;

typedef TSharedRef<FD3D12QueryHeap>   FD3D12QueryHeapRef;
typedef TSharedRef<struct FD3D12Query> FD3D12QueryRef;

struct FD3D12QueryAllocation
{
    FD3D12QueryAllocation()
        : QueryHeap(nullptr)
        , IndexInQueryPool(D3D12_INVALID_QUERY_INDEX)
        , Results(nullptr)
    {
    }

    FD3D12QueryAllocation(FD3D12QueryHeap* InQueryHeap, int32 InIndexInQueryPool, uint64* InResults)
        : QueryHeap(InQueryHeap)
        , IndexInQueryPool(InIndexInQueryPool)
        , Results(InResults)
    {
    }

    bool IsValid() const
    {
        return QueryHeap != nullptr && IndexInQueryPool != D3D12_INVALID_QUERY_INDEX;
    }

    FD3D12QueryHeap* QueryHeap;
    int32            IndexInQueryPool;
    uint64*          Results;
};

struct FD3D12Query : public FRHIQuery, public FD3D12DeviceChild
{
    FD3D12Query(FD3D12Device* InDevice, EQueryType InQueryType);
    virtual ~FD3D12Query() = default;

    // QueryAllocation should only used on RHI Thread
    FD3D12QueryAllocation QueryAllocation;
    uint64                Result;
};

class FD3D12QueryHeap : public FD3D12DeviceChild
{
public:
    FD3D12QueryHeap(FD3D12Device* InDevice, FD3D12QueryHeapManager* InQueryHeapManager);
    ~FD3D12QueryHeap() = default;

    bool Initialize(D3D12_QUERY_HEAP_TYPE InQueryHeapType);
    FD3D12QueryAllocation AllocateQueries(uint64* Results);
    void ResolveQueries(FD3D12CommandList& CommandList);
    void ReadBackResults(FD3D12Queue& Queue);
    void SetDebugName(const FString& InName);

    ID3D12QueryHeap* GetD3D12QueryHeap() const
    {
        return QueryHeap.Get();
    }

    FD3D12Resource* GetReadResource() const
    {
        return ReadResource.Get();
    }

    FD3D12QueryHeapManager* GetQueryHeapManager() const
    {
        return QueryHeapManager;
    }

    D3D12_QUERY_HEAP_TYPE GetQueryType() const
    {
        return QueryHeapType;
    }
    
private:
    FD3D12ResourceRef             ReadResource;
    TComPtr<ID3D12QueryHeap>      QueryHeap;
    FD3D12QueryHeapManager*       QueryHeapManager;
    TArray<FD3D12QueryAllocation> QueryAllocations;
    D3D12_QUERY_HEAP_TYPE         QueryHeapType;
    int32                         CurrentQueryIndex;
    int32                         NumQueries;
};

class FD3D12QueryAllocator : public FD3D12DeviceChild
{
public:
    FD3D12QueryAllocator(FD3D12Device* InDevice, FD3D12CommandContext& InContext, EQueryType InQueryType);
    ~FD3D12QueryAllocator();

    FD3D12QueryAllocation Allocate(uint64* InResults);
    void PrepareForNewCommanBuffer();

private:
    FD3D12CommandContext&   Context;
    FD3D12QueryHeap*        QueryHeap;
    EQueryType              QueryType;
    FD3D12QueryHeapManager* QueryHeapManager;
};

class FD3D12QueryHeapManager : public FD3D12DeviceChild
{
public:
    FD3D12QueryHeapManager(FD3D12Device* InDevice, EQueryType InQueryType);
    ~FD3D12QueryHeapManager();

    FD3D12QueryHeap* ObtainQueryHeap();
    void RecycleQueryHeap(FD3D12QueryHeap* InQueryPool);

    EQueryType GetQueryType() const 
    {
        return QueryType;
    }

private:
    EQueryType               QueryType;
    TQueue<FD3D12QueryHeap*> AvailableQueryHeaps;
    TArray<FD3D12QueryHeap*> QueryHeaps;
    FCriticalSection         QueryHeapsCS;
};