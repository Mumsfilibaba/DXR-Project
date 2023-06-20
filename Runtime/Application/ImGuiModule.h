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

    static FResponse OnGamepadButtonEvent(EControllerButton Button, bool bIsDown);

    static FResponse OnGamepadAnalogEvent(EControllerAnalog AnalogSource, float Analog);

    static FResponse OnKeyEvent(EKey Key, bool bIsDown);

    static FResponse OnKeyCharEvent(uint32 Character);

    static FResponse OnMouseMoveEvent(int32 x, int32 y);

    static FResponse OnMouseButtonEvent(EMouseButton ButtonIndex, bool bIsDown);

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

private:
    static ImGuiContext* Context;
};