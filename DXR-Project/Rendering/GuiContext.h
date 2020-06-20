#pragma once
#include <memory>
#include <imgui.h>

class D3D12Device;
class D3D12Texture;

class GuiContext
{
public:
	GuiContext();
	~GuiContext();

	void BeginFrame();
	void EndFrame();

	void Render(class D3D12CommandList* InCommandList);

	ImGuiContext* GetCurrentContext() const
	{
		return Context;
	}

	static GuiContext* Create(std::shared_ptr<D3D12Device> InDevice);
	static GuiContext* Get();

private:
	bool Initialize(std::shared_ptr<D3D12Device> InDevice);

	bool CreateFontTexture();

private:
	ImGuiContext*					Context		= nullptr;
	std::shared_ptr<D3D12Device>	Device		= nullptr;
	std::shared_ptr<D3D12Texture>	FontTexture	= nullptr;

	static std::unique_ptr<GuiContext> Instance;
};