#include "Containers/TUniquePtr.h"

#include "D3D12RenderingAPI.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandAllocator.h"
#include "D3D12ComputePipelineState.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Fence.h"
#include "D3D12GraphicsPipelineState.h"
#include "D3D12ImmediateCommandList.h"
#include "D3D12RayTracingPipelineState.h"
#include "D3D12RayTracingScene.h"
#include "D3D12RootSignature.h"
#include "D3D12Views.h"
#include "D3D12SwapChain.h"

/*
* D3D12RenderingAPI
*/

D3D12RenderingAPI::D3D12RenderingAPI()
	: RenderingAPI()
	, Device(nullptr)
	, ImmediateCommandList(nullptr)
	, SwapChain(nullptr)
{
}

D3D12RenderingAPI::~D3D12RenderingAPI()
{
	SwapChain.Reset();
}

bool D3D12RenderingAPI::Initialize(TSharedRef<GenericWindow> InRenderWindow, bool EnableDebug)
{
	Device = MakeShared<D3D12Device>();
	if (!Device->Initialize(EnableDebug))
	{
		return false;
	}

	ImmediateCommandList = MakeShared<D3D12ImmediateCommandList>(Device.Get());
	if (!ImmediateCommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	Queue = MakeShared<D3D12CommandQueue>(Device.Get());
	if (!Queue->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	RenderWindow	= StaticCast<WindowsWindow>(InRenderWindow);
	SwapChain		= MakeShared<D3D12SwapChain>(Device.Get());
	if (!SwapChain->Initialize(RenderWindow.Get(), Queue.Get()))
	{
		return false;
	}

	return true;
}

D3D12Texture* D3D12RenderingAPI::CreateTexture(const TextureProperties& Properties) const
{
	TUniquePtr<D3D12Texture> Texture = TUniquePtr(new D3D12Texture(Device.Get()));
	if (Texture->Initialize(Properties))
	{
		return Texture.Release();
	}

	return nullptr;
}

D3D12Buffer* D3D12RenderingAPI::CreateBuffer(const BufferProperties& Properties) const
{
	TUniquePtr<D3D12Buffer> Buffer = TUniquePtr(new D3D12Buffer(Device.Get()));
	if (Buffer->Initialize(Properties))
	{
		return Buffer.Release();
	}

	return nullptr;
}

D3D12RayTracingScene* D3D12RenderingAPI::CreateRayTracingScene(class D3D12RayTracingPipelineState* PipelineState) const
{
	TUniquePtr<D3D12RayTracingScene> Scene = TUniquePtr(new D3D12RayTracingScene(Device.Get()));
	if (Scene->Initialize(PipelineState))
	{
		return Scene.Release();
	}

	return nullptr;
}

D3D12RayTracingGeometry* D3D12RenderingAPI::CreateRayTracingGeometry() const
{
	return new D3D12RayTracingGeometry(Device.Get());
}

D3D12DescriptorTable* D3D12RenderingAPI::CreateDescriptorTable(uint32 DescriptorCount) const
{
	return new D3D12DescriptorTable(Device.Get(), DescriptorCount);
}

D3D12ShaderResourceView* D3D12RenderingAPI::CreateShaderResourceView(ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* InDesc) const
{
	return new D3D12ShaderResourceView(Device.Get(), InResource, InDesc);
}

D3D12UnorderedAccessView* D3D12RenderingAPI::CreateUnorderedAccessView(ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* InDesc) const
{
	return new D3D12UnorderedAccessView(Device.Get(), InCounterResource, InResource, InDesc);
}

D3D12RenderTargetView* D3D12RenderingAPI::CreateRenderTargetView(ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC* InDesc) const
{
	return new D3D12RenderTargetView(Device.Get(), InResource, InDesc);
}

D3D12DepthStencilView* D3D12RenderingAPI::CreateDepthStencilView(ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* InDesc) const
{
	return new D3D12DepthStencilView(Device.Get(), InResource, InDesc);
}

D3D12ConstantBufferView* D3D12RenderingAPI::CreateConstantBufferView(ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC* InDesc) const
{
	return new D3D12ConstantBufferView(Device.Get(), InResource, InDesc);
}

D3D12Fence* D3D12RenderingAPI::CreateFence(uint64 InitalValue) const
{
	TUniquePtr<D3D12Fence> Fence = TUniquePtr(new D3D12Fence(Device.Get()));
	if (Fence->Initialize(InitalValue))
	{
		return Fence.Release();
	}

	return nullptr;
}

D3D12CommandAllocator* D3D12RenderingAPI::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE ListType) const
{
	TUniquePtr<D3D12CommandAllocator> Allocator = TUniquePtr(new D3D12CommandAllocator(Device.Get()));
	if (Allocator->Initialize(ListType))
	{
		return Allocator.Release();
	}

	return nullptr;
}

D3D12CommandList* D3D12RenderingAPI::CreateCommandList(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline) const
{
	TUniquePtr<D3D12CommandList> List = TUniquePtr(new D3D12CommandList(Device.Get()));
	if (List->Initialize(Type, Allocator, InitalPipeline))
	{
		return List.Release();
	}

	return nullptr;
}

D3D12CommandQueue* D3D12RenderingAPI::CreateCommandQueue() const
{
	return nullptr;
}

D3D12ComputePipelineState* D3D12RenderingAPI::CreateComputePipelineState(const ComputePipelineStateProperties& Properties) const
{
	TUniquePtr<D3D12ComputePipelineState> PipelineState = TUniquePtr(new D3D12ComputePipelineState(Device.Get()));
	if (PipelineState->Initialize(Properties))
	{
		return PipelineState.Release();
	}

	return nullptr;
}

D3D12GraphicsPipelineState* D3D12RenderingAPI::CreateGraphicsPipelineState(const GraphicsPipelineStateProperties& Properties) const
{
	TUniquePtr<D3D12GraphicsPipelineState> PipelineState = TUniquePtr(new D3D12GraphicsPipelineState(Device.Get()));
	if (PipelineState->Initialize(Properties))
	{
		return PipelineState.Release();
	}

	return nullptr;
}

D3D12RayTracingPipelineState* D3D12RenderingAPI::CreateRayTracingPipelineState(const RayTracingPipelineStateProperties& Properties) const
{
	TUniquePtr<D3D12RayTracingPipelineState> PipelineState = TUniquePtr(new D3D12RayTracingPipelineState(Device.Get()));
	if (PipelineState->Initialize(Properties))
	{
		return PipelineState.Release();
	}

	return nullptr;
}

D3D12RootSignature* D3D12RenderingAPI::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc) const
{
	TUniquePtr<D3D12RootSignature> RootSignature = TUniquePtr(new D3D12RootSignature(Device.Get()));
	if (RootSignature->Initialize(Desc))
	{
		return RootSignature.Release();
	}

	return nullptr;
}

