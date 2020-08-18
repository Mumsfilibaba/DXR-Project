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
#include "D3D12/D3D12ShaderCompiler.h"
#include "D3D12/D3D12DescriptorHeap.h"

#include "Application/InputCodes.h"
#include "Application/Application.h"

#include "TextureFactory.h"

TUniquePtr<GuiContext> GuiContext::Instance = nullptr;

GuiContext::GuiContext()
{
}

GuiContext::~GuiContext()
{
	ImGui::DestroyContext(Context);
}

GuiContext* GuiContext::Make(TSharedPtr<D3D12Device> Device)
{
	Instance = TUniquePtr<GuiContext>(new GuiContext());
	if (Instance->Initialize(Device))
	{
		return Instance.Get();
	}
	else
	{
		return nullptr;
	}
}

GuiContext* GuiContext::Get()
{
	return Instance.Get();
}

void GuiContext::BeginFrame()
{
	ImGuiIO& IO = ImGui::GetIO();
	
	// Get deltatime
	FrameClock.Tick();

	Timestamp Delta = FrameClock.GetDeltaTime();
	IO.DeltaTime	= static_cast<Float32>(Delta.AsSeconds());

	// Set Mouseposition
	TSharedPtr<WindowsWindow> Window = Application::Get()->GetWindow();
	if (IO.WantSetMousePos)
	{
		Application::Get()->SetCursorPos(Window, static_cast<Int32>(IO.MousePos.x), static_cast<Int32>(IO.MousePos.y));
	}

	// Get the display size
	WindowShape CurrentWindowShape;
	Window->GetWindowShape(CurrentWindowShape);

	IO.DisplaySize = ImVec2(CurrentWindowShape.Width, CurrentWindowShape.Height);

	// Get Mouseposition
	Int32 X = 0;
	Int32 Y = 0;
	Application::Get()->GetCursorPos(Window, X, Y);

	IO.MousePos = ImVec2(static_cast<Float32>(X), static_cast<Float32>(Y));

	// Check modifer keys
	ModifierKeyState KeyState = Application::Get()->GetModifierKeyState();
	IO.KeyCtrl	= KeyState.IsCtrlDown();
	IO.KeyShift = KeyState.IsShiftDown();
	IO.KeyAlt	= KeyState.IsAltDown();
	IO.KeySuper = KeyState.IsSuperKeyDown();

	// Set MouseCursor
	if (!(IO.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
	{
		ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
		if (ImguiCursor == ImGuiMouseCursor_None || IO.MouseDrawCursor)
		{
			Application::Get()->SetCursor(nullptr);
		}
		else
		{
			// Show OS mouse cursor
			TSharedPtr<WindowsCursor> Cursor = CursorArrow;
			switch (ImguiCursor)
			{
			case ImGuiMouseCursor_Arrow:		Cursor = CursorArrow;						break;
			case ImGuiMouseCursor_TextInput:	Cursor = CursorTextInput;					break;
			case ImGuiMouseCursor_ResizeAll:	Cursor = CursorResizeAll;					break;
			case ImGuiMouseCursor_ResizeEW:		Cursor = CursorResizeEastWest;				break;
			case ImGuiMouseCursor_ResizeNS:		Cursor = CursorResizeNorthSouth;			break;
			case ImGuiMouseCursor_ResizeNESW:	Cursor = CursorResizeNorthEastSouthWest;	break;
			case ImGuiMouseCursor_ResizeNWSE:	Cursor = CursorResizeNorthWestSouthEast;	break;
			case ImGuiMouseCursor_Hand:			Cursor = CursorHand;						break;
			case ImGuiMouseCursor_NotAllowed:	Cursor = CursorNotAllowed;					break;
			}

			Application::Get()->SetCursor(Cursor);
		}
	}

	// Begin new frame
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
	ViewPort.Width		= DrawData->DisplaySize.x;
	ViewPort.Height		= DrawData->DisplaySize.y;
	ViewPort.MinDepth	= 0.0f;
	ViewPort.MaxDepth	= 1.0f;
	ViewPort.TopLeftX	= 0.0f;
	ViewPort.TopLeftY	= 0.0f;
	CommandList->RSSetViewports(&ViewPort, 1);

	// Bind shader and vertex buffers
	const Uint32 Stride = sizeof(ImDrawVert);

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = { };
	VertexBufferView.BufferLocation = VertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.SizeInBytes	= DrawData->TotalVtxCount * Stride;
	VertexBufferView.StrideInBytes	= Stride;
	CommandList->IASetVertexBuffers(0, &VertexBufferView, 1);

	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	IndexBufferView.BufferLocation	= IndexBuffer->GetGPUVirtualAddress();
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
				
				CommandList->SetGraphicsRootDescriptorTable(DescriptorTable->GetGPUTableStartHandle(), 1);
				CommandList->RSSetScissorRects(&ScissorRect, 1);

				CommandList->DrawIndexedInstanced(Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
			//}
		}

		GlobalIndexOffset	+= CmdList->IdxBuffer.Size;
		GlobalVertexOffset	+= CmdList->VtxBuffer.Size;
	}
}

void GuiContext::OnKeyPressed(EKey KeyCode)
{
	ImGuiIO& IO = ImGui::GetIO();
	IO.KeysDown[KeyCode] = true;
}

void GuiContext::OnKeyReleased(EKey KeyCode)
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

void GuiContext::OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta)
{
	ImGuiIO& IO = ImGui::GetIO();
	IO.MouseWheel	+= VerticalDelta;
	IO.MouseWheelH	+= HorizontalDelta;
}

