#pragma once
#include "RenderLayer/GPUProfiler.h"

#include "D3D12Resource.h"

class D3D12GPUProfiler : public GPUProfiler, public D3D12DeviceChild
{
public:
    D3D12GPUProfiler(D3D12Device* InDevice);
    ~D3D12GPUProfiler() = default;

    virtual void GetTimeQuery(TimeQuery& OutQuery, UInt32 Index) const override final;

    virtual UInt64 GetFrequency() const override final
    {
        return (UInt64)Frequency;
    }

    void BeginQuery(ID3D12GraphicsCommandList* CmdList, UInt32 Index);
    void EndQuery(ID3D12GraphicsCommandList* CmdList, UInt32 Index);

    void ResolveQueries(class D3D12CommandContext& CmdContext);

    ID3D12QueryHeap* GetQueryHeap() const { return QueryHeap.Get(); }

    static D3D12GPUProfiler* Create(D3D12Device* InDevice, class D3D12CommandContext& CmdContext);

private:
    Bool AllocateReadResource();

    TComPtr<ID3D12QueryHeap> QueryHeap;
    TRef<D3D12Resource>      WriteResource;
    
    TArray<TRef<D3D12Resource>> ReadResources;
    TArray<TimeQuery> TimeQueries;
    
    UINT64 Frequency;
};