#pragma once
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "CanvasUser.h"
#include "ICanvasRenderer.h"

#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timestamp.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"

#include "CoreApplication/ICursor.h"
#include "CoreApplication/Generic/GenericApplication.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CCanvasApplication

class CANVAS_API CCanvasApplication : public CGenericApplicationMessageHandler
{
protected:

    friend struct TDefaultDelete<CCanvasApplication>;

    CCanvasApplication(const TSharedPtr<CGenericApplication>& InPlatformApplication);
    virtual ~CCanvasApplication();

public:
    
    DECLARE_EVENT(CExitEvent, CCanvasApplication, int32);
    CExitEvent GetExitEvent() const { return ExitEvent; }
    
    DECLARE_EVENT(CMainViewportChange, CCanvasApplication, const TSharedRef<CGenericWindow>&);
    CMainViewportChange GetMainViewportChange() const { return MainViewportChange; }

public:

    /**
     * @brief: Creates a standard main application
     */
    static bool CreateApplication();

    /**
     * @brief: Initializes the singleton from an existing application - Used for classes inheriting from CCanvasApplication
     */
    static bool CreateApplication(const TSharedPtr<CCanvasApplication>& InApplication);

    /**
     * @brief: Releases the global application instance
     */
    static void Release();

    /**
     * @return: Returns a reference to the CCanvasApplication
     */
    static FORCEINLINE CCanvasApplication& Get() { return *Instance; }

    /**
     * @return: Returns true if the CCanvasApplication is initialized
     */
    static FORCEINLINE bool IsInitialized() { return Instance.IsValid(); }


public:

    /*/////////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericApplicationMessageHandler Interface

    virtual void HandleKeyReleased(EKey KeyCode, SModifierKeyState ModierKeyState) override;
    
    virtual void HandleKeyPressed(EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState) override;
    
    virtual void HandleKeyChar(uint32 Character) override;

    virtual void HandleMouseMove(int32 x, int32 y) override;
    
    virtual void HandleMouseReleased(EMouseButton Button, SModifierKeyState ModierKeyState) override;
    
    virtual void HandleMousePressed(EMouseButton Button, SModifierKeyState ModierKeyState) override;
    
    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) override;

    virtual void HandleWindowResized(const TSharedRef<CGenericWindow>& Window, uint32 Width, uint32 Height) override;
    
    virtual void HandleWindowMoved(const TSharedRef<CGenericWindow>& Window, int32 x, int32 y) override;
    
    virtual void HandleWindowFocusChanged(const TSharedRef<CGenericWindow>& Window, bool HasFocus) override;
    
    virtual void HandleWindowMouseLeft(const TSharedRef<CGenericWindow>& Window) override;
    
    virtual void HandleWindowMouseEntered(const TSharedRef<CGenericWindow>& Window) override;
    
    virtual void HandleWindowClosed(const TSharedRef<CGenericWindow>& Window) override;

    virtual void HandleApplicationExit(int32 ExitCode) override;
    
public:

    /**
     * @return: Returns the newly created window
     */
    TSharedRef<CGenericWindow> CreateWindow();

    /**
     * @brief: Tick and update the CCanvasApplication
     */
    void Tick(CTimestamp DeltaTime);

    /**
     * @return: Returns true if the application is running
     */
    bool IsRunning() const { return bIsRunning; }
    
    /**
     * @param Cursor: Cursor to set
     */
    void SetCursor(ECursor Cursor);

    /**
     * @brief: Set the global cursor position
     */
    void SetCursorPos(const CIntVector2& Position);

    /**
     * @brief: Set the cursor position relative to a window
     */
    void SetCursorPos(const TSharedRef<CGenericWindow>& RelativeWindow, const CIntVector2& Position);

    /**
     * @return: Returns  the current global cursor position
     */
    CIntVector2 GetCursorPos() const;

    /**
     * @return: Returns the current cursor position relative to a window
     */
    CIntVector2 GetCursorPos(const TSharedRef<CGenericWindow>& RelativeWindow) const;

    /**
     * @brief: Set the visibility of the cursor
     */
    void ShowCursor(bool bIsVisible);

    /**
     * @return: Returns true if the cursor is visible
     */
    bool IsCursorVisibile() const;

    /**
     * @return: Returns true if high-precision mouse movement is supported
     */
    bool SupportsHighPrecisionMouse() const { return PlatformApplication->SupportsHighPrecisionMouse();  }

