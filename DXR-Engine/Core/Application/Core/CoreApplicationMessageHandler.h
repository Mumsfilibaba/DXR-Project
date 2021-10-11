#pragma once
#include "Core/Input/InputCodes.h"
#include "Core/Application/ModifierKeyState.h"

#include "CoreWindow.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/* Interface for a listener to OS events */
class CCoreApplicationMessageHandler
{
public:

    virtual ~CCoreApplicationMessageHandler() = default;

    virtual void HandleKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState )
    {
    }

    virtual void HandleKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState )
    {
    }

    virtual void HandleKeyTyped( uint32 Character )
    {
    }

    virtual void HandleMouseMove( int32 x, int32 y )
    {
    }

    virtual void HandleMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState )
    {
    }

    virtual void HandleMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState )
    {
    }

    virtual void HandleMouseScrolled( float HorizontalDelta, float VerticalDelta )
    {
    }

    virtual void HandleWindowResized( const TSharedRef<CCoreWindow>& Window, uint16 Width, uint16 Height )
    {
    }

    virtual void HandleWindowMoved( const TSharedRef<CCoreWindow>& Window, int16 x, int16 y )
    {
    }

    virtual void HandleWindowFocusChanged( const TSharedRef<CCoreWindow>& Window, bool HasFocus )
    {
    }

    virtual void HandleWindowMouseLeft( const TSharedRef<CCoreWindow>& Window )
    {
    }

    virtual void HandleWindowMouseEntered( const TSharedRef<CCoreWindow>& Window )
    {
    }

    virtual void HandleWindowClosed( const TSharedRef<CCoreWindow>& Window )
    {
    }

    virtual void HandleApplicationExit( int32 ExitCode )
    {
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

