#pragma once
#include "Core.h"
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "ICursor.h"
#include "ApplicationUser.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Time/Timestamp.h"
#include "Core/Application/Generic/GenericApplication.h"
#include "Core/Application/Generic/GenericApplicationMessageListener.h"
#include "Core/Math/IntPoint2.h"
#include "Core/Delegates/Event.h"

/* Application class for the engine */
class CMainApplication : public CGenericApplicationMessageListener
{
public:

    /* Public destructor for the TSharedPtr */
    virtual ~CMainApplication();

    /* Creates a standard main application */
    static FORCEINLINE TSharedPtr<CMainApplication> Make( const TSharedPtr<CGenericApplication>& InPlatformApplication )
    {
        ApplicationInstance = TSharedPtr<CMainApplication>( DBG_NEW CMainApplication( InPlatformApplication ) );
        InPlatformApplication->SetMessageListener( ApplicationInstance );
        return ApplicationInstance;
    }

    /* Init the singleton from an existing application - Used for classes inheriting from CMainApplication */
    static FORCEINLINE TSharedPtr<CMainApplication> Make( const TSharedPtr<CMainApplication>& InApplication )
    {
        ApplicationInstance = InApplication;
        return ApplicationInstance;
    }

    static FORCEINLINE void Release()
    {
        ApplicationInstance->SetPlatformApplication( nullptr );
        ApplicationInstance.Reset();
    }

    /* Retrieve the singleton application instance */
    static FORCEINLINE CMainApplication& Get()
    {
        return *ApplicationInstance;
    }

    /* Delegate for when the application is about to exit */
    DECLARE_EVENT( CApplicationExitEvent, CMainApplication, int32 );
    CApplicationExitEvent ApplicationExitEvent;

    /* Creates a window */
    virtual TSharedRef<CGenericWindow> MakeWindow();

    /* Tick the application */
    virtual void Tick( CTimestamp DeltaTime );

    /* Set the current cursor type */
    virtual void SetCursor( ECursor Cursor );

    /* Set the cursor position */
    virtual void SetCursorPosition( const CIntPoint2& Position );

    /* Set the cursor position */
    virtual void SetCursorPosition( const TSharedRef<CGenericWindow>& RelativeWindow, const CIntPoint2& Position );

    /* Retrieve the current cursor position */
    virtual CIntPoint2 GetCursorPosition() const;

    /* Retrieve the current cursor position */
    virtual CIntPoint2 GetCursorPosition( const TSharedRef<CGenericWindow>& RelativeWindow ) const;

    /* Set the visibility of the cursor */
    virtual void SetCursorVisibility( bool IsVisible );

    /* Check the visibility for the cursor */
    virtual bool IsCursorVisibile() const;

    /* Sets the window that currently has the keyboard focus */
    virtual void SetCapture( const TSharedRef<CGenericWindow>& CaptureWindow );

    /* Sets the window that is currently active */
    virtual void SetActiveWindow( const TSharedRef<CGenericWindow>& ActiveWindow );

    /* Retrieves the window that currently has the keyboard focus, can return nullptr */
    virtual TSharedRef<CGenericWindow> GetCapture() const;

    /* Retrieves the window that is currently active */
    virtual TSharedRef<CGenericWindow> GetActiveWindow() const;

    /* Adds a InputHandler to the application, which gets processed before the game */
    virtual void AddInputHandler( CInputHandler* NewInputHandler );

    /* Removes a InputHandler from the application */
    virtual void RemoveInputHandler( CInputHandler* InputHandler );

    /* Adds a InputHandler to the application, which gets processed before the game */
    virtual void AddWindowMessageHandler( CWindowMessageHandler* NewWindowMessageHandler );

    /* Removes a InputHandler from the application */
    virtual void RemoveWindowMessageHandler( CWindowMessageHandler* WindowMessageHandler );

    /* Set the platform application */
    virtual void SetPlatformApplication( const TSharedPtr<CGenericApplication>& InPlatformApplication );

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

public: // CGenericApplicationMessageListener interface

    /* Key Events */

    virtual void OnKeyReleased( EKey KeyCode, SModifierKeyState ModierKeyState ) override;

    virtual void OnKeyPressed( EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState ) override;

    virtual void OnKeyTyped( uint32 Character ) override;

    /* Mouse Events */

    virtual void OnMouseMove( int32 x, int32 y ) override;

    virtual void OnMouseReleased( EMouseButton Button, SModifierKeyState ModierKeyState ) override;

    virtual void OnMousePressed( EMouseButton Button, SModifierKeyState ModierKeyState ) override;

    virtual void OnMouseScrolled( float HorizontalDelta, float VerticalDelta ) override;

    /* Window Events */

    virtual void OnWindowResized( const TSharedRef<CGenericWindow>& Window, uint16 Width, uint16 Height ) override;

    virtual void OnWindowMoved( const TSharedRef<CGenericWindow>& Window, int16 x, int16 y ) override;

    virtual void OnWindowFocusChanged( const TSharedRef<CGenericWindow>& Window, bool HasFocus ) override;

    virtual void OnWindowMouseLeft( const TSharedRef<CGenericWindow>& Window ) override;

    virtual void OnWindowMouseEntered( const TSharedRef<CGenericWindow>& Window ) override;

    virtual void OnWindowClosed( const TSharedRef<CGenericWindow>& Window ) override;

    /* Other Application Events */
    virtual void OnApplicationExit( int32 ExitCode ) override;

public:

    /* Retrieve the platform application */
    FORCEINLINE TSharedPtr<CGenericApplication> GetPlatformApplication() const
    {
        return PlatformApplication;
    }

    /* Retrieve the cursor interface */
    FORCEINLINE ICursor* GetCursor() const
    {
        return PlatformApplication->GetCursor();
    }

    /* Get the number of registered users */
    FORCEINLINE uint32 GetNumUsers() const
    {
        return static_cast<uint32>( RegisteredUsers.Size() );
    }

protected:

    CMainApplication( const TSharedPtr<CGenericApplication>& InPlatformApplication );

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
    TSharedPtr<CGenericApplication> PlatformApplication;

    /* Input handlers in the application */
    TArray<CInputHandler*> InputHandlers;

    /* Input handlers in the application */
    TArray<CWindowMessageHandler*> WindowMessageHandlers;

    /* All registered users */
    TArray<TSharedPtr<CApplicationUser>> RegisteredUsers;

    static TSharedPtr<CMainApplication> ApplicationInstance;
};
