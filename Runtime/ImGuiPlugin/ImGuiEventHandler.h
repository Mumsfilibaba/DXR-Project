#pragma once
#include "Application/Events.h"

struct FImGuiEventHandler
{
    FResponse OnGamepadButtonEvent(FKey Key, bool bIsDown);
    FResponse OnGamepadAnalogEvent(EAnalogSourceName::Type AnalogSource, float Analog);
    FResponse OnKeyEvent(FKey Key, FModifierKeyState ModifierKeyState, bool bIsDown);
    FResponse OnKeyCharEvent(uint32 Character);
    FResponse OnMouseMoveEvent(int32 x, int32 y);
    FResponse OnMouseButtonEvent(FKey Key, bool bIsDown);
    FResponse OnMouseScrollEvent(float ScrollDelta, bool bVertical);
    FResponse OnMouseLeft();
    FResponse OnWindowResize(void* PlatformHandle);
    FResponse OnWindowMoved(void* PlatformHandle);
    FResponse OnWindowClose(void* PlatformHandle);
    FResponse OnFocusLost();
    FResponse OnFocusGained();
};