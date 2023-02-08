#pragma once
#include "GenericWindow.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericApplicationMessageHandler
{
    virtual ~FGenericApplicationMessageHandler() = default;

    virtual void OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState) { }

    virtual void OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState) { }

    virtual void OnKeyChar(uint32 Character) { }

    virtual void OnMouseMove(int32 x, int32 y) { }

    virtual void OnMouseDown(EMouseButton Button, FModifierKeyState ModierKeyState) { }

    virtual void OnMouseUp(EMouseButton Button, FModifierKeyState ModierKeyState) { }

    virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta) { }

    virtual void OnHighPrecisionMouseInput(const TSharedRef<FGenericWindow>& Window, int32 x, uint32 y) { }

    virtual void OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) { }

    virtual void OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y) { }

    virtual void OnWindowMouseEntered(const TSharedRef<FGenericWindow>& Window) { }

    virtual void OnWindowMouseLeft(const TSharedRef<FGenericWindow>& Window) { }
    
    virtual void OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window) { }
    
    virtual void OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window) { }

    virtual void OnWindowClosed(const TSharedRef<FGenericWindow>& Window) { }

    virtual void OnApplicationExit(int32 ExitCode) { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
