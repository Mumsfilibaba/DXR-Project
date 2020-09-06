#pragma once
#include "Application/InputCodes.h"
#include "Application/Events/Event.h"

#include <imgui.h>

class DebugUI
{
public:
	typedef void(*UIDrawFunc)();

	static bool Initialize();
	static void Release();

	static void DrawUI(UIDrawFunc DrawFunc);
	static void DrawDebugString(const std::string& DebugString);

	static bool OnEvent(const Event& Event);
	
	// Should only be called by the renderer
	static void Render(class D3D12CommandList* CommandList);

	static ImGuiContext* GetCurrentContext();
};