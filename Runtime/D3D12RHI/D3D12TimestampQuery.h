#pragma once
#include "RHI/RHITimestampQuery.h"

#include "D3D12Resource.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHITimestampQuery

class FD3D12TimestampQuery : public FRHITimestampQuery, public FD3D12DeviceChild
{
public:

    FD3D12TimestampQuery(FD3D12Device* InDevice);
    ~FD3D12TimestampQuery() = default;

    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const override final;

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

    static FD3D12TimestampQuery* Create(FD3D12Device* InDevice);

private:

    // TODO: The download resource should be allocated in the context
    bool AllocateReadResource();

    TComPtr<ID3D12QueryHeap> QueryHeap;
    TSharedRef<FD3D12Resource>      WriteResource;

    TArray<TSharedRef<FD3D12Resource>> ReadResources;
    TArray<SRHITimestamp> TimeQueries;

    UINT64 Frequency;
};