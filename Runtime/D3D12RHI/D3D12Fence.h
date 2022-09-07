#pragma once
#include "D3D12DeviceChild.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Fence

class FD3D12Fence 
    : public FD3D12DeviceChild
{
public:
    FD3D12Fence(FD3D12Device* InDevice);
    ~FD3D12Fence();

    bool Initialize(uint64 InitalValue);

    bool WaitForValue(uint64 Value);

    FORCEINLINE uint64 GetCompletedValue() const
    {
        return Fence->GetCompletedValue();
    }

    FORCEINLINE bool Signal(uint64 Value)
    {
        HRESULT Result = Fence->Signal(Value);
        return SUCCEEDED(Result);
    }

    FORCEINLINE void SetName(const FString& Name)
    {
        Fence->SetPrivateData(WKPDID_D3DDebugObjectName, Name.GetLength(), Name.GetCString());
    }

    FORCEINLINE ID3D12Fence* GetFence() const
    {
        return Fence.Get();
    }

private:
    TComPtr<ID3D12Fence> Fence;
    HANDLE               Event;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12FenceSyncPoint

struct FD3D12FenceSyncPoint
{
    FD3D12FenceSyncPoint()
        : Fence(nullptr)
        , FenceValue(0)
    { }

	FD3D12FenceSyncPoint(FD3D12Fence* InFence, uint64 InFenceValue)
		: Fence(InFence)
		, FenceValue(InFenceValue)
	{ }

    FORCEINLINE bool IsComplete() const
    {
        return (FenceValue <= Fence->GetCompletedValue());
    }

    FD3D12Fence* Fence;
    uint64       FenceValue;
};