void GuiContext::OnCharacterInput(Uint32 Character)
{
	ImGuiIO& IO = ImGui::GetIO();
	IO.AddInputCharacter(Character);
}

bool GuiContext::Initialize(TSharedPtr<D3D12Device> InDevice)
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
	
	TSharedPtr<WindowsWindow> Window = Application::Get()->GetWindow();
	IO.ImeWindowHandle = Window->GetHandle();

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

	ImGuiStyle& Style = ImGui::GetStyle();
	Style.WindowRounding	= 0.0f;
	Style.FrameRounding		= 3.0f;
	Style.GrabRounding		= 4.0f;

	Style.Colors[ImGuiCol_WindowBg].x = 0.15f;
	Style.Colors[ImGuiCol_WindowBg].y = 0.15f;
	Style.Colors[ImGuiCol_WindowBg].z = 0.15f;
	Style.Colors[ImGuiCol_WindowBg].w = 0.9f;

	Style.Colors[ImGuiCol_TitleBg].x = 0.3f;
	Style.Colors[ImGuiCol_TitleBg].y = 0.3f;
	Style.Colors[ImGuiCol_TitleBg].z = 0.3f;
	Style.Colors[ImGuiCol_TitleBg].w = 1.0f;

	Style.Colors[ImGuiCol_TitleBgActive].x = 0.15f;
	Style.Colors[ImGuiCol_TitleBgActive].y = 0.15f;
	Style.Colors[ImGuiCol_TitleBgActive].z = 0.15f;
	Style.Colors[ImGuiCol_TitleBgActive].w = 1.0f;

	Style.Colors[ImGuiCol_FrameBg].x = 0.4f;
	Style.Colors[ImGuiCol_FrameBg].y = 0.4f;
	Style.Colors[ImGuiCol_FrameBg].z = 0.4f;
	Style.Colors[ImGuiCol_FrameBg].w = 1.0f;

	Style.Colors[ImGuiCol_FrameBgHovered].x = 0.3f;
	Style.Colors[ImGuiCol_FrameBgHovered].y = 0.3f;
	Style.Colors[ImGuiCol_FrameBgHovered].z = 0.3f;
	Style.Colors[ImGuiCol_FrameBgHovered].w = 1.0f;

	Style.Colors[ImGuiCol_FrameBgActive].x = 0.24f;
	Style.Colors[ImGuiCol_FrameBgActive].y = 0.24f;
	Style.Colors[ImGuiCol_FrameBgActive].z = 0.24f;
	Style.Colors[ImGuiCol_FrameBgActive].w = 1.0f;

	Style.Colors[ImGuiCol_Button].x = 0.4f;
	Style.Colors[ImGuiCol_Button].y = 0.4f;
	Style.Colors[ImGuiCol_Button].z = 0.4f;
	Style.Colors[ImGuiCol_Button].w = 1.0f;

	Style.Colors[ImGuiCol_ButtonHovered].x = 0.3f;
	Style.Colors[ImGuiCol_ButtonHovered].y = 0.3f;
	Style.Colors[ImGuiCol_ButtonHovered].z = 0.3f;
	Style.Colors[ImGuiCol_ButtonHovered].w = 1.0f;

	Style.Colors[ImGuiCol_ButtonActive].x = 0.25f;
	Style.Colors[ImGuiCol_ButtonActive].y = 0.25f;
	Style.Colors[ImGuiCol_ButtonActive].z = 0.25f;
	Style.Colors[ImGuiCol_ButtonActive].w = 1.0f;

	Style.Colors[ImGuiCol_CheckMark].x = 0.15f;
	Style.Colors[ImGuiCol_CheckMark].y = 0.15f;
	Style.Colors[ImGuiCol_CheckMark].z = 0.15f;
	Style.Colors[ImGuiCol_CheckMark].w = 1.0f;

	Style.Colors[ImGuiCol_SliderGrab].x = 0.15f;
	Style.Colors[ImGuiCol_SliderGrab].y = 0.15f;
	Style.Colors[ImGuiCol_SliderGrab].z = 0.15f;
	Style.Colors[ImGuiCol_SliderGrab].w = 1.0f;

	Style.Colors[ImGuiCol_SliderGrabActive].x = 0.16f;
	Style.Colors[ImGuiCol_SliderGrabActive].y = 0.16f;
	Style.Colors[ImGuiCol_SliderGrabActive].z = 0.16f;
	Style.Colors[ImGuiCol_SliderGrabActive].w = 1.0f;

	Style.Colors[ImGuiCol_ResizeGrip].x = 0.25f;
	Style.Colors[ImGuiCol_ResizeGrip].y = 0.25f;
	Style.Colors[ImGuiCol_ResizeGrip].z = 0.25f;
	Style.Colors[ImGuiCol_ResizeGrip].w = 1.0f;

	Style.Colors[ImGuiCol_ResizeGripHovered].x = 0.35f;
	Style.Colors[ImGuiCol_ResizeGripHovered].y = 0.35f;
	Style.Colors[ImGuiCol_ResizeGripHovered].z = 0.35f;
	Style.Colors[ImGuiCol_ResizeGripHovered].w = 1.0f;

	Style.Colors[ImGuiCol_ResizeGripActive].x = 0.5f;
	Style.Colors[ImGuiCol_ResizeGripActive].y = 0.5f;
	Style.Colors[ImGuiCol_ResizeGripActive].z = 0.5f;
	Style.Colors[ImGuiCol_ResizeGripActive].w = 1.0f;

	Style.Colors[ImGuiCol_Tab].x = 0.55f;
	Style.Colors[ImGuiCol_Tab].y = 0.55f;
	Style.Colors[ImGuiCol_Tab].z = 0.55f;
	Style.Colors[ImGuiCol_Tab].w = 1.0f;

	Style.Colors[ImGuiCol_TabHovered].x = 0.4f;
	Style.Colors[ImGuiCol_TabHovered].y = 0.4f;
	Style.Colors[ImGuiCol_TabHovered].z = 0.4f;
	Style.Colors[ImGuiCol_TabHovered].w = 1.0f;

	Style.Colors[ImGuiCol_TabActive].x = 0.25f;
	Style.Colors[ImGuiCol_TabActive].y = 0.25f;
	Style.Colors[ImGuiCol_TabActive].z = 0.25f;
	Style.Colors[ImGuiCol_TabActive].w = 1.0f;

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

	FontTexture = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromMemory(Device.Get(), Pixels, Width, Height, 0, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (FontTexture)
	{
		DescriptorTable = MakeShared<D3D12DescriptorTable>(Device.Get(), 1);
		DescriptorTable->SetShaderResourceView(FontTexture->GetShaderResourceView(0).Get(), 0);
		DescriptorTable->CopyDescriptors();

		return true;
	}
	else
	{
		return false;
	}
}

