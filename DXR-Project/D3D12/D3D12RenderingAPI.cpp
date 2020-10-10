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

bool D3D12RenderingAPI::Initialize(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug)
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

	SwapChain = MakeShared<D3D12SwapChain>(Device.Get());
	if (!SwapChain->Initialize(StaticCast<WindowsWindow>(RenderWindow).Get(), Queue.Get()))
	{
		return false;
	}

	return true;
}

Texture1D* D3D12RenderingAPI::CreateTexture1D() const
{
	return nullptr;
}

Texture2D* D3D12RenderingAPI::CreateTexture2D() const
{
	return nullptr;
}

Texture2DArray* D3D12RenderingAPI::CreateTexture2DArray() const
{
	return nullptr;
}

Texture3D* D3D12RenderingAPI::CreateTexture3D() const
{
	return nullptr;
}

TextureCube* D3D12RenderingAPI::CreateTextureCube() const
{
	return nullptr;
}

VertexBuffer* D3D12RenderingAPI::CreateVertexBuffer(Uint32 VertexCount, Uint32 VertexStride) const
{
	const Uint64 SizeInBytes = VertexCount * VertexStride;

	ComRef<ID3D12Resource> Buffer = AllocateBuffer(D3D12_HEAP_TYPE_DEFAULT, SizeInBytes);
	if (!Buffer)
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate buffer");
		return nullptr;
	}

	D3D12VertexBuffer* NewBuffer = new D3D12VertexBuffer(Device.Get(), VertexCount, VertexStride);
	NewBuffer->Resource = Buffer;

	D3D12_VERTEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	View.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	View.SizeInBytes	= SizeInBytes;
	View.StrideInBytes	= VertexStride;

	return NewBuffer;
}

IndexBuffer* D3D12RenderingAPI::CreateIndexBuffer(Uint32 IndexCount, EFormat IndexFormat) const
{
	D3D12IndexBuffer* NewBuffer = new D3D12IndexBuffer(Device.Get(), IndexCount, IndexFormat);
	return NewBuffer;
}

ConstantBuffer* D3D12RenderingAPI::CreateConstantBuffer(Uint32 SizeInBytes) const
{
	D3D12ConstantBuffer* NewBuffer = new D3D12ConstantBuffer(Device.Get(), SizeInBytes);
	return NewBuffer;
}

StructuredBuffer* D3D12RenderingAPI::CreateStructuredBuffer(Uint32 SizeInBytes, Uint32 StructuredByteStride) const
{
	D3D12StructuredBuffer* NewBuffer = new D3D12StructuredBuffer(Device.Get(), SizeInBytes, StructuredByteStride);
	return NewBuffer;
}

ByteAddressBuffer* D3D12RenderingAPI::CreateByteAddressBuffer(Uint32 SizeInBytes) const
{
	D3D12ByteAddressBuffer* NewBuffer = new D3D12ByteAddressBuffer(Device.Get(), SizeInBytes);
	return NewBuffer;
}

RayTracingGeometry* D3D12RenderingAPI::CreateRayTracingGeometry() const
{
	return nullptr;
}

RayTracingScene* D3D12RenderingAPI::CreateRayTracingScene() const
{
	return nullptr;
}

Shader* D3D12RenderingAPI::CreateShader() const
{
	return nullptr;
}

DepthStencilState* D3D12RenderingAPI::CreateDepthStencilState() const
{
	return nullptr;
}

RasterizerState* D3D12RenderingAPI::CreateRasterizerState() const
{
	return nullptr;
}

BlendState* D3D12RenderingAPI::CreateBlendState() const
{
	return nullptr;
}

InputLayout* D3D12RenderingAPI::CreateInputLayout() const
{
	return nullptr;
}

GraphicsPipelineState* D3D12RenderingAPI::CreateGraphicsPipelineState() const
{
	return nullptr;
}

ComputePipelineState* D3D12RenderingAPI::CreateComputePipelineState() const
{
	return nullptr;
}

RayTracingPipelineState* D3D12RenderingAPI::CreateRayTracingPipelineState() const
{
	return nullptr;
}

ICommandContext* D3D12RenderingAPI::CreateCommandContext() const
{
	return nullptr;
}

CommandList& D3D12RenderingAPI::GetDefaultCommandList() const
{
	// TODO: insert return statement here
}

CommandExecutor& D3D12RenderingAPI::GetDefaultCommandExecutor() const
{
	// TODO: insert return statement here
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

ComRef<ID3D12Resource> D3D12RenderingAPI::AllocateBuffer(D3D12_HEAP_TYPE HeapType, Uint32 SizeInBytes) const
{
	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags					= ConvertBufferFlags(InInitializer.Flags);
	Desc.Format					= DXGI_FORMAT_UNKNOWN;
	Desc.Width					= SizeInBytes;
	Desc.Height					= 1;
	Desc.DepthOrArraySize		= 1;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= 1;
	Desc.SampleDesc.Count		= 1;
	Desc.SampleDesc.Quality		= 0;

	ComRef<ID3D12Resource> CommitedResource;
	HRESULT HR = Device->CreateCommitedResource();
	if (FAILED(HR))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create resource");
		return ComRef<ID3D12Resource>();
	}

	return CommitedResource;
}
