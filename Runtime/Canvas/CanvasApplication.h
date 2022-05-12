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

    /**
     * @brief: Creates a standard main application 
     * 
     * @return: Returns true if the creation was successful
     */
    static bool CreateApplication();

    /**
     * @brief: Initializes the singleton from an existing application - Used for classes inheriting from CCanvasApplication
     * 
     * @param InApplication: Existing application
     * @return: Returns true if the initialization was successful
     */
    static bool CreateApplication(const TSharedPtr<CCanvasApplication>& InApplication);

    /** 
     * @brief: Releases the global application instance 
     */
    static void Release();

    /** 
     * @return: Returns a reference to the CCanvasApplication 
     */
    static FORCEINLINE CCanvasApplication& Get() 
    { 
        return *Instance; 
    }

    /**
     * @return: Returns true if the InterfaceApplication is initialized 
     */
    static FORCEINLINE bool IsInitialized() 
    { 
        return Instance.IsValid();
    }

public:

    /**
     * @brief: Delegate for when the application is about to exit 
     */
    DECLARE_EVENT(CExitEvent, CCanvasApplication, int32);
    CExitEvent GetExitEvent() const { return ExitEvent; }

    /** 
     * @brief: Delegate for when the application gets a new main-viewport 
     */
    DECLARE_EVENT(CMainViewportChange, CCanvasApplication, const TSharedRef<CGenericWindow>&);
    CMainViewportChange GetMainViewportChange() const { return MainViewportChange; }

