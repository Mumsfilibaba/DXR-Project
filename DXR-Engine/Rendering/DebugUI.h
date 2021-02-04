#pragma once
#include "Application/InputCodes.h"
#include "Application/Events/Events.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/ResourceViews.h"

#include "Core/TSharedRef.h"

#include <imgui.h>

// Used when rendering images with ImGui
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
    TSharedRef<Texture> Image;
    EResourceState      BeforeState;
    EResourceState      AfterState;
    Bool AllowBlending = false;
};

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