#pragma once
#include <memory>
#include <imgui.h>

class GuiContext
{
public:
	GuiContext();
	~GuiContext();

	void BeginFrame();
	void EndFrame();

	ImGuiContext* GetCurrentContext() const
	{
		return Context;
	}

	static GuiContext* Create();
	static GuiContext* Get();

private:

	bool Initialize();

private:
	ImGuiContext* Context = nullptr;

	static std::unique_ptr<GuiContext> Instance;
};