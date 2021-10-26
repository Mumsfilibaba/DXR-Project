#pragma once
#include "RHI/GPUProfiler.h"

#include "D3D12Resource.h"

class CD3D12GPUProfiler : public CGPUProfiler, public CD3D12DeviceChild
{
public:
    CD3D12GPUProfiler( CD3D12Device* InDevice );
    ~CD3D12GPUProfiler() = default;

    virtual void GetTimeQuery( STimeQuery& OutQuery, uint32 Index ) const override final;

    virtual uint64 GetFrequency() const override final
    {
        return (uint64)Frequency;
    }

    void BeginQuery( ID3D12GraphicsCommandList* CmdList, uint32 Index );
    void EndQuery( ID3D12GraphicsCommandList* CmdList, uint32 Index );

    void ResolveQueries( class CD3D12RHICommandContext& CmdContext );

    FORCEINLINE ID3D12QueryHeap* GetQueryHeap() const
    {
        return QueryHeap.Get();
    }

    static CD3D12GPUProfiler* Create( CD3D12Device* InDevice );

private:
    bool AllocateReadResource();

    TComPtr<ID3D12QueryHeap> QueryHeap;
    TSharedRef<CD3D12Resource>      WriteResource;

    TArray<TSharedRef<CD3D12Resource>> ReadResources;
    TArray<STimeQuery> TimeQueries;

    UINT64 Frequency;
};