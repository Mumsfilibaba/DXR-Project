#include "GuiContext.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12CommandList.h"
#include "D3D12/D3D12CommandAllocator.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Fence.h"
#include "D3D12/HeapProps.h"

std::unique_ptr<GuiContext> GuiContext::Instance = nullptr;

GuiContext::GuiContext()
{
}

GuiContext::~GuiContext()
{
	ImGui::DestroyContext(Context);
}

GuiContext* GuiContext::Create(std::shared_ptr<D3D12Device> InDevice)
{
	Instance = std::unique_ptr<GuiContext>(new GuiContext());
	if (Instance->Initialize(InDevice))
	{
		return Instance.get();
	}
	else
	{
		return nullptr;
	}
}

GuiContext* GuiContext::Get()
{
	return Instance.get();
}

void GuiContext::BeginFrame()
{
	ImGui::NewFrame();
}

void GuiContext::EndFrame()
{
	ImGui::EndFrame();
}

void GuiContext::Render(D3D12CommandList* CommandList)
{
	// Render ImgGui draw data
	ImGui::Render();
}

bool GuiContext::Initialize(std::shared_ptr<D3D12Device> InDevice)
{
	// Save device so that we can create resource on the GPU
	Device = InDevice;

	// Create context
	IMGUI_CHECKVERSION();
	Context = ImGui::CreateContext();
	if (!Context)
	{
		return false;
	}

	// Setup style
	ImGui::StyleColorsDark();

	// Setup D3D12
	if (!CreateFontTexture())
	{
		return false;
	}

	return true;
}

bool GuiContext::CreateFontTexture()
{
	// Build texture atlas
	ImGuiIO& IO = ImGui::GetIO();
	Byte*	Pixels	= nullptr;
	Int32	Width	= 0;
	Int32	Height	= 0;
	IO.Fonts->GetTexDataAsRGBA32(&Pixels, &Width, &Height);

	Uint32 UploadPitch	= (Width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
	Uint32 UploadSize	= Height * UploadPitch;

	TextureProperties FontTextureProps = { };
	FontTextureProps.Name			= "FontTexture";
	FontTextureProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	FontTextureProps.Width			= Width;
	FontTextureProps.Height			= Height;
	FontTextureProps.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
	FontTextureProps.HeapProperties	= HeapProps::DefaultHeap();

	FontTexture = std::shared_ptr<D3D12Texture>(new D3D12Texture(Device.get()));
	if (!FontTexture->Initialize(FontTextureProps))
	{
		return false;
	}

	BufferProperties UploadBufferProps = { };
	UploadBufferProps.Name				= "UploadBuffer";
	UploadBufferProps.Flags				= D3D12_RESOURCE_FLAG_NONE;
	UploadBufferProps.InitalState		= D3D12_RESOURCE_STATE_GENERIC_READ;
	UploadBufferProps.SizeInBytes		= UploadSize;
	UploadBufferProps.HeapProperties	= HeapProps::UploadHeap();

	std::shared_ptr<D3D12Buffer> UploadBuffer = std::shared_ptr<D3D12Buffer>(new D3D12Buffer(Device.get()));
	if (UploadBuffer->Initialize(UploadBufferProps))
	{
		void* Memory = UploadBuffer->Map();
		for (Int32 Y = 0; Y < Height; Y++)
		{
			memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Memory) + Y * UploadPitch), Pixels + Y * Width * 4, Width * 4);
		}
		UploadBuffer->Unmap();
	}
	else
	{
		return false;
	}

	std::shared_ptr<D3D12Fence> Fence = std::shared_ptr<D3D12Fence>(new D3D12Fence(Device.get()));
	if (!Fence->Initialize(0))
	{
		return false;
	}

	std::shared_ptr<D3D12CommandAllocator> Allocator = std::shared_ptr<D3D12CommandAllocator>(new D3D12CommandAllocator(Device.get()));
	if (!Allocator->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	std::shared_ptr<D3D12CommandList> CommandList = std::shared_ptr<D3D12CommandList>(new D3D12CommandList(Device.get()));
	if (!CommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator.get(), nullptr))
	{
		return false;
	}

	std::shared_ptr<D3D12CommandQueue> Queue = std::shared_ptr<D3D12CommandQueue>(new D3D12CommandQueue(Device.get()));
	if (!Queue->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	Allocator->Reset();
	CommandList->Reset(Allocator.get());

	CommandList->TransitionBarrier(FontTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	
	D3D12_TEXTURE_COPY_LOCATION SourceLocation = {};
	SourceLocation.pResource							= UploadBuffer->GetResource();
	SourceLocation.Type									= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	SourceLocation.PlacedFootprint.Footprint.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
	SourceLocation.PlacedFootprint.Footprint.Width		= Width;
	SourceLocation.PlacedFootprint.Footprint.Height		= Height;
	SourceLocation.PlacedFootprint.Footprint.Depth		= 1;
	SourceLocation.PlacedFootprint.Footprint.RowPitch	= UploadPitch;

	D3D12_TEXTURE_COPY_LOCATION DestLocation = {};
	DestLocation.pResource			= FontTexture->GetResource();
	DestLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	DestLocation.SubresourceIndex	= 0;
	
	CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SourceLocation, nullptr);
	CommandList->TransitionBarrier(FontTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	CommandList->Close();

	Queue->ExecuteCommandList(CommandList.get());
	Queue->SignalFence(Fence.get(), 1);

	Fence->WaitForValue(1);

	return true;
}
