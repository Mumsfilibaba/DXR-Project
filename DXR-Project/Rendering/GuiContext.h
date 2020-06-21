#pragma once
#include <memory>
#include <imgui.h>

#include "Types.h"

#include "Application/InputCodes.h"

class D3D12Device;
class D3D12Texture;
class D3D12Buffer;
class D3D12GraphicsPipelineState;
class D3D12RootSignature;

class GuiContext
{
public:
	GuiContext();
	~GuiContext();

	void BeginFrame();
	void EndFrame();

	void Render(class D3D12CommandList* InCommandList);

	void OnMouseMove(Int32 X, Int32 Y);
	void OnKeyDown(EKey KeyCode);
	void OnKeyUp(EKey KeyCode);
	void OnMouseButtonPressed(EMouseButton Button);
	void OnMouseButtonReleased(EMouseButton Button);

	ImGuiContext* GetCurrentContext() const
	{
		return Context;
	}

	static GuiContext* Create(std::shared_ptr<D3D12Device>& InDevice);
	static GuiContext* Get();

private:
	bool Initialize(std::shared_ptr<D3D12Device>& InDevice);

	bool CreateFontTexture();
	bool CreatePipeline();
	bool CreateBuffers();

private:
	ImGuiContext*					Context		= nullptr;
	std::shared_ptr<D3D12Device>	Device		= nullptr;
	std::shared_ptr<D3D12Texture>	FontTexture	= nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE FontTextureCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE FontTextureGPUHandle;

	std::shared_ptr<D3D12RootSignature>			RootSignature = nullptr;
	std::shared_ptr<D3D12GraphicsPipelineState> PipelineState = nullptr;

	std::shared_ptr<D3D12Buffer> VertexBuffer	= nullptr;
	std::shared_ptr<D3D12Buffer> IndexBuffer	= nullptr;

	static std::unique_ptr<GuiContext> Instance;
};