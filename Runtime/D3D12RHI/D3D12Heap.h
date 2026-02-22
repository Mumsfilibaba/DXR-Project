#pragma once
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12RefCounted.h"
#include "D3D12RHI/D3D12ResidencyManager.h"

typedef TSharedRef<class FD3D12Heap> FD3D12HeapRef;

class FD3D12Heap : public FD3D12DeviceChild, public FD3D12RefCounted
{
public:
    FD3D12Heap(FD3D12Device* InDevice, ID3D12Heap* InHeap);
    ~FD3D12Heap();

    void SetDebugName(const FString& Name);

    void DeferredRelease();

    void StartResidencyTracking();
    void EndResidencyTracking();

    void DisableDeferredRelease() { bShouldDeferredRelease = false; }
    bool ShouldDeferredRelease() const { return bShouldDeferredRelease; }

    FORCEINLINE D3D12_HEAP_TYPE GetHeapType() const
    {
        return Desc.Properties.Type;
    }

    FORCEINLINE ID3D12Heap* GetD3D12Heap() const
    {
        return Heap.Get();
    }

    FORCEINLINE const D3D12_HEAP_DESC& GetDesc() const
    {
        return Desc;
    }

    FORCEINLINE uint64 GetSize() const
    {
        return Desc.SizeInBytes;
    }

    FD3D12ResidencyHandle* GetResidencyHandle() { return &ResidencyHandle; }

private:
    TComPtr<ID3D12Heap>   Heap;
    D3D12_HEAP_DESC       Desc;
    FD3D12ResidencyHandle ResidencyHandle;
    bool                  bShouldDeferredRelease;
};
