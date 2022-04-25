#pragma once
#include "PlatformWindow.h"

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
// Interface for a listener to platform messages

class CPlatformApplicationMessageHandler
{
public:

    virtual ~CPlatformApplicationMessageHandler() = default;

    /**
     * @brief: Handle a key released event
     * 
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleKeyReleased(EKey KeyCode, SModifierKeyState ModierKeyState) { }

    /**
     * @brief: Handle a key pressed event
     *
     * @param KeyCode: The key-code for the event
     * @param bIsRepeat: True if this event is a repeated event die to the user holding the key
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleKeyPressed(EKey KeyCode, bool bIsRepeat, SModifierKeyState ModierKeyState) { }

    /**
     * @brief: Handle a key character event
     *
     * @param Character: An character code for the event which takes into account modifier keys
     */
    virtual void HandleKeyChar(uint32 Character) { }

    /**
     * @brief: Handle a mouse move event
     *
     * @param x: The new x-coordinate for the mouse within the client window
     * @param y: The new y-coordinate for the mouse within the client window
     */
    virtual void HandleMouseMove(int32 x, int32 y) { }

    /**
     * @brief: Handle a mouse released event
     *
     * @param Button: The mousebutton-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleMouseReleased(EMouseButton Button, SModifierKeyState ModierKeyState) { }

    /**
     * @brief: Handle a key released event
     *
     * @param Button: The mousebutton-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleMousePressed(EMouseButton Button, SModifierKeyState ModierKeyState) { }

    /**
     * @brief: Handle a mouse scrolled event
     *
     * @param HorizontalDelta: The delta of the horizontal scroll-wheel
     * @param VerticalDelta: The delta of the vertical scroll-wheel
     */
    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) { }

    /**
     * @brief: Handle a high-precision mouse event
     *
     * @param Window: Window which received the event
     * @param x: Delta in x-position for this event
     * @param y: Delta in y-position for this event
     */
    virtual void HandleHighPrecisionMouseInput(const TSharedRef<CPlatformWindow>& Window, int32 x, uint32 y) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleWindowResized(const TSharedRef<CPlatformWindow>& Window, uint32 Width, uint32 Height) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleWindowMoved(const TSharedRef<CPlatformWindow>& Window, int32 x, int32 y) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleWindowFocusChanged(const TSharedRef<CPlatformWindow>& Window, bool bHasFocus) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleWindowMouseLeft(const TSharedRef<CPlatformWindow>& Window) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleWindowMouseEntered(const TSharedRef<CPlatformWindow>& Window) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleWindowClosed(const TSharedRef<CPlatformWindow>& Window) { }

    /**
     * @brief: Handle a key released event
     *
     * @param KeyCode: The key-code for the event
     * @param ModifierKeyState: The state of the modifier keys when the event occurred
     */
    virtual void HandleApplicationExit(int32 ExitCode) { }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