bool GuiContext::CreatePipeline()
{
	D3D12_STATIC_SAMPLER_DESC StaticSampler = {};
	StaticSampler.Filter			= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	StaticSampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSampler.MipLODBias		= 0.0f;
	StaticSampler.MaxAnisotropy		= 0;
	StaticSampler.ComparisonFunc	= D3D12_COMPARISON_FUNC_ALWAYS;
	StaticSampler.BorderColor		= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	StaticSampler.MinLOD			= 0.0f;
	StaticSampler.MaxLOD			= 0.0f;
	StaticSampler.ShaderRegister	= 0;
	StaticSampler.RegisterSpace		= 0;
	StaticSampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_DESCRIPTOR_RANGE DescRange = {};
	DescRange.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescRange.NumDescriptors					= 1;
	DescRange.BaseShaderRegister				= 0;
	DescRange.RegisterSpace						= 0;
	DescRange.OffsetInDescriptorsFromTableStart	= 0;

	D3D12_ROOT_PARAMETER Parameters[2];
	Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[0].Constants.ShaderRegister	= 0;
	Parameters[0].Constants.RegisterSpace	= 0;
	Parameters[0].Constants.Num32BitValues	= 16;
	Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

	Parameters[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters[1].DescriptorTable.NumDescriptorRanges	= 1;
	Parameters[1].DescriptorTable.pDescriptorRanges		= &DescRange;
	Parameters[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC RootSignaturDesc;
	RootSignaturDesc.NumParameters		= 2;
	RootSignaturDesc.pParameters		= Parameters;
	RootSignaturDesc.NumStaticSamplers	= 1;
	RootSignaturDesc.pStaticSamplers	= &StaticSampler;
	RootSignaturDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS			|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	RootSignature = TSharedPtr<D3D12RootSignature>(new D3D12RootSignature(Device.Get()));
	if (!RootSignature->Initialize(RootSignaturDesc))
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
		PS_INPUT Main(VS_INPUT input)\
		{\
			PS_INPUT output;\
			output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
			output.col = input.col;\
			output.uv  = input.uv;\
			return output;\
		}";

	IDxcBlob* VSBlob = D3D12ShaderCompiler::CompileFromSource(VertexShader, "Main", "vs_6_1");
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
		float4 Main(PS_INPUT input) : SV_Target\
		{\
			float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
			return out_col; \
		}";

	IDxcBlob* PSBlob = D3D12ShaderCompiler::CompileFromSource(PixelShader, "Main", "ps_6_1");
	if (!PSBlob)
	{
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, pos)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, uv)),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, static_cast<UINT>(IM_OFFSETOF(ImDrawVert, col)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	GraphicsPipelineStateProperties GuiPipelineProps = { };
	GuiPipelineProps.DebugName			= "ImGui Pipeline";
	GuiPipelineProps.RootSignature		= RootSignature.Get();
	GuiPipelineProps.VSBlob				= VSBlob;
	GuiPipelineProps.PSBlob				= PSBlob;
	GuiPipelineProps.InputElements		= InputElementDesc;
	GuiPipelineProps.NumInputElements	= 3;
	GuiPipelineProps.EnableDepth		= false;
	GuiPipelineProps.DepthBufferFormat	= DXGI_FORMAT_UNKNOWN;
	GuiPipelineProps.EnableBlending		= true;

	DXGI_FORMAT Formats[] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM
	};

	GuiPipelineProps.RTFormats			= Formats;
	GuiPipelineProps.NumRenderTargets	= 1;

	PipelineState = TSharedPtr<D3D12GraphicsPipelineState>(new D3D12GraphicsPipelineState(Device.Get()));
	if (!PipelineState->Initialize(GuiPipelineProps))
	{
		return false;
	}

	return true;
}

bool GuiContext::CreateBuffers()
{
	BufferProperties BufferProps = { };
	BufferProps.Name		= "ImGui VertexBuffer";
	BufferProps.SizeInBytes	= 1024 * 1024 * 8;
	BufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;
	
	// VertexBuffer
	VertexBuffer = TSharedPtr<D3D12Buffer>(new D3D12Buffer(Device.Get()));
	if (!VertexBuffer->Initialize(BufferProps))
	{
		return false;
	}

	// IndexBuffer
	BufferProps.Name = "ImGui IndexBuffer";

	IndexBuffer = TSharedPtr<D3D12Buffer>(new D3D12Buffer(Device.Get()));
	if (!IndexBuffer->Initialize(BufferProps))
	{
		return false;
	}

	return true;
}
