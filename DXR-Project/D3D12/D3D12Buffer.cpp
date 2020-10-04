#include "D3D12Buffer.h"
#include "D3D12Device.h"
#include "D3D12Helpers.h"

#include <codecvt>
#include <locale>

D3D12Buffer::D3D12Buffer(D3D12Device* InDevice)
	: D3D12Resource(InDevice)
	, ConstantBufferView()
{
}

D3D12Buffer::~D3D12Buffer()
{
}

bool D3D12Buffer::Initialize(const BufferInitializer& InInitializer)
{
	VALIDATE(InInitializer.SizeInBytes > 0);

	// Align data properly
	Uint32 SizeInBytes = InInitializer.SizeInBytes;
	if (InInitializer.Flags & EBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER)
	{
		SizeInBytes = AlignUp<Uint32>(SizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	// Create resource
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.DepthOrArraySize	= 1;
	ResourceDesc.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Flags				= ConvertBufferFlags(InInitializer.Flags);
	ResourceDesc.Format				= DXGI_FORMAT_UNKNOWN;
	ResourceDesc.Height				= 1;
	ResourceDesc.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.MipLevels			= 1;
	ResourceDesc.SampleDesc.Count	= 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Width				= SizeInBytes;

	D3D12_RESOURCE_STATES InitalState = D3D12_RESOURCE_STATE_COMMON;
	if (CreateResource(&ResourceDesc, nullptr, InitalState, InInitializer.MemoryType))
	{
		LOG_INFO("[D3D12Buffer]: Created Buffer");

		Initializer = InInitializer;
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12Buffer]: FAILED to create Buffer");
		return false;
	}
}

VoidPtr D3D12Buffer::Map()
{
	VoidPtr HostMemory = nullptr;
	if (SUCCEEDED(NativeResource->Map(0, nullptr, &HostMemory)))
	{
		return HostMemory;
	}
	else
	{
		return nullptr;
	}
}

void D3D12Buffer::Unmap()
{
	NativeResource->Unmap(0, nullptr);
}

void D3D12Buffer::SetName(const std::string& InName)
{
}