    /**
     * @brief: Enables high-precision mouse movement for a certain window
     */
    bool EnableHighPrecisionMouseForWindow(const TSharedRef<CGenericWindow>& Window)
    { 
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window); 
    }

    /**
     * @brief: Sets the window that should have keyboard focus
     */
    void SetCapture(const TSharedRef<CGenericWindow>& CaptureWindow);

    /**
     * @brief: Sets the window that should be the active window
     */
    void SetActiveWindow(const TSharedRef<CGenericWindow>& ActiveWindow);

    /**
     * @return: Returns the window that currently has the keyboard focus
     */
    TSharedRef<CGenericWindow> GetCapture() const { return PlatformApplication->GetCapture();  }

    /**
     * @return: Returns the window that is currently active
     */
    TSharedRef<CGenericWindow> GetActiveWindow() const { return PlatformApplication->GetActiveWindow(); }

    /**
     * @return: Returns the window that currently is under the cursor
     */
    TSharedRef<CGenericWindow> GetWindowUnderCursor() const { return PlatformApplication->GetActiveWindow(); }

    /**
     * @brief: Adds a InputHandler to the application, which gets processed before the main viewport
     */
    void AddInputHandler(const TSharedPtr<CInputHandler>& NewInputHandler, uint32 Priority);

    /**
     * @brief: Removes a InputHandler from the application
     */
    void RemoveInputHandler(const TSharedPtr<CInputHandler>& InputHandler);

    /**
     * @brief: Registers the main window of the application
     */
    void RegisterMainViewport(const TSharedRef<CGenericWindow>& NewMainViewport);

    /**
     * @brief:  Sets the interface renderer
     */
    void SetRenderer(const TSharedRef<ICanvasRenderer>& NewRenderer);

    /**
     * @brief: Register a window to add that should be drawn the next frame
     */
    void AddWindow(const TSharedRef<CCanvasWindow>& NewWindow);

    /**
     * @brief: Removes a window
     */
    void RemoveWindow(const TSharedRef<CCanvasWindow>& Window);

    /**
     * @brief: Draws a string in the viewport during the current frame, the strings are reset every frame
     */
    void DrawString(const String& NewString);

    /**
     * @brief: Draw all InterfaceWindows
     */
    void DrawWindows(class FRHICommandList& InCommandList);

    /**
     * @brief: Sets the platform application used to dispatch messages from the platform
     */
    void SetPlatformApplication(const TSharedPtr<CGenericApplication>& InPlatformApplication);

    /**
     * @brief: Adds a WindowMessageHandler to the application, which gets processed before the application module
     */
    void AddWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority);

    /**
     * @brief: Removes a WindowMessageHandler from the application
     */
    void RemoveWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& WindowMessageHandler);

    /**
     * @return: Returns the PlatformApplication
     */
    TSharedPtr<CGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    /**
     * @return: Returns the renderer for the Application
     */
    TSharedRef<ICanvasRenderer> GetRenderer() const { return Renderer; }

    /**
     * @return: Returns the window registered as the main viewport
     */
    TSharedRef<CGenericWindow> GetMainViewport() const { return MainViewport; }

    /**
     * @return: Returns the cursor interface
     */
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->GetCursor(); }

public:

     /** @brief: Get the number of registered users */
    uint32 GetNumUsers() const { return static_cast<uint32>(RegisteredUsers.Size()); }

     /** @brief: Register a new user to the application */
    void RegisterUser(const TSharedPtr<CCanvasUser>& NewUser) { RegisteredUsers.Push(NewUser); }

     /** @brief: Retrieve the first user */
    TSharedPtr<CCanvasUser> GetFirstUser() const
    {
        if (!RegisteredUsers.IsEmpty())
        {
            return RegisteredUsers.FirstElement();
        }
        else
        {
            return nullptr;
        }
    }

     /** @brief: Retrieve a user from user index */
    TSharedPtr<CCanvasUser> GetUserFromIndex(uint32 UserIndex) const
    {
        if (UserIndex < (uint32)RegisteredUsers.Size())
        {
            return RegisteredUsers[UserIndex];
        }
        else
        {
            return nullptr;
        }
    }

protected:

    template<typename MessageHandlerType>
    static void InsertMessageHandler( TArray<TPair<TSharedPtr<MessageHandlerType>, uint32>>& OutMessageHandlerArray
                                    , const TSharedPtr<MessageHandlerType>& NewMessageHandler
                                    , uint32 NewPriority);

    bool CreateContext();

    void HandleKeyEvent(const SKeyEvent& KeyEvent);
    
    void HandleMouseButtonEvent(const SMouseButtonEvent& MouseButtonEvent);
    
    void HandleWindowFrameMouseEvent(const SWindowFrameMouseEvent& WindowFrameMouseEvent);

    void RenderStrings();

    TSharedPtr<CGenericApplication>      PlatformApplication;

    TSharedRef<CGenericWindow>           MainViewport;
    TSharedRef<ICanvasRenderer>          Renderer;

    TArray<String>                       DebugStrings;
    TArray<TSharedRef<CCanvasWindow>>    InterfaceWindows;
    TArray<TSharedPtr<CCanvasUser>>      RegisteredUsers;

    CExitEvent                           ExitEvent;
    CMainViewportChange                  MainViewportChange;

    TArray<TPair<TSharedPtr<CInputHandler>        , uint32>> InputHandlers;
    TArray<TPair<TSharedPtr<CWindowMessageHandler>, uint32>> WindowMessageHandlers;

    // Is false when the platform application reports that the application should exit
    bool bIsRunning = true;

    struct ImGuiContext* Context = nullptr;

    static TSharedPtr<CCanvasApplication> Instance;
};

