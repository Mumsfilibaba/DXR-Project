#pragma once
#include "D3D12Resource.h"

#include "RHI/RHIResources.h"

typedef TSharedRef<class FD3D12TimestampQuery> FD3D12TimestampQueryRef;

class FD3D12TimestampQuery 
    : public FRHITimestampQuery
    , public FD3D12DeviceChild
{
public:
    FD3D12TimestampQuery(FD3D12Device* InDevice);
    ~FD3D12TimestampQuery() = default;

    static FD3D12TimestampQuery* Create(FD3D12Device* InDevice);

    virtual void GetTimestampFromIndex(FRHITimestamp& OutQuery, uint32 Index) const override final;

    virtual uint64 GetFrequency() const override final
    {
        return static_cast<uint64>(Frequency);
    }

    void BeginQuery(ID3D12GraphicsCommandList* CmdList, uint32 Index);
    void EndQuery(ID3D12GraphicsCommandList* CmdList, uint32 Index);

    void ResolveQueries(class FD3D12CommandContext& CmdContext);

    FORCEINLINE ID3D12QueryHeap* GetQueryHeap() const
    {
        return QueryHeap.Get();
    }

private:
    // TODO: The download resource should be allocated in the context
    bool AllocateReadResource();

    TComPtr<ID3D12QueryHeap> QueryHeap;
    FD3D12ResourceRef      WriteResource;

    TArray<FD3D12ResourceRef> ReadResources;
    TArray<FRHITimestamp> TimeQueries;

    UINT64 Frequency;
};