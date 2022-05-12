#pragma once
#include "GenericWindow.h"

#include "Core/Input/InputCodes.h"
#include "Core/Input/ModifierKeyState.h"
#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericApplicationMessageHandler

class CGenericApplicationMessageHandler
{
public:

    virtual ~CGenericApplicationMessageHandler() = default;

    virtual void HandleKeyReleased(EKey KeyCode, SModifierKeyState ModierKeyState) { }

    virtual void HandleKeyPressed(EKey KeyCode, bool bIsRepeat, SModifierKeyState ModierKeyState) { }

    virtual void HandleKeyChar(uint32 Character) { }

    virtual void HandleMouseMove(int32 x, int32 y) { }

    virtual void HandleMouseReleased(EMouseButton Button, SModifierKeyState ModierKeyState) { }

    virtual void HandleMousePressed(EMouseButton Button, SModifierKeyState ModierKeyState) { }

    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) { }

    virtual void HandleHighPrecisionMouseInput(const TSharedRef<CGenericWindow>& Window, int32 x, uint32 y) { }

    virtual void HandleWindowResized(const TSharedRef<CGenericWindow>& Window, uint32 Width, uint32 Height) { }

    virtual void HandleWindowMoved(const TSharedRef<CGenericWindow>& Window, int32 x, int32 y) { }

    virtual void HandleWindowFocusChanged(const TSharedRef<CGenericWindow>& Window, bool bHasFocus) { }

    virtual void HandleWindowMouseLeft(const TSharedRef<CGenericWindow>& Window) { }

    virtual void HandleWindowMouseEntered(const TSharedRef<CGenericWindow>& Window) { }

    virtual void HandleWindowClosed(const TSharedRef<CGenericWindow>& Window) { }

    virtual void HandleApplicationExit(int32 ExitCode) { }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
