#pragma once
#include "GenericWindow.h"
#include "Core/Input/InputCodes.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FGenericApplicationMessageHandler
{
    virtual ~FGenericApplicationMessageHandler() = default;

    virtual void OnKeyReleased(EKey KeyCode, FModifierKeyState ModierKeyState) { }

    virtual void OnKeyPressed(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState) { }

    virtual void OnKeyChar(uint32 Character) { }

    virtual void OnCursorMove(int32 x, int32 y) { }

    virtual void OnCursorReleased(EMouseButton Button, FModifierKeyState ModierKeyState) { }

    virtual void OnCursorPressed(EMouseButton Button, FModifierKeyState ModierKeyState) { }

    virtual void OnCursorScrolled(float HorizontalDelta, float VerticalDelta) { }

    virtual void OnHighPrecisionMouseInput(const FGenericWindowRef& Window, int32 x, uint32 y) { }

    virtual void OnWindowResized(const FGenericWindowRef& Window, uint32 Width, uint32 Height) { }

    virtual void OnWindowMoved(const FGenericWindowRef& Window, int32 x, int32 y) { }

    virtual void OnWindowCursorEntered(const FGenericWindowRef& Window) { }

    virtual void OnWindowCursorLeft(const FGenericWindowRef& Window) { }
    
    virtual void OnWindowFocusLost(const FGenericWindowRef& Window) { }
    
    virtual void OnWindowFocusGained(const FGenericWindowRef& Window) { }

    virtual void OnWindowClosed(const FGenericWindowRef& Window) { }

    virtual void OnApplicationExit(int32 ExitCode) { }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
