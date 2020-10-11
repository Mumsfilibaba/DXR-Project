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
#include "D3D12Helpers.h"

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

VertexBuffer* D3D12RenderingAPI::CreateVertexBuffer(Uint32 SizeInBytes, Uint32 VertexStride) const
{
	D3D12VertexBuffer* NewBuffer = new D3D12VertexBuffer(Device.Get(), SizeInBytes, VertexStride);
	if (!AllocateBuffer(*NewBuffer, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, SizeInBytes))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate buffer");
		return nullptr;
	}

	D3D12_VERTEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	View.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	View.SizeInBytes	= SizeInBytes;
	View.StrideInBytes	= VertexStride;
	NewBuffer->VertexBufferView = View;

	return NewBuffer;
}

IndexBuffer* D3D12RenderingAPI::CreateIndexBuffer(Uint32 SizeInBytes, EFormat IndexFormat) const
{
	D3D12IndexBuffer* NewBuffer = new D3D12IndexBuffer(Device.Get(), SizeInBytes, IndexFormat);
	if (!AllocateBuffer(*NewBuffer, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, SizeInBytes))
	{
		return nullptr;
	}
	
	D3D12_INDEX_BUFFER_VIEW View;
	Memory::Memzero(&View, sizeof(D3D12_INDEX_BUFFER_VIEW));

	View.BufferLocation = NewBuffer->GetGPUVirtualAddress();
	View.Format			= ConvertFormat();
	View.SizeInBytes	= SizeInBytes;
	NewBuffer->IndexBufferView = View;

	return NewBuffer;
}

ConstantBuffer* D3D12RenderingAPI::CreateConstantBuffer(Uint32 SizeInBytes) const
{
	D3D12ConstantBuffer* NewBuffer = new D3D12ConstantBuffer(Device.Get(), SizeInBytes);
	if (!AllocateBuffer(*NewBuffer, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, SizeInBytes))
	{
		return nullptr;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
	Memory::Memzero(&ViewDesc, sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC));

	ViewDesc.BufferLocation	= NewBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes	= SizeInBytes;

	D3D12ConstantBufferView* View = new D3D12ConstantBufferView(Device.Get(), Buffer.Get(), ViewDesc);
	NewBuffer->View = View;

	return NewBuffer;
}

StructuredBuffer* D3D12RenderingAPI::CreateStructuredBuffer(Uint32 ElementCount, Uint32 StructuredByteStride) const
{
	const Uint32 SizeInBytes = ElementCount * StructuredByteStride;

	D3D12StructuredBuffer* NewBuffer = new D3D12StructuredBuffer(Device.Get(), ElementCount, StructuredByteStride);
	if (!AllocateBuffer(*NewBuffer, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, SizeInBytes))
	{
		return nullptr;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
	Memory::Memzero(&ViewDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));
	
	ViewDesc.Format						= DXGI_FORMAT_UNKNOWN;
	ViewDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	ViewDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ViewDesc.Buffer.FirstElement		= 0;
	ViewDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
	ViewDesc.Buffer.NumElements			= ElementCount;
	ViewDesc.Buffer.StructureByteStride	= StructuredByteStride;
	
	D3D12ShaderResourceView* View = new D3D12ShaderResourceView(Device.Get(), Buffer.Get(), ViewDesc);
	NewBuffer->View = View;

	return NewBuffer;
}

ByteAddressBuffer* D3D12RenderingAPI::CreateByteAddressBuffer(Uint32 SizeInBytes) const
{
	D3D12ByteAddressBuffer* NewBuffer = new D3D12ByteAddressBuffer(Device.Get(), SizeInBytes);
	if (!AllocateBuffer(*NewBuffer, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, SizeInBytes))
	{
		return nullptr;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc;
	Memory::Memzero(&ViewDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	ViewDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
	ViewDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	ViewDesc.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ViewDesc.Buffer.FirstElement		= 0;
	ViewDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	ViewDesc.Buffer.NumElements			= SizeInBytes / 4;
	ViewDesc.Buffer.StructureByteStride = 0;

	D3D12ShaderResourceView* View = new D3D12ShaderResourceView(Device.Get(), NewBuffer->D3DResource.Get(), ViewDesc);
	NewBuffer->View = View;

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

bool D3D12RenderingAPI::AllocateBuffer(D3D12Resource& Resource, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_FLAGS Flags, Uint32 SizeInBytes) const
{
	D3D12_HEAP_PROPERTIES HeapProperties;
	Memory::Memzero(&HeapProperties, sizeof(D3D12_HEAP_PROPERTIES));

	HeapProperties.Type					= HeapType;
	HeapProperties.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC Desc;
	Memory::Memzero(&Desc, sizeof(D3D12_RESOURCE_DESC));

	Desc.Dimension				= D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags					= Flags;
	Desc.Format					= DXGI_FORMAT_UNKNOWN;
	Desc.Width					= SizeInBytes;
	Desc.Height					= 1;
	Desc.DepthOrArraySize		= 1;
	Desc.Layout					= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels				= 1;
	Desc.SampleDesc.Count		= 1;
	Desc.SampleDesc.Quality		= 0;

	HRESULT HR = Device->CreateCommitedResource(
		&HeapProperties, 
		D3D12_HEAP_FLAG_NONE, 
		&Desc, 
		D3D12_RESOURCE_STATE_COMMON, 
		nullptr, 
		IID_PPV_ARGS(&Resource.D3DResource));
	if (FAILED(HR))
	{
		LOG_ERROR("[D3D12RenderingAPI]: Failed to create resource");
		return false;
	}
	else
	{
		return true;
	}
}
