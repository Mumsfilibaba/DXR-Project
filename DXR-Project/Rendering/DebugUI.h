#pragma once
#include "Application/InputCodes.h"
#include "Application/Events/Events.h"

#include <imgui.h>

/*
* DebugUI
*/

class DebugUI
{
public:
	typedef void(*UIDrawFunc)();

	static Bool Init();
	static void Release();

	static void DrawUI(UIDrawFunc DrawFunc);
	static void DrawDebugString(const std::string& DebugString);

	static Bool OnEvent(const Event& Event);
	
	// Should only be called by the renderer
	static void Render(class CommandList& CmdList);

	static ImGuiContext* GetCurrentContext();
};