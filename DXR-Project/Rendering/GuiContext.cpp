#include "GuiContext.h"
#include "Defines.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12CommandList.h"
#include "D3D12/D3D12CommandAllocator.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12GraphicsPipelineState.h"
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12Fence.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12RootSignature.h"
#include "D3D12/HeapProps.h"
#include "D3D12/D3D12ShaderCompiler.h"
#include "D3D12/D3D12DescriptorHeap.h"

#include "Application/InputCodes.h"
#include "Application/Application.h"

std::unique_ptr<GuiContext> GuiContext::Instance = nullptr;

GuiContext::GuiContext()
{
}

GuiContext::~GuiContext()
{
	ImGui::DestroyContext(Context);
}

GuiContext* GuiContext::Create(std::shared_ptr<D3D12Device>& InDevice)
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
	ImGuiIO& IO = ImGui::GetIO();
	
	// Get the display size
	WindowShape WindowShape;
	Application::Get()->GetWindow()->GetWindowShape(WindowShape);

	IO.DisplaySize = ImVec2(WindowShape.Width, WindowShape.Height);

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

	// Get Draw data
	ImDrawData* DrawData = ImGui::GetDrawData();

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
	struct VERTEX_CONSTANT_BUFFER
	{
		Float32 MVP[4][4];
	} VertexConstantBuffer = { };

	Float32 L = DrawData->DisplayPos.x;
	Float32 R = DrawData->DisplayPos.x + DrawData->DisplaySize.x;
	Float32 T = DrawData->DisplayPos.y;
	Float32 B = DrawData->DisplayPos.y + DrawData->DisplaySize.y;
	Float32 MVP[4][4] =
	{
		{ 2.0f / (R - L),		0.0f,				0.0f,	0.0f },
		{ 0.0f,					2.0f / (T - B),		0.0f,	0.0f },
		{ 0.0f,					0.0f,				0.5f,	0.0f },
		{ (R + L) / (L - R),	(T + B) / (B - T),	0.5f,	1.0f },
	};

	memcpy(&VertexConstantBuffer.MVP, MVP, sizeof(MVP));

	// Setup viewport
	D3D12_VIEWPORT ViewPort = { };
	ZERO_MEMORY(&ViewPort, sizeof(D3D12_VIEWPORT));

	ViewPort.Width		= DrawData->DisplaySize.x;
	ViewPort.Height		= DrawData->DisplaySize.y;
	ViewPort.MinDepth	= 0.0f;
	ViewPort.MaxDepth	= 1.0f;
	ViewPort.TopLeftX	= 0.0f;
	ViewPort.TopLeftY	= 0.0f;
	CommandList->RSSetViewports(&ViewPort, 1);

	// Bind shader and vertex buffers
	Uint32 Stride = sizeof(ImDrawVert);
	Uint32 Offset = 0;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = { };
	ZERO_MEMORY(&VertexBufferView, sizeof(D3D12_VERTEX_BUFFER_VIEW));

	VertexBufferView.BufferLocation = VertexBuffer->GetVirtualAddress();
	VertexBufferView.SizeInBytes	= DrawData->TotalVtxCount * Stride;
	VertexBufferView.StrideInBytes	= Stride;
	CommandList->IASetVertexBuffers(0, &VertexBufferView, 1);

	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	ZERO_MEMORY(&IndexBufferView, sizeof(D3D12_INDEX_BUFFER_VIEW));

	IndexBufferView.BufferLocation	= IndexBuffer->GetVirtualAddress();
	IndexBufferView.SizeInBytes		= DrawData->TotalIdxCount * sizeof(ImDrawIdx);
	IndexBufferView.Format			= sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	CommandList->IASetIndexBuffer(&IndexBufferView);
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	CommandList->SetPipelineState(PipelineState->GetPipelineState());
	CommandList->SetGraphicsRootSignature(RootSignature->GetRootSignature());
	CommandList->SetGraphicsRoot32BitConstants(&VertexConstantBuffer, 16, 0, 0);

	// Setup BlendFactor
	const Float32 BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	CommandList->OMSetBlendFactor(BlendFactor);

	// Upload vertex/index data into a single contiguous GPU buffer
	ImDrawVert* VertexDest	= reinterpret_cast<ImDrawVert*>(VertexBuffer->Map());
	ImDrawIdx*	IndexDest	= reinterpret_cast<ImDrawIdx*>(IndexBuffer->Map());
	for (Int32 N = 0; N < DrawData->CmdListsCount; N++)
	{
		const ImDrawList* CmdList = DrawData->CmdLists[N];
		memcpy(VertexDest, CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(IndexDest, CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

		VertexDest	+= CmdList->VtxBuffer.Size;
		IndexDest	+= CmdList->IdxBuffer.Size;
	}
	VertexBuffer->Unmap();
	IndexBuffer->Unmap();

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	Int32	GlobalVertexOffset	= 0;
	Int32	GlobalIndexOffset	= 0;
	ImVec2	ClipOff				= DrawData->DisplayPos;
	for (Int32 N = 0; N < DrawData->CmdListsCount; N++)
	{
		const ImDrawList* CmdList = DrawData->CmdLists[N];
		for (Int32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
		{
			const ImDrawCmd* Cmd = &CmdList->CmdBuffer[CmdIndex];
			//if (Cmd->UserCallback != NULL)
			//{
			//	// User callback, registered via ImDrawList::AddCallback()
			//	// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
			//	if (Cmd->UserCallback == ImDrawCallback_ResetRenderState)
			//		ImGui_ImplDX12_SetupRenderState(draw_data, ctx, fr);
			//	else
			//		Cmd->UserCallback(CmdList, Cmd);
			//}
			//else
			//{
				// Apply Scissor, Bind texture, Draw
				const D3D12_RECT ScissorRect = 
				{ 
					static_cast<LONG>(Cmd->ClipRect.x - ClipOff.x), 
					static_cast<LONG>(Cmd->ClipRect.y - ClipOff.y), 
					static_cast<LONG>(Cmd->ClipRect.z - ClipOff.x), 
					static_cast<LONG>(Cmd->ClipRect.w - ClipOff.y) 
				};
				
				CommandList->SetGraphicsRootDescriptorTable(FontTextureGPUHandle, 1);
				CommandList->RSSetScissorRects(&ScissorRect, 1);

				CommandList->DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
			//}
		}

		GlobalIndexOffset	+= CmdList->IdxBuffer.Size;
		GlobalVertexOffset	+= CmdList->VtxBuffer.Size;
	}
}

void GuiContext::OnMouseMove(Int32 X, Int32 Y)
{
	ImGuiIO& IO = ImGui::GetIO();
	IO.MousePos = ImVec2(static_cast<Float32>(X), static_cast<Float32>(Y));

	::OutputDebugString(("MouseMove: x:" + std::to_string(X) + ", y:" + std::to_string(Y) + "\n").c_str());
}

void GuiContext::OnKeyDown(EKey KeyCode)
{
	ImGuiIO& IO = ImGui::GetIO();
	IO.KeysDown[KeyCode] = true;
}

void GuiContext::OnKeyUp(EKey KeyCode)
{
	ImGuiIO& IO = ImGui::GetIO();
	IO.KeysDown[KeyCode] = false;
}

void GuiContext::OnMouseButtonPressed(EMouseButton Button)
{
	ImGuiIO& IO = ImGui::GetIO();
	Uint32 ButtonIndex = 0;
	if (Button == EMouseButton::MOUSE_BUTTON_LEFT)
	{
		ButtonIndex = 0;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_MIDDLE)
	{
		ButtonIndex = 2;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_RIGHT)
	{
		ButtonIndex = 1;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_BACK)
	{
		ButtonIndex = 3;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_FORWARD)
	{
		ButtonIndex = 4;
	}

	IO.MouseDown[ButtonIndex] = true;
}

void GuiContext::OnMouseButtonReleased(EMouseButton Button)
{
	ImGuiIO& IO = ImGui::GetIO();
	Uint32 ButtonIndex = 0;
	if (Button == EMouseButton::MOUSE_BUTTON_LEFT)
	{
		ButtonIndex = 0;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_MIDDLE)
	{
		ButtonIndex = 2;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_RIGHT)
	{
		ButtonIndex = 1;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_BACK)
	{
		ButtonIndex = 3;
	}
	else if (Button == EMouseButton::MOUSE_BUTTON_FORWARD)
	{
		ButtonIndex = 4;
	}

	IO.MouseDown[ButtonIndex] = false;
}

bool GuiContext::Initialize(std::shared_ptr<D3D12Device>& InDevice)
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

	ImGuiIO& IO = ImGui::GetIO();
	IO.BackendFlags			|= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
	IO.BackendFlags			|= ImGuiBackendFlags_HasSetMousePos;          // We can honor IO.WantSetMousePos requests (optional, rarely used)
	IO.BackendPlatformName	= "Windows";
	IO.ImeWindowHandle		= 0;

	// Keyboard mapping. ImGui will use those indices to peek into the IO.KeysDown[] array that we will update during the application lifetime.
	IO.KeyMap[ImGuiKey_Tab]			= EKey::KEY_TAB;
	IO.KeyMap[ImGuiKey_LeftArrow]	= EKey::KEY_LEFT;
	IO.KeyMap[ImGuiKey_RightArrow]	= EKey::KEY_RIGHT;
	IO.KeyMap[ImGuiKey_UpArrow]		= EKey::KEY_UP;
	IO.KeyMap[ImGuiKey_DownArrow]	= EKey::KEY_DOWN;
	IO.KeyMap[ImGuiKey_PageUp]		= EKey::KEY_PAGE_UP;
	IO.KeyMap[ImGuiKey_PageDown]	= EKey::KEY_PAGE_DOWN;
	IO.KeyMap[ImGuiKey_Home]		= EKey::KEY_HOME;
	IO.KeyMap[ImGuiKey_End]			= EKey::KEY_END;
	IO.KeyMap[ImGuiKey_Insert]		= EKey::KEY_INSERT;
	IO.KeyMap[ImGuiKey_Delete]		= EKey::KEY_DELETE;
	IO.KeyMap[ImGuiKey_Backspace]	= EKey::KEY_BACKSPACE;
	IO.KeyMap[ImGuiKey_Space]		= EKey::KEY_SPACE;
	IO.KeyMap[ImGuiKey_Enter]		= EKey::KEY_ENTER;
	IO.KeyMap[ImGuiKey_Escape]		= EKey::KEY_ESCAPE;
	IO.KeyMap[ImGuiKey_KeyPadEnter] = EKey::KEY_KEYPAD_ENTER;
	IO.KeyMap[ImGuiKey_A]			= EKey::KEY_A;
	IO.KeyMap[ImGuiKey_C]			= EKey::KEY_C;
	IO.KeyMap[ImGuiKey_V]			= EKey::KEY_V;
	IO.KeyMap[ImGuiKey_X]			= EKey::KEY_X;
	IO.KeyMap[ImGuiKey_Y]			= EKey::KEY_Y;
	IO.KeyMap[ImGuiKey_Z]			= EKey::KEY_Z;

	// Setup style
	ImGui::StyleColorsDark();

	// Setup D3D12
	if (!CreateFontTexture())
	{
		return false;
	}

	if (!CreatePipeline())
	{
		return false;
	}

	if (!CreateBuffers())
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

	Uint32 UploadPitch	= (Width * 4 + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
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

	// VertexBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format						= DXGI_FORMAT_R8G8B8A8_UNORM;
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Texture2D.MipLevels			= 1;
	SrvDesc.Texture2D.MostDetailedMip	= 0;

	FontTextureCPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetCPUDescriptorHandleAt(5);
	FontTextureGPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetGPUDescriptorHandleAt(5);

	Device->GetDevice()->CreateShaderResourceView(FontTexture->GetResource(), &SrvDesc, FontTextureCPUHandle);

	return true;
}

bool GuiContext::CreatePipeline()
{
	RootSignature = std::shared_ptr<D3D12RootSignature>(new D3D12RootSignature(Device.get()));
	if (!RootSignature->Initialize())
	{
		return false;
	}

	static const char* VertexShader =
		"cbuffer vertexBuffer : register(b0) \
		{\
			float4x4 ProjectionMatrix; \
		};\
		struct VS_INPUT\
		{\
			float2 pos : POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
		};\
		\
		struct PS_INPUT\
		{\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
		};\
		\
		PS_INPUT main(VS_INPUT input)\
		{\
			PS_INPUT output;\
			output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
			output.col = input.col;\
			output.uv  = input.uv;\
			return output;\
		}";

	IDxcBlob* VSBlob = D3D12ShaderCompiler::Get()->CompileFromSource(VertexShader, "main", "vs_6_1");
	if (!VSBlob)
	{
		return false;
	}

	static const char* PixelShader =
		"struct PS_INPUT\
		{\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
		};\
		SamplerState sampler0 : register(s0);\
		Texture2D texture0 : register(t0);\
		\
		float4 main(PS_INPUT input) : SV_Target\
		{\
			float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
			return out_col; \
		}";

	IDxcBlob* PSBlob = D3D12ShaderCompiler::Get()->CompileFromSource(PixelShader, "main", "ps_6_1");
	if (!PSBlob)
	{
		return false;
	}

	GraphicsPipelineStateProperties GuiPipelineProps = { };
	GuiPipelineProps.Name			= "ImGui Pipeline";
	GuiPipelineProps.RootSignature	= RootSignature.get();
	GuiPipelineProps.VSBlob			= VSBlob;
	GuiPipelineProps.PSBlob			= PSBlob;

	PipelineState = std::shared_ptr<D3D12GraphicsPipelineState>(new D3D12GraphicsPipelineState(Device.get()));
	if (!PipelineState->Initialize(GuiPipelineProps))
	{
		return false;
	}

	return true;
}

bool GuiContext::CreateBuffers()
{
	BufferProperties BufferProps = { };
	BufferProps.Name			= "ImGui VertexBuffer";
	BufferProps.SizeInBytes		= 1024 * 1024 * 8;
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.HeapProperties	= HeapProps::UploadHeap();
	BufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	
	// VertexBuffer
	VertexBuffer = std::shared_ptr<D3D12Buffer>(new D3D12Buffer(Device.get()));
	if (!VertexBuffer->Initialize(BufferProps))
	{
		return false;
	}

	// IndexBuffer
	BufferProps.Name = "ImGui IndexBuffer";

	IndexBuffer = std::shared_ptr<D3D12Buffer>(new D3D12Buffer(Device.get()));
	if (!IndexBuffer->Initialize(BufferProps))
	{
		return false;
	}

	return true;
}
