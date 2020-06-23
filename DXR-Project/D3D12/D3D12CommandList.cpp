#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12CommandAllocator.h"

D3D12CommandList::D3D12CommandList(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, CommandList(nullptr)
	, DXRCommandList(nullptr)
	, DeferredResourceBarriers()
{
}

D3D12CommandList::~D3D12CommandList()
{
}

bool D3D12CommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* InAllocator, ID3D12PipelineState* InitalPipeline)
{
	HRESULT hResult = Device->GetDevice()->CreateCommandList(0, Type, InAllocator->GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CommandList));
	if (SUCCEEDED(hResult))
	{
		CommandList->Close();
		::OutputDebugString("[D3D12CommandList]: Created CommandList\n");

		if (FAILED(CommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
		{
			::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-CommandList\n");
			return false;
		}

		return true;
	}
	else
	{
		::OutputDebugString("[D3D12CommandList]: Failed to create CommandList\n");
		return false;
	}
}

void D3D12CommandList::SetName(const std::string& InName)
{
	CommandList->SetName(ConvertToWide(InName).c_str());
}
