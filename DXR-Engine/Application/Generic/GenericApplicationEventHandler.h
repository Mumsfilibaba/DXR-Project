#pragma once
#include "Application/InputCodes.h"

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericWindow;
struct ModifierKeyState;

class GenericApplicationEventHandler
{
public:
    virtual ~GenericApplicationEventHandler() = default;

    virtual void OnKeyReleased(EKey KeyCode, const ModifierKeyState& ModierKeyState)
    {
    }

    virtual void OnKeyPressed(EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModierKeyState)
    {
    }

    virtual void OnCharacterInput(uint32 Character)
    {
    }

    virtual void OnMouseMove(int32 x, int32 y)
    {
    }

    virtual void OnMouseButtonReleased(EMouseButton Button, const ModifierKeyState& ModierKeyState)
    {
    }

    virtual void OnMouseButtonPressed(EMouseButton Button, const ModifierKeyState& ModierKeyState)
    {
    }

    virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta)
    {
    }

    virtual void OnWindowResized(const TRef<GenericWindow>& Window, uint16 Width, uint16 Height)
    {
    }

    virtual void OnWindowMoved(const TRef<GenericWindow>& Window, int16 x, int16 y)
    {
    }
    
    virtual void OnWindowFocusChanged(const TRef<GenericWindow>& Window, bool HasFocus)
    {
    }
    
    virtual void OnWindowMouseLeft(const TRef<GenericWindow>& Window)
    {
    }
    
    virtual void OnWindowMouseEntered(const TRef<GenericWindow>& Window)
    {
    }
    
    virtual void OnWindowClosed(const TRef<GenericWindow>& Window)
    {
    }
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif