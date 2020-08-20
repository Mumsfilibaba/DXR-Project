#pragma once
#include "Application/InputCodes.h"

#include <imgui.h>

class DebugUI
{
public:
	typedef void(*DebugGUIDrawFunc)();

	static bool Initialize(TSharedPtr<class D3D12Device> InDevice);
	static void Release();

	static void DrawImgui(DebugGUIDrawFunc DrawFunc);
	static void DrawDebugString(const std::string& DebugString);

	// TODO: Have a single function taking in events (Event-Class)
	static void OnKeyPressed(EKey KeyCode);
	static void OnKeyReleased(EKey KeyCode);
	static void OnMouseButtonPressed(EMouseButton Button);
	static void OnMouseButtonReleased(EMouseButton Button);
	static void OnMouseScrolled(Float32 HorizontalDelta, Float32 VerticalDelta);
	static void OnCharacterInput(Uint32 Character);
	
	// Should only be called by the renderer
	static void Render(class D3D12CommandList* CommandList);

	static ImGuiContext* GetCurrentContext();
};