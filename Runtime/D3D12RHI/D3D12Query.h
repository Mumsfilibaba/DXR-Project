#pragma once
#include "D3D12Resource.h"
#include "RHI/RHIResources.h"

typedef TSharedRef<class FD3D12Query> FD3D12QueryRef;

class FD3D12Query : public FRHIQuery, public FD3D12DeviceChild
{
public:
    FD3D12Query(FD3D12Device* InDevice);
    virtual ~FD3D12Query() = default;

    bool Initialize();

    virtual void GetTimestampFromIndex(FTimingQuery& OutQuery, uint32 Index) const override final;

    virtual uint64 GetFrequency() const override final
    {
        return Frequency;
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

    TComPtr<ID3D12QueryHeap>  QueryHeap;
    FD3D12ResourceRef         WriteResource;

    TArray<FD3D12ResourceRef> ReadResources;
    TArray<FTimingQuery>     TimeQueries;

    UINT64 Frequency;
};