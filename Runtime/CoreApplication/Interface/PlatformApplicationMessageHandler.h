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

/* Interface for a listener to platform messages */
class CPlatformApplicationMessageHandler
{
public:

    virtual ~CPlatformApplicationMessageHandler() = default;

    virtual void HandleKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState ) {}

    virtual void HandleKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState ) {}

    virtual void HandleKeyTyped( uint32 Character ) {}

    virtual void HandleMouseMove( int32 x, int32 y ) {}

    virtual void HandleMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState ) {}

    virtual void HandleMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState ) {}

    virtual void HandleMouseScrolled( float HorizontalDelta, float VerticalDelta ) {}

    virtual void HandleRawMouseInput( const TSharedRef<CPlatformWindow>& Window, int32 x, uint32 y ) {}

    virtual void HandleWindowResized( const TSharedRef<CPlatformWindow>& Window, uint32 Width, uint32 Height ) {}

    virtual void HandleWindowMoved( const TSharedRef<CPlatformWindow>& Window, int32 x, int32 y ) {}

    virtual void HandleWindowFocusChanged( const TSharedRef<CPlatformWindow>& Window, bool HasFocus ) {}

    virtual void HandleWindowMouseLeft( const TSharedRef<CPlatformWindow>& Window ) {}

    virtual void HandleWindowMouseEntered( const TSharedRef<CPlatformWindow>& Window ) {}

    virtual void HandleWindowClosed( const TSharedRef<CPlatformWindow>& Window ) {}

    virtual void HandleApplicationExit( int32 ExitCode ) {}
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
