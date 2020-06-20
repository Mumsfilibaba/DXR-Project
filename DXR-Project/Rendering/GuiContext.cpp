#include "GuiContext.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12CommandList.h"
#include "D3D12/D3D12CommandAllocator.h"
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

	TextureProperties FontTextureProps = { };
	FontTextureProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	FontTextureProps.Width			= Width;
	FontTextureProps.Height			= Height;
	FontTextureProps.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
	FontTextureProps.HeapProperties	= HeapProps::DefaultHeap();

	FontTexture = std::shared_ptr<D3D12Texture>(new D3D12Texture(Device.get()));
	if (FontTexture->Initialize(FontTextureProps))
	{
		return false;
	}

	BufferProperties UploadBufferProps = { };
	UploadBufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	UploadBufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
	UploadBufferProps.SizeInBytes	= Width * Height * 4;
	UploadBufferProps.HeapProperties = HeapProps::UploadHeap();

	std::shared_ptr<D3D12Buffer> UploadBuffer = std::shared_ptr<D3D12Buffer>(new D3D12Buffer(Device.get()));
	if (!UploadBuffer->Initialize(UploadBufferProps))
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



	return true;
}
