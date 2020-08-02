#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12CommandAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"

D3D12CommandList::D3D12CommandList(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, CommandList(nullptr)
	, DXRCommandList(nullptr)
	, DeferredResourceBarriers()
{
}

D3D12CommandList::~D3D12CommandList()
{
	SAFEDELETE(UploadBuffer);
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

		return CreateUploadBuffer();
	}
	else
	{
		LOG_ERROR("[D3D12CommandList]: FAILED to create CommandList");
		return false;
	}
}

void D3D12CommandList::UploadBufferData(D3D12Buffer* Dest, const Uint32 DestOffset, const void* Src, const Uint32 SizeInBytes)
{
	const Uint32 NewOffset = UploadBufferOffset + SizeInBytes;
	if (NewOffset >= UploadBuffer->GetSizeInBytes())
	{
		// Destroy old buffer
		DeferDestruction(UploadBuffer);
		SAFEDELETE(UploadBuffer);

		// Create new one
		CreateUploadBuffer(NewOffset);
	}

	// Copy to GPU buffer
	memcpy(UploadPointer + UploadBufferOffset, Src, SizeInBytes);
	// Copy to Dest
	CopyBuffer(Dest, DestOffset, UploadBuffer, UploadBufferOffset, SizeInBytes);

	UploadBufferOffset = NewOffset;
}

void D3D12CommandList::UploadTextureData(class D3D12Texture* Dest, const void* Src, DXGI_FORMAT Format, const Uint32 Width, const Uint32 Height, const Uint32 Depth, const Uint32 Stride, const Uint32 RowPitch)
{
	const Uint32 SizeInBytes	= Height * RowPitch;
	const Uint32 NewOffset		= UploadBufferOffset + SizeInBytes;
	if (NewOffset >= UploadBuffer->GetSizeInBytes())
	{
		// Destroy old buffer
		DeferDestruction(UploadBuffer);
		SAFEDELETE(UploadBuffer);

		// Create new one
		CreateUploadBuffer(NewOffset);
	}

	// Copy to GPU buffer
	Byte* Memory = UploadPointer + UploadBufferOffset;
	const Byte* Source = reinterpret_cast<const Byte*>(Src);
	for (Uint32 Y = 0; Y < Height; Y++)
	{
		memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Memory) + Y * RowPitch), Source + (Y * Width * Stride), Width * Stride);
	}

	// Copy to Dest
	D3D12_TEXTURE_COPY_LOCATION SourceLocation = { };
	SourceLocation.pResource							= UploadBuffer->GetResource();
	SourceLocation.Type									= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	SourceLocation.PlacedFootprint.Footprint.Format		= Format;
	SourceLocation.PlacedFootprint.Footprint.Width		= Width;
	SourceLocation.PlacedFootprint.Footprint.Height		= Height;
	SourceLocation.PlacedFootprint.Footprint.Depth		= 1;
	SourceLocation.PlacedFootprint.Footprint.RowPitch	= RowPitch;

	D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
	DestLocation.pResource			= Dest->GetResource();
	DestLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	DestLocation.SubresourceIndex	= 0;
	
	CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);

	UploadBufferOffset = NewOffset;
}

void D3D12CommandList::DeferDestruction(D3D12Resource* Resource)
{
	ResourcesPendingRelease.emplace_back(Resource->GetResource());
}

void D3D12CommandList::TransitionBarrier(D3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
	D3D12_RESOURCE_BARRIER Barrier = { };
	Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource	= Resource->GetResource();
	Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Barrier.Transition.StateBefore	= BeforeState;
	Barrier.Transition.StateAfter	= AfterState;

	DeferredResourceBarriers.push_back(Barrier);
}

void D3D12CommandList::UnorderedAccessBarrier(D3D12Resource* Resource)
{
	D3D12_RESOURCE_BARRIER Barrier = { };
	Barrier.Type			= D3D12_RESOURCE_BARRIER_TYPE_UAV;
	Barrier.UAV.pResource	= Resource->GetResource();
	
	DeferredResourceBarriers.push_back(Barrier);
}

void D3D12CommandList::SetDebugName(const std::string& DebugName)
{
	std::wstring WideDebugName = ConvertToWide(DebugName);
	CommandList->SetName(WideDebugName.c_str());
}

bool D3D12CommandList::CreateUploadBuffer(Uint32 SizeInBytes)
{
	BufferProperties UploadBufferProps = { };
	UploadBufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	UploadBufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;
	UploadBufferProps.SizeInBytes	= SizeInBytes;
	UploadBufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;

	UploadBuffer = new D3D12Buffer(Device);
	if (UploadBuffer->Initialize(UploadBufferProps))
	{
		UploadBufferOffset = 0;

		UploadPointer = reinterpret_cast<Byte*>(UploadBuffer->Map());
		return true;
	}
	else
	{
		return false;
	}
}
