#pragma once
#include "ViewportRenderer.h"
#include "Widgets/Viewport.h"
#include "Widgets/Window.h"

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

    static FResponse OnGamepadButtonEvent(EControllerButton Button, bool bIsDown);

    static FResponse OnGamepadAnalogEvent(EControllerAnalog AnalogSource, float Analog);

    static FResponse OnKeyEvent(EKey Key, bool bIsDown);

    static FResponse OnKeyCharEvent(uint32 Character);

    static FResponse OnMouseMoveEvent(int32 x, int32 y);

    static FResponse OnMouseButtonEvent(EMouseButton ButtonIndex, bool bIsDown);

    static FResponse OnMouseScrollEvent(float ScrollDelta, bool bVertical);

private:
    static ImGuiContext* Context;
};