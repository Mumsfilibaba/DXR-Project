#pragma once
#include "Core/Input/InputCodes.h"
#include "Core/Application/ModifierKeyState.h"

#include "GenericWindow.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Interface for a listener to OS events */
class CGenericApplicationMessageListener
{
public:

    virtual ~CGenericApplicationMessageListener() = default;

    virtual void OnKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState )
    {
    }

    virtual void OnKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState )
    {
    }

    virtual void OnKeyTyped( uint32 Character )
    {
    }

    virtual void OnMouseMove( int32 x, int32 y )
    {
    }

    virtual void OnMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState )
    {
    }

    virtual void OnMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState )
    {
    }

    virtual void OnMouseScrolled( float HorizontalDelta, float VerticalDelta )
    {
    }

    virtual void OnWindowResized( const TSharedRef<CGenericWindow>& Window, uint16 Width, uint16 Height )
    {
    }

    virtual void OnWindowMoved( const TSharedRef<CGenericWindow>& Window, int16 x, int16 y )
    {
    }

    virtual void OnWindowFocusChanged( const TSharedRef<CGenericWindow>& Window, bool HasFocus )
    {
    }

    virtual void OnWindowMouseLeft( const TSharedRef<CGenericWindow>& Window )
    {
    }

    virtual void OnWindowMouseEntered( const TSharedRef<CGenericWindow>& Window )
    {
    }

    virtual void OnWindowClosed( const TSharedRef<CGenericWindow>& Window )
    {
    }

    virtual void OnApplicationExit( int32 ExitCode )
    {
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