D3D12RootSignature* D3D12RenderingAPI::CreateRootSignature(IDxcBlob* ShaderBlob) const
{
	TUniquePtr<D3D12RootSignature> RootSignature = TUniquePtr(new D3D12RootSignature(Device.Get()));
	if (RootSignature->Initialize(ShaderBlob))
	{
		return RootSignature.Release();
	}

	return nullptr;
}

D3D12CommandQueue* D3D12RenderingAPI::GetQueue() const
{
	return Queue.Get();
}

D3D12SwapChain* D3D12RenderingAPI::GetSwapChain() const
{
	return SwapChain.Get();
}

TSharedPtr<D3D12ImmediateCommandList> D3D12RenderingAPI::GetImmediateCommandList() const
{
	return ImmediateCommandList;
}

bool D3D12RenderingAPI::IsRayTracingSupported() const
{
	return Device->IsRayTracingSupported();
}

bool D3D12RenderingAPI::UAVSupportsFormat(DXGI_FORMAT Format) const
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
	Memory::Memzero(&FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

	HRESULT Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
	if (SUCCEEDED(Result))
	{
		if (FeatureData.TypedUAVLoadAdditionalFormats)
		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport =
			{
				Format,
				D3D12_FORMAT_SUPPORT1_NONE,
				D3D12_FORMAT_SUPPORT2_NONE
			};

			Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
			if (FAILED(Result) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
			{
				return false;
			}
		}
	}

	return true;
}
