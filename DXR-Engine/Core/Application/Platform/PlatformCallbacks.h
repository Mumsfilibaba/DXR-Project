#pragma once
#include "Core/Input/InputCodes.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class GenericWindow;
struct ModifierKeyState;

class PlatformCallbacks
{
public:
    virtual ~PlatformCallbacks() = default;

    virtual void OnKeyReleased( EKey KeyCode, const ModifierKeyState& ModierKeyState )
    {
    }

    virtual void OnKeyPressed( EKey KeyCode, bool IsRepeat, const ModifierKeyState& ModierKeyState )
    {
    }

    virtual void OnKeyTyped( uint32 Character )
    {
    }

    virtual void OnMouseMove( int32 x, int32 y )
    {
    }

    virtual void OnMouseReleased( EMouseButton Button, const ModifierKeyState& ModierKeyState )
    {
    }

    virtual void OnMousePressed( EMouseButton Button, const ModifierKeyState& ModierKeyState )
    {
    }

    virtual void OnMouseScrolled( float HorizontalDelta, float VerticalDelta )
    {
    }

    virtual void OnWindowResized( const TSharedRef<GenericWindow>& Window, uint16 Width, uint16 Height )
    {
    }

    virtual void OnWindowMoved( const TSharedRef<GenericWindow>& Window, int16 x, int16 y )
    {
    }

    virtual void OnWindowFocusChanged( const TSharedRef<GenericWindow>& Window, bool HasFocus )
    {
    }

    virtual void OnWindowMouseLeft( const TSharedRef<GenericWindow>& Window )
    {
    }

    virtual void OnWindowMouseEntered( const TSharedRef<GenericWindow>& Window )
    {
    }

    virtual void OnWindowClosed( const TSharedRef<GenericWindow>& Window )
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
