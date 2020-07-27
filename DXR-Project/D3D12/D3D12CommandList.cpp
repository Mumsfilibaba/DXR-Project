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

bool D3D12CommandList::Initialize(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline)
{
	HRESULT hResult = Device->GetDevice()->CreateCommandList(0, Type, Allocator->GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CommandList));
	if (SUCCEEDED(hResult))
	{
		CommandList->Close();
		LOG_INFO("[D3D12CommandList]: Created CommandList");

		if (FAILED(CommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
		{
			LOG_ERROR("[D3D12CommandList]: FAILED to retrive DXR-CommandList");
			return false;
		}

		return true;
	}
	else
	{
		LOG_ERROR("[D3D12CommandList]: FAILED to create CommandList");
		return false;
	}
}

void D3D12CommandList::SetName(const std::string& Name)
{
	CommandList->SetName(ConvertToWide(Name).c_str());
}