public:

    /**
     * @brief: Creates a new window 
     * 
     * @return: Returns the newly created window
     */
    TSharedRef<CGenericWindow> MakeWindow();

    /**
     * @brief: Tick and update the InterfaceApplication
     * 
     * @param DeltaTime: Time between this and the previous update
     */
    void Tick(CTimestamp DeltaTime);

    /**
     * @brief: Check if the application is currently running
     *
     * @return: Returns true if the application is running
     */
    bool IsRunning() const { return bIsRunning; }
    
    /**
     * @brief: Set the current cursor type 
     * 
     * @param Cursor: Cursor to set
     */
    void SetCursor(ECursor Cursor);

    /**
     * @brief: Set the global cursor position
     * 
     * @param Position: New position to the cursor
     */
    void SetCursorPos(const CIntVector2& Position);

    /**
     * @brief: Set the cursor position relative to a window
     *
     * @param RelativeWindow: Window that the position should be relative to
     * @param Position: New position to the cursor
     */
    void SetCursorPos(const TSharedRef<CGenericWindow>& RelativeWindow, const CIntVector2& Position);

    /**
     * @brief: Retrieve the current global cursor position
     *
     * @return: Returns the cursor position
     */
    CIntVector2 GetCursorPos() const;

    /**
     * @brief: Retrieve the current cursor position relative to a window
     *
     * @param RelativeWindow: Window that the position should be relative to
     * @return: Returns the global cursor position
     */
    CIntVector2 GetCursorPos(const TSharedRef<CGenericWindow>& RelativeWindow) const;

    /**
     * @brief: Set the visibility of the cursor 
     * 
     * @param bIsVisible: Should the cursor be visible or not
     */
    void ShowCursor(bool bIsVisible);

    /**
     * @brief: Check the cursor visibility
     *
     * @return: Returns true if the cursor is visible
     */
    bool IsCursorVisibile() const;

    /**
     * @brief: Check if the PlatformApplication supports high-precision mouse movement
     *
     * @return: Returns true if high-precision mouse movement is supported
     */
    bool SupportsHighPrecisionMouse() const { return PlatformApplication->SupportsHighPrecisionMouse();  }

    /**
     * @brief: Enables high-precision mouse movement for a certain window
     *
     * @param Window: Window to enable high-precision mouse movement for
     * @return: Returns true if the window now uses high-precision mouse movement
     */
    bool EnableHighPrecisionMouseForWindow(const TSharedRef<CGenericWindow>& Window)
    { 
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window); 
    }

    /**
     * @brief: Sets the window that should have keyboard focus 
     * 
     * @param CaptureWindow: Window that should have keyboard focus
     */
    void SetCapture(const TSharedRef<CGenericWindow>& CaptureWindow);

    /**
     * @brief: Sets the window that should be the active window
     *
     * @param ActiveWindow: Window that should be the active window
     */
    void SetActiveWindow(const TSharedRef<CGenericWindow>& ActiveWindow);

    /**
     * @brief: Retrieves the window that currently has the keyboard focus, can return nullptr
     *
     * @return: Returns the window that currently has the keyboard focus
     */
    TSharedRef<CGenericWindow> GetCapture() const { return PlatformApplication->GetCapture();  }

    /**
      * @brief: Retrieves the window that is currently active
      *
      * @return: Returns the currently active window
      */
    TSharedRef<CGenericWindow> GetActiveWindow() const { return PlatformApplication->GetActiveWindow(); }

    /**
     * @brief: Retrieves the window under the cursor
     *
     * @return: Returns the window that currently is under the cursor
     */
    TSharedRef<CGenericWindow> GetWindowUnderCursor() const { return PlatformApplication->GetActiveWindow(); }

    /**
     * @brief: Adds a InputHandler to the application, which gets processed before the main viewport
     *
     * @param NewInputHandler: InputHandler to add
     * @param Priority: Priority of the InputHandler
     */
    void AddInputHandler(const TSharedPtr<CInputHandler>& NewInputHandler, uint32 Priority);

    /**
     * @brief: Removes a InputHandler from the application 
     * 
     * @param InputHandler: InputHandler to remove
     */
    void RemoveInputHandler(const TSharedPtr<CInputHandler>& InputHandler);

    /**
     * @brief: Registers the main window of the application 
     * 
     * @param NewMainViewport: Viewport to be the new main-viewport
     */
    void RegisterMainViewport(const TSharedRef<CGenericWindow>& NewMainViewport);

    /**
     * @brief:  Sets the interface renderer 
     * 
     * @param NewRenderer: New renderer for the InterfaceApplication
     */
    void SetRenderer(const TSharedRef<ICanvasRenderer>& NewRenderer);

    /**
     * @brief: Register a interface window 
     * 
     * @param NewWindow: Window to add that should be drawn the next frame
     */
    void AddWindow(const TSharedRef<CCanvasWindow>& NewWindow);

    /**
     * @brief: Removes a interface window
     *
     * @param NewWindow: Window to remove
     */
    void RemoveWindow(const TSharedRef<CCanvasWindow>& Window);

    /**
     * @brief: Draws a string in the viewport during the current frame, the strings are reset every frame 
     * 
     * @param NewString: String to start drawing
     */
    void DrawString(const String& NewString);

    /**
     * @brief: Draw all InterfaceWindows
     * 
     * @param InCommandList: CommandList to submit draw-commands into
     */
    void DrawWindows(class CRHICommandList& InCommandList);

    /**
     * @brief: Sets the platform application used to dispatch messages from the platform
     * 
     * @param InPlatformApplication: New PlatformApplication
     */
    void SetPlatformApplication(const TSharedPtr<CGenericApplication>& InPlatformApplication);

    /**
     * @brief: Adds a WindowMessageHandler to the application, which gets processed before the application module 
     * 
     * @param NewWindowMessageHandler: WindowMessageHandler to add
     * @param Priority: Priority of the InputHandler
     */
    void AddWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority);

    /**
     * @brief: Removes a WindowMessageHandler from the application
     *
     * @param WindowMessageHandler: WindowMessageHandler to remove
     */
    void RemoveWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& WindowMessageHandler);

    /**
     * @brief: Retrieve the native application
     *
     * @return: Returns the PlatformApplication
     */
    TSharedPtr<CGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    /**
     * @brief: Retrieve the renderer for the Application
     *
     * @return: Returns the renderer
     */
    TSharedRef<ICanvasRenderer> GetRenderer() const { return Renderer; }

    /**
     * @brief:  Retrieve the window registered as the main viewport
     *
     * @return: Returns the main viewport
     */
    TSharedRef<CGenericWindow> GetMainViewport() const { return MainViewport; }

    /**
     * @brief: Retrieve the cursor interface
     *
     * @return: Returns the cursor interface
     */
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->GetCursor(); }

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

