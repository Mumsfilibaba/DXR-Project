#pragma once
#include "RenderingCore/SamplerState.h"

#include "D3D12DescriptorHeap.h"
#include "D3D12Device.h"

/*
* D3D12SamplerState
*/

class D3D12SamplerState : public SamplerState, public D3D12DeviceChild
{
public:
	inline D3D12SamplerState(D3D12Device* InDevice, D3D12OfflineDescriptorHeap* InOfflineHeap, const D3D12_SAMPLER_DESC& InDesc)
		: D3D12DeviceChild(InDevice)
		, SamplerState()
		, OfflineHeap(InOfflineHeap)
		, OfflineHandle({0})
		, Desc()
	{
		VALIDATE(InOfflineHeap !=  nullptr);
		OfflineHandle = OfflineHeap->Allocate(OfflineHeapIndex);

		CreateView(Desc);
	}

	FORCEINLINE void CreateView(const D3D12_SAMPLER_DESC& InDesc)
	{
		Desc = InDesc;
		Device->CreateSampler(&Desc, OfflineHandle);
	}

	FORCEINLINE D3D12_CPU_DESCRIPTOR_HANDLE GetOfflineHandle() const
	{
		return OfflineHandle;
	}

	FORCEINLINE const D3D12_SAMPLER_DESC& GetDesc() const
	{
		return Desc;
	}

private:
	D3D12OfflineDescriptorHeap* OfflineHeap = nullptr;
	UInt32						OfflineHeapIndex = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle;
	D3D12_SAMPLER_DESC Desc;
};