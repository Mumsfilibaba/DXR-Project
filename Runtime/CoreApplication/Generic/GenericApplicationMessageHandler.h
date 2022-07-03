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
// FGenericApplicationMessageHandler

class FGenericApplicationMessageHandler
{
public:

    virtual ~FGenericApplicationMessageHandler() = default;

    virtual void HandleKeyReleased(EKey KeyCode, FModifierKeyState ModierKeyState) { }

    virtual void HandleKeyPressed(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState) { }

    virtual void HandleKeyChar(uint32 Character) { }

    virtual void HandleMouseMove(int32 x, int32 y) { }

    virtual void HandleMouseReleased(EMouseButton Button, FModifierKeyState ModierKeyState) { }

    virtual void HandleMousePressed(EMouseButton Button, FModifierKeyState ModierKeyState) { }

    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) { }

    virtual void HandleHighPrecisionMouseInput(const TSharedRef<FGenericWindow>& Window, int32 x, uint32 y) { }

    virtual void HandleWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) { }

    virtual void HandleWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y) { }

    virtual void HandleWindowFocusChanged(const TSharedRef<FGenericWindow>& Window, bool bHasFocus) { }

    virtual void HandleWindowMouseLeft(const TSharedRef<FGenericWindow>& Window) { }

    virtual void HandleWindowMouseEntered(const TSharedRef<FGenericWindow>& Window) { }

    virtual void HandleWindowClosed(const TSharedRef<FGenericWindow>& Window) { }

    virtual void HandleApplicationExit(int32 ExitCode) { }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
