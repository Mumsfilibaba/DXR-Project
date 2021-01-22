#pragma once
#include "Application/InputCodes.h"
#include "Application/Events/Events.h"

#include "RenderingCore/Texture.h"
#include "RenderingCore/ResourceViews.h"

#include "Core/TSharedRef.h"

#include <imgui.h>

/*
* ImGuiImage - Should be sent into ImGuiTextureID
*/

struct ImGuiImage
{
	ImGuiImage() = default;

	ImGuiImage(const TSharedRef<ShaderResourceView>& InImageView, const TSharedRef<Texture>& InImage, EResourceState InBefore, EResourceState InAfter)
		: ImageView(InImageView)
		, Image(InImage)
		, BeforeState(InBefore)
		, AfterState(InAfter)
	{
	}

	TSharedRef<ShaderResourceView> ImageView;
	TSharedRef<Texture>	Image;
	EResourceState		BeforeState;
	EResourceState		AfterState;
	Bool AllowBlending = false;
};

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