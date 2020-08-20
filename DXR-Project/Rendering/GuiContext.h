#pragma once
#include <imgui.h>

#include "Types.h"

#include "Application/InputCodes.h"
#include "Application/Clock.h"

class D3D12Device;
class D3D12Texture;
class D3D12Buffer;
class D3D12GraphicsPipelineState;
class D3D12RootSignature;
class D3D12DescriptorTable;

class GuiContext
{
public:
	GuiContext();
	~GuiContext();

	void BeginFrame();
	void EndFrame();

	void Render(class D3D12CommandList* CommandList);

	FORCEINLINE ImGuiContext* GetCurrentContext() const
	{
		return Context;
	}

	static GuiContext* Make(TSharedPtr<D3D12Device> Device);
	static GuiContext* Get();

public:
	// EventHandling
	void OnKeyPressed(EKey KeyCode);
	void OnKeyReleased(EKey KeyCode);
	void OnMouseButtonPressed(EMouseButton Button);
	void OnMouseButtonReleased(EMouseButton Button);
	void OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta);
	void OnCharacterInput(Uint32 Character);

private:
	bool Initialize(TSharedPtr<D3D12Device> InDevice);

	bool CreateFontTexture();
	bool CreatePipeline();
	bool CreateBuffers();

private:
	ImGuiContext* Context = nullptr;

	TSharedPtr<D3D12Device>				Device			= nullptr;
	TSharedPtr<D3D12Texture>			FontTexture		= nullptr;
	TSharedPtr<D3D12DescriptorTable>	DescriptorTable	= nullptr;

	Clock FrameClock;

	TSharedPtr<D3D12RootSignature>			RootSignature = nullptr;
	TSharedPtr<D3D12GraphicsPipelineState> PipelineState = nullptr;

	TSharedPtr<D3D12Buffer> VertexBuffer	= nullptr;
	TSharedPtr<D3D12Buffer> IndexBuffer	= nullptr;

	static TUniquePtr<GuiContext> Instance;
};