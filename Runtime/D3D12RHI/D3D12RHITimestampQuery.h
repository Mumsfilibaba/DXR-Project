#pragma once
#include "RHI/RHITimestampQuery.h"

#include "D3D12Resource.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12RHITimestampQuery

class CD3D12RHITimestampQuery : public CRHITimestampQuery, public CD3D12DeviceObject
{
public:

    CD3D12RHITimestampQuery(CD3D12Device* InDevice);
    ~CD3D12RHITimestampQuery() = default;

    virtual void GetTimestampFromIndex(SRHITimestamp& OutQuery, uint32 Index) const override final;

    virtual uint64 GetFrequency() const override final
    {
        return static_cast<uint64>(Frequency);
    }

    void BeginQuery(ID3D12GraphicsCommandList* CmdList, uint32 Index);
    void EndQuery(ID3D12GraphicsCommandList* CmdList, uint32 Index);

    void ResolveQueries(class CD3D12CommandContext& CmdContext);

    FORCEINLINE ID3D12QueryHeap* GetQueryHeap() const
    {
        return QueryHeap.Get();
    }

    static CD3D12RHITimestampQuery* Create(CD3D12Device* InDevice);

private:

    // TODO: The download resource should be allocated in the context
    bool AllocateReadResource();

    TComPtr<ID3D12QueryHeap> QueryHeap;
    TSharedRef<CD3D12Resource>      WriteResource;

    TArray<TSharedRef<CD3D12Resource>> ReadResources;
    TArray<SRHITimestamp> TimeQueries;

    UINT64 Frequency;
};