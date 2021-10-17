#pragma once
#include "Core.h"
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "ICursor.h"
#include "ApplicationUser.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Time/Timestamp.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"
#include "Core/Application/Core/CoreApplication.h"

/* Application class for the engine */
class CORE_API CApplication : public CCoreApplicationMessageHandler
{
    /* Delegate for when the application is about to exit */
    DECLARE_EVENT( CExitEvent, CApplication, int32 );
    CExitEvent ExitEvent;

public:

    /* Creates a standard main application */
    static bool Make();
    
    /* Init the singleton from an existing application - Used for classes inheriting from CApplication */
    static bool Make( const TSharedPtr<CApplication>& InApplication );

    static void Release();

    /* Retrieve the singleton application instance */
    static FORCEINLINE CApplication& Get()
    {
        return *Instance;
    }

    /* Public destructor for the TSharedPtr */
    virtual ~CApplication() = default;

    /* Creates a window */
    virtual TSharedRef<CCoreWindow> MakeWindow();

    /* Tick the application */
    virtual void Tick( CTimestamp DeltaTime );

    /* Set the current cursor type */
    virtual void SetCursor( ECursor Cursor );

    /* Set the cursor position */
    virtual void SetCursorPos( const CIntVector2& Position );

    /* Set the cursor position */
    virtual void SetCursorPos( const TSharedRef<CCoreWindow>& RelativeWindow, const CIntVector2& Position );

    /* Retrieve the current cursor position */
    virtual CIntVector2 GetCursorPos() const;

    /* Retrieve the current cursor position */
    virtual CIntVector2 GetCursorPos( const TSharedRef<CCoreWindow>& RelativeWindow ) const;

    /* Set the visibility of the cursor */
    virtual void ShowCursor( bool IsVisible );

    /* Check the visibility for the cursor */
    virtual bool IsCursorVisibile() const;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CCoreWindow>& CaptureWindow );

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CCoreWindow>& ActiveWindow );

    /* Retrieves the window that currently has the keyboard focus, can return nullptr */
    virtual TSharedRef<CCoreWindow> GetCapture() const;

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CCoreWindow> GetActiveWindow() const;

    /* Adds a InputHandler to the application, which gets processed before the game */
    virtual void AddInputHandler( CInputHandler* NewInputHandler );

    /* Removes a InputHandler from the application */
    virtual void RemoveInputHandler( CInputHandler* InputHandler );

    /* Adds a InputHandler to the application, which gets processed before the application module */
    void AddWindowMessageHandler( CWindowMessageHandler* NewWindowMessageHandler );

    void RemoveWindowMessageHandler( CWindowMessageHandler* WindowMessageHandler );

    virtual void SetPlatformApplication( const TSharedPtr<CCoreApplication>& InPlatformApplication );

    FORCEINLINE TSharedPtr<CCoreApplication> GetPlatformApplication() const
    {
        return PlatformApplication;
    }

    FORCEINLINE ICursor* GetCursor() const
    {
        return PlatformApplication->GetCursor();
    }

    FORCEINLINE bool IsRunning() const 
    {
        return Running;
    }

    FORCEINLINE CExitEvent GetExitEvent() const
    {
        return ExitEvent;
    }

    /* Get the number of registered users */
    FORCEINLINE uint32 GetNumUsers() const
    {
        return static_cast<uint32>(RegisteredUsers.Size());
    }

    /* Register a new user to the application */
    FORCEINLINE void RegisterUser( const TSharedPtr<CApplicationUser>& NewUser )
    {
        RegisteredUsers.Push( NewUser );
    }

    /* Retrieve the first user */
    FORCEINLINE TSharedPtr<CApplicationUser> GetFirstUser() const
    {
        if ( !RegisteredUsers.IsEmpty() )
        {
            return RegisteredUsers.FirstElement();
        }
        else
        {
            return nullptr;
        }
    }

    /* Retrieve a user from user index */
    FORCEINLINE TSharedPtr<CApplicationUser> GetUserFromIndex( uint32 UserIndex ) const
    {
        if ( UserIndex < (uint32)RegisteredUsers.Size() )
        {
            return RegisteredUsers[UserIndex];
        }
        else
        {
            return nullptr;
        }
    }

public:

    virtual void HandleKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState ) override;
    virtual void HandleKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState ) override;
    virtual void HandleKeyTyped( uint32 Character ) override;

    virtual void HandleMouseMove( int32 x, int32 y ) override;
    virtual void HandleMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState ) override;
    virtual void HandleMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState ) override;
    virtual void HandleMouseScrolled( float HorizontalDelta, float VerticalDelta ) override;

    virtual void HandleWindowResized( const TSharedRef<CCoreWindow>& Window, uint16 Width, uint16 Height ) override;
    virtual void HandleWindowMoved( const TSharedRef<CCoreWindow>& Window, int16 x, int16 y ) override;
    virtual void HandleWindowFocusChanged( const TSharedRef<CCoreWindow>& Window, bool HasFocus ) override;
    virtual void HandleWindowMouseLeft( const TSharedRef<CCoreWindow>& Window ) override;
    virtual void HandleWindowMouseEntered( const TSharedRef<CCoreWindow>& Window ) override;
    virtual void HandleWindowClosed( const TSharedRef<CCoreWindow>& Window ) override;

    virtual void HandleApplicationExit( int32 ExitCode ) override;

protected:

    /* Hidden constructor, use make */
    CApplication( const TSharedPtr<CCoreApplication>& InPlatformApplication );

    /* Handles key events */
    void OnKeyEvent( const SKeyEvent& KeyEvent );

    /* Handles mouse button events */
    void OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent );

    /* Handles mouse exit window or entered window events */
    void OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& WindowFrameMouseEvent );

    /* Templated insertion method */
    template<typename MessageHandlerType>
    static void InsertMessageHandler( TArray<MessageHandlerType*>& OutMessageHandlerArray, MessageHandlerType* NewMessageHandler );

    /* The native platform application */
    TSharedPtr<CCoreApplication> PlatformApplication;

    /* Input handlers in the application */
    TArray<CInputHandler*> InputHandlers;

    /* Input handlers in the application */
    TArray<CWindowMessageHandler*> WindowMessageHandlers;

    /* All registered users */
    TArray<TSharedPtr<CApplicationUser>> RegisteredUsers;

    // Is false when the platform application reports that the application should exit
    bool Running = true;
    
    static TSharedPtr<CApplication> Instance;
};

