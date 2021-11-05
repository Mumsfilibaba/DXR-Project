#pragma once
#include "Core.h"
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "InterfaceUser.h"
#include "IInterfaceRenderer.h"

#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Time/Timestamp.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"

#include "CoreApplication/ICursor.h"
#include "CoreApplication/Interface/PlatformApplication.h"

/* Application class for the engine */
class INTERFACE_API CInterfaceApplication : public CPlatformApplicationMessageHandler
{
    /* Delegate for when the application is about to exit */
    DECLARE_EVENT( CExitEvent, CInterfaceApplication, int32 );
    CExitEvent ExitEvent;

    /* Delegate for when the application gets a new main-viewport */
    DECLARE_EVENT( CMainViewportChange, CInterfaceApplication, const TSharedRef<CPlatformWindow>& );
    CMainViewportChange MainViewportChange;

public:

    /* Creates a standard main application */
    static bool Make();

    /* Init the singleton from an existing application - Used for classes inheriting from CInterfaceApplication */
    static bool Make( const TSharedPtr<CInterfaceApplication>& InApplication );

    /* Releases the global application instance, before calling release the platform application should be set to nullptr */
    static void Release();

    /* Retrieve the singleton application instance */
    static FORCEINLINE CInterfaceApplication& Get()
    {
        return *Instance;
    }

    /* Returns true if the application has been initialized */
    static FORCEINLINE bool IsInitialized()
    {
        return Instance.IsValid();
    } 

    /* Public destructor for the TSharedPtr */
    virtual ~CInterfaceApplication() = default;

    /* Creates a window */
    TSharedRef<CPlatformWindow> MakeWindow();

    /* Tick the application */
    void Tick( CTimestamp DeltaTime );

    /* Set the current cursor type */
    void SetCursor( ECursor Cursor );

    /* Set the cursor position */
    void SetCursorPos( const CIntVector2& Position );

    /* Set the cursor position */
    void SetCursorPos( const TSharedRef<CPlatformWindow>& RelativeWindow, const CIntVector2& Position );

    /* Retrieve the current cursor position */
    CIntVector2 GetCursorPos() const;

    /* Retrieve the current cursor position */
    CIntVector2 GetCursorPos( const TSharedRef<CPlatformWindow>& RelativeWindow ) const;

    /* Set the visibility of the cursor */
    void ShowCursor( bool IsVisible );

    /* Check the visibility for the cursor */
    bool IsCursorVisibile() const;

    /* Sets the window that currently has the keyboard focus */
    void SetCapture( const TSharedRef<CPlatformWindow>& CaptureWindow );

    /* Sets the window that is currently active */
    void SetActiveWindow( const TSharedRef<CPlatformWindow>& ActiveWindow );

    /* Retrieves the window that currently has the keyboard focus, can return nullptr */
    TSharedRef<CPlatformWindow> GetCapture() const;

    /* Retrieves the window that is currently active */
    TSharedRef<CPlatformWindow> GetActiveWindow() const;

    /* Adds a InputHandler to the application, which gets processed before the game */
    void AddInputHandler( CInputHandler* NewInputHandler );

    /* Removes a InputHandler from the application */
    void RemoveInputHandler( CInputHandler* InputHandler );

    /* Registers the main window of the application */
    void RegisterMainViewport( const TSharedRef<CPlatformWindow>& NewMainViewport );

    /* Sets the UI renderer */
    void SetRenderer( const TSharedRef<IInterfaceRenderer>& NewRenderer );

    /* Register a new UI window */
    void AddWindow( const TSharedRef<IInterfaceWindow>& NewWindow );

    /* Removes a UI window */
    void RemoveWindow( const TSharedRef<IInterfaceWindow>& Window );

    /* Draws a string in the viewport during the current frame, the strings are reset every frame */
    void DrawString( const CString& NewString );

    /* Renders all the UI */
    void DrawWindows( class CRHICommandList& CommandList );

