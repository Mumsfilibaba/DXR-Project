#pragma once
#include "ImGuiRenderer.h"
#include "Viewport.h"

#include <imgui.h>

class APPLICATION_API FImGui
{
public:
    static void CreateContext();

    static void DestroyContext();

    static bool IsInitialized()
    {
        return Context != nullptr;
    }

    static FORCEINLINE ImGuiContext* GetContext()
    {
        return Context;
    }

    static void InitializeStyle();

    static void SetupMainViewport(FViewport* InViewport);

    static FResponse OnGamepadButtonEvent(EGamepadButtonName::Type Button, bool bIsDown);

    static FResponse OnGamepadAnalogEvent(EAnalogSourceName::Type AnalogSource, float Analog);

    static FResponse OnKeyEvent(EKeyName::Type Key, bool bIsDown);

    static FResponse OnKeyCharEvent(uint32 Character);

    static FResponse OnMouseMoveEvent(int32 x, int32 y);

    static FResponse OnMouseButtonEvent(EMouseButtonName::Type ButtonIndex, bool bIsDown);

    static FResponse OnMouseScrollEvent(float ScrollDelta, bool bVertical);

    static FResponse OnWindowResize(void* PlatformHandle);

    static FResponse OnWindowMoved(void* PlatformHandle);

    static FResponse OnFocusLost();

    static FResponse OnFocusGained();

    static FResponse OnMouseLeft();

    static FResponse OnWindowClose(void* PlatformHandle);

    static FORCEINLINE ImVec2 GetDisplaySize()
    {
        return ImGui::GetIO().DisplaySize;
    }

    static FORCEINLINE ImVec2 GetDisplayFramebufferScale()
    {
        return ImGui::GetIO().DisplayFramebufferScale;
    }

    static FORCEINLINE ImVec2 GetMainViewportPos()
    {
        ImVec2 Position = ImVec2(0.0f, 0.0f);
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Position = Viewport->WorkPos;
        }

        return Position;
    }

    static FORCEINLINE ImVec2 GetMainViewportSize()
    {
        ImVec2 Size = ImVec2(0.0f, 0.0f);
        if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
        {
            Size = Viewport->WorkSize;
        }

        return Size;
    }

private:
    static ImGuiContext* Context;
};