#pragma once
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "User.h"
#include "IApplicationRenderer.h"

#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"

#include "CoreApplication/ICursor.h"
#include "CoreApplication/Generic/GenericApplication.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FApplication

class CANVAS_API FApplication 
    : public FGenericApplicationMessageHandler
{
protected:
    FApplication(const TSharedPtr<FGenericApplication>& InFPlatformApplication);

public:
    virtual ~FApplication();

    DECLARE_EVENT(FExitEvent, FApplication, int32);
    FExitEvent GetExitEvent() const { return ExitEvent; }
    
    DECLARE_EVENT(FViewportChangedEvent, FApplication, const FGenericWindowRef&);
    FViewportChangedEvent GetViewportChangedEvent() const { return ViewportChangedEvent; }

    /** @brief: Create the application instance */
    static bool Create();

    /** @brief: Releases the application instance */
    static void Release();

    /** @return: Returns true if the FApplication is initialized */
    static FORCEINLINE bool IsInitialized() { return Instance.IsValid(); }

    /** @return: Returns a reference to the FApplication */
    static FORCEINLINE FApplication& Get() { return Instance.Dereference(); }

public:
    virtual void HandleKeyReleased(EKey KeyCode, FModifierKeyState ModierKeyState)               override;
    virtual void HandleKeyPressed(EKey KeyCode, bool IsRepeat, FModifierKeyState ModierKeyState) override;
    
    virtual void HandleKeyChar(uint32 Character) override;

    virtual void HandleMouseMove(int32 x, int32 y) override;
    
    virtual void HandleMouseReleased(EMouseButton Button, FModifierKeyState ModierKeyState) override;
    virtual void HandleMousePressed(EMouseButton Button, FModifierKeyState ModierKeyState)  override;
    
    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) override;

    virtual void HandleWindowResized(const FGenericWindowRef& Window, uint32 Width, uint32 Height) override;
    virtual void HandleWindowMoved(const FGenericWindowRef& Window, int32 x, int32 y)              override;
    
    virtual void HandleWindowFocusChanged(const FGenericWindowRef& Window, bool HasFocus) override;
    
    virtual void HandleWindowMouseLeft(const FGenericWindowRef& Window)    override;
    virtual void HandleWindowMouseEntered(const FGenericWindowRef& Window) override;
    
    virtual void HandleWindowClosed(const FGenericWindowRef& Window) override;

    virtual void HandleApplicationExit(int32 ExitCode) override;
    