    /* Sets the platform application used to dispatch messages from the OS, should be set to nullptr before releasing the application */
    void SetPlatformApplication( const TSharedPtr<CPlatformApplication>& InPlatformApplication );

    /* Adds a InputHandler to the application, which gets processed before the application module */
    void AddWindowMessageHandler( CWindowMessageHandler* NewWindowMessageHandler );

    void RemoveWindowMessageHandler( CWindowMessageHandler* WindowMessageHandler );

    FORCEINLINE TSharedPtr<CPlatformApplication> GetPlatformApplication() const
    {
        return PlatformApplication;
    }

    FORCEINLINE TSharedRef<IInterfaceRenderer> GetRenderer() const
    {
        return Renderer;
    }

    FORCEINLINE TSharedRef<CPlatformWindow> GetMainViewport() const
    {
        return MainViewport;
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

    FORCEINLINE CMainViewportChange GetMainViewportChange() const
    {
        return MainViewportChange;
    }

    /* Get the number of registered users */
    FORCEINLINE uint32 GetNumUsers() const
    {
        return static_cast<uint32>(RegisteredUsers.Size());
    }

    /* Register a new user to the application */
    FORCEINLINE void RegisterUser( const TSharedPtr<CInterfaceUser>& NewUser )
    {
        RegisteredUsers.Push( NewUser );
    }

    /* Retrieve the first user */
    FORCEINLINE TSharedPtr<CInterfaceUser> GetFirstUser() const
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
    FORCEINLINE TSharedPtr<CInterfaceUser> GetUserFromIndex( uint32 UserIndex ) const
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

    virtual void HandleWindowResized( const TSharedRef<CPlatformWindow>& Window, uint16 Width, uint16 Height ) override;
    virtual void HandleWindowMoved( const TSharedRef<CPlatformWindow>& Window, int16 x, int16 y ) override;
    virtual void HandleWindowFocusChanged( const TSharedRef<CPlatformWindow>& Window, bool HasFocus ) override;
    virtual void HandleWindowMouseLeft( const TSharedRef<CPlatformWindow>& Window ) override;
    virtual void HandleWindowMouseEntered( const TSharedRef<CPlatformWindow>& Window ) override;
    virtual void HandleWindowClosed( const TSharedRef<CPlatformWindow>& Window ) override;

    virtual void HandleApplicationExit( int32 ExitCode ) override;

protected:

    /* Hidden constructor, use make */
    CInterfaceApplication( const TSharedPtr<CPlatformApplication>& InPlatformApplication );

    /* Handles key events */
    void OnKeyEvent( const SKeyEvent& KeyEvent );

    /* Handles mouse button events */
    void OnMouseButtonEvent( const SMouseButtonEvent& MouseButtonEvent );

    /* Handles mouse exit window or entered window events */
    void OnWindowFrameMouseEvent( const SWindowFrameMouseEvent& WindowFrameMouseEvent );

    /* Templated insertion method */
    template<typename MessageHandlerType>
    static void InsertMessageHandler( TArray<MessageHandlerType*>& OutMessageHandlerArray, MessageHandlerType* NewMessageHandler );

    /* Render all the debug strings and clear the array */
    void RenderStrings();

    /* The native platform application */
    TSharedPtr<CPlatformApplication> PlatformApplication;

    /* Renderer for the UI */
    TSharedRef<IInterfaceRenderer> Renderer;

    /* Renderer for the UI */
    TSharedRef<CPlatformWindow> MainViewport;

    /* All registered UI windows */
    TArray<TSharedRef<IInterfaceWindow>> UIWindows;

    /* Debug strings */
    TArray<CString> DebugStrings;

    /* Input handlers in the application */
    TArray<CInputHandler*> InputHandlers;

    /* Input handlers in the application */
    TArray<CWindowMessageHandler*> WindowMessageHandlers;

    /* All registered users */
    TArray<TSharedPtr<CInterfaceUser>> RegisteredUsers;

    // Is false when the platform application reports that the application should exit
    bool Running = true;

    static TSharedPtr<CInterfaceApplication> Instance;
};