public:
    /** @return: Returns the newly created window */
    FGenericWindowRef CreateWindow();

    /** @brief: Tick and update the FApplication */
    void Tick(FTimespan DeltaTime);

    /** @return: Returns true if the application is running */
    bool IsRunning() const { return bIsRunning; }
    
    /** @param Cursor: Cursor to set */
    void SetCursor(ECursor Cursor);

    /** @brief: Set the global cursor position */
    void SetCursorPos(const FIntVector2& Position);

    /** @brief: Set the cursor position relative to a window */
    void SetCursorPos(const FGenericWindowRef& RelativeWindow, const FIntVector2& Position);

    /** @return: Returns  the current global cursor position */
    FIntVector2 GetCursorPos() const;

    /** @return: Returns the current cursor position relative to a window */
    FIntVector2 GetCursorPos(const FGenericWindowRef& RelativeWindow) const;

    /** @brief: Set the visibility of the cursor */
    void ShowCursor(bool bIsVisible);

    /** @return: Returns true if the cursor is visible */
    bool IsCursorVisibile() const;

    /** @return: Returns true if high-precision mouse movement is supported */
    bool SupportsHighPrecisionMouse() const { return PlatformApplication->SupportsHighPrecisionMouse(); }

    /** @brief: Enables high-precision mouse movement for a certain window */
    bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window) { return PlatformApplication->EnableHighPrecisionMouseForWindow(Window);  }

    /** @brief: Sets the window that should have keyboard focus */
    void SetCapture(const FGenericWindowRef& CaptureWindow);

    /** @brief: Sets the window that should be the active window */
    void SetActiveWindow(const FGenericWindowRef& ActiveWindow);

    /** @return: Returns the window that currently has the keyboard focus */
    FGenericWindowRef GetCapture() const { return PlatformApplication->GetCapture();  }

    /** @return: Returns the window that is currently active */
    FGenericWindowRef GetActiveWindow() const { return PlatformApplication->GetActiveWindow(); }

    /** @return: Returns the window that currently is under the cursor */
    FGenericWindowRef GetWindowUnderCursor() const { return PlatformApplication->GetActiveWindow(); }

    /** @brief: Adds a InputHandler to the application, which gets processed before the main viewport */
    void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority);

    /** @brief: Removes a InputHandler from the application */
    void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /** @brief: Registers the main window of the application */
    void RegisterMainViewport(const FGenericWindowRef& NewMainViewport);

    /** @brief:  Sets the interface renderer */
    void SetRenderer(const TSharedRef<IApplicationRenderer>& NewRenderer);

    /** @brief: Register a window to add that should be drawn the next frame */
    void AddWindow(const TSharedRef<FWindow>& NewWindow);

    /** @brief: Removes a window */
    void RemoveWindow(const TSharedRef<FWindow>& Window);

    /** @brief: Draws a string in the viewport during the current frame, the strings are reset every frame */
    void DrawString(const FString& NewString);

    /** @brief: Draw all InterfaceWindows */
    void DrawWindows(class FRHICommandList& InCommandList);

    /** @brief: Sets the platform application used to dispatch messages from the platform */
    void SetPlatformApplication(const TSharedPtr<FGenericApplication>& InFPlatformApplication);

    /** @brief: Adds a WindowMessageHandler to the application, which gets processed before the application module */
    void AddWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority);

    /** @brief: Removes a WindowMessageHandler from the application */
    void RemoveWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& WindowMessageHandler);

    /** @return: Returns the FPlatformApplication */
    TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    /** @return: Returns the renderer for the Application */
    TSharedRef<IApplicationRenderer> GetRenderer() const { return Renderer; }

    /** @return: Returns the window registered as the main viewport */
    FGenericWindowRef GetMainViewport() const { return MainViewport; }

    /** @return: Returns the cursor interface */
    TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->GetCursor(); }

public:

     /** @brief: Get the number of registered users */
    uint32 GetNumUsers() const { return static_cast<uint32>(RegisteredUsers.Size()); }

     /** @brief: Register a new user to the application */
    void RegisterUser(const TSharedPtr<FUser>& NewUser) { RegisteredUsers.Push(NewUser); }

     /** @brief: Retrieve the first user */
    TSharedPtr<FUser> GetFirstUser() const
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
    TSharedPtr<FUser> GetUserFromIndex(uint32 UserIndex) const
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
    static void InsertMessageHandler(
        TArray<TPair<TSharedPtr<MessageHandlerType>, uint32>>& OutMessageHandlerArray,
        const TSharedPtr<MessageHandlerType>& NewMessageHandler,
        uint32 NewPriority);

    bool CreateContext();

    void HandleKeyEvent(const FKeyEvent& KeyEvent);
    void HandleMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent);
    void HandleWindowFrameMouseEvent(const FWindowFrameMouseEvent& WindowFrameMouseEvent);

    void RenderStrings();

    TSharedPtr<FGenericApplication>  PlatformApplication;

    FGenericWindowRef                MainViewport;
    TSharedRef<IApplicationRenderer> Renderer;

    TArray<FString>                  DebugStrings;
    TArray<TSharedRef<FWindow>>      InterfaceWindows;
    TArray<TSharedPtr<FUser>>        RegisteredUsers;

    FExitEvent                       ExitEvent;
    FViewportChangedEvent            ViewportChangedEvent;

    TArray<TPair<TSharedPtr<FInputHandler>        , uint32>> InputHandlers;
    TArray<TPair<TSharedPtr<FWindowMessageHandler>, uint32>> WindowMessageHandlers;

    // Is false when the platform application reports that the application should exit
    bool bIsRunning = true;

    struct ImGuiContext* Context = nullptr;

    static TSharedPtr<FApplication> Instance;
};

