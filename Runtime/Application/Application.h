#pragma once
#include "InputHandler.h"
#include "Viewport.h"
#include "ViewportRenderer.h"
#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/ICursor.h"
#include "CoreApplication/Generic/GenericApplication.h"

class APPLICATION_API FApplication 
    : public FGenericApplicationMessageHandler
{
public:
    FApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    virtual ~FApplication();

    /** @brief - Create the application instance */
    static bool Create();

    /** @brief - Releases the application instance */
    static void Destroy();

    /** @return - Returns true if the FApplication is initialized */
    static bool IsInitialized() { return GInstance.IsValid(); }

    /** @return - Returns a reference to the FApplication */
    static FApplication& Get() { return GInstance.Dereference(); }
    
public:

    DECLARE_EVENT(FExitEvent, FApplication, int32);
    FExitEvent GetExitEvent() const { return ExitEvent; };
    
    DECLARE_EVENT(FViewportChangedEvent, FApplication, const FGenericWindowRef&);
    FViewportChangedEvent GetViewportChangedEvent() const { return ViewportChangedEvent; }

    /** @return - Returns true if the initialization is true */
    virtual bool Initialize();

    /** @return - Returns true if the initiaization of the RHI specific code succeeded. Must be called after the RHI layer is initialized */
    virtual bool InitializeRHI();

    /** @return - Returns the newly created window */
    virtual FGenericWindowRef CreateWindow();

    /** @brief - Tick and update the FApplication */
    virtual void Tick(FTimespan DeltaTime);

    /** @return - Returns true if the application is running */
    virtual bool IsRunning() const { return bIsRunning; }
    
    /** @param Cursor - Cursor to set */
    virtual void SetCursor(ECursor Cursor);

    /** @brief - Set the global cursor position */
    virtual void SetCursorPos(const FIntVector2& Position);

    /** @brief - Set the cursor position relative to a window */
    virtual void SetCursorPos(const FGenericWindowRef& RelativeWindow, const FIntVector2& Position);

    /** @return - Returns  the current global cursor position */
    virtual FIntVector2 GetCursorPos() const;

    /** @return - Returns the current cursor position relative to a window */
    virtual FIntVector2 GetCursorPos(const FGenericWindowRef& RelativeWindow) const;

    /** @brief - Set the visibility of the cursor */
    virtual void ShowCursor(bool bIsVisible);

    /** @return - Returns true if the cursor is visible */
    virtual bool IsCursorVisibile() const;

    /** @return - Returns true if high-precision mouse movement is supported */
    virtual bool SupportsHighPrecisionMouse() const
    {
        return PlatformApplication->SupportsHighPrecisionMouse();
    }

    /** @brief - Enables high-precision mouse movement for a certain window */
    virtual bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window)
    {
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window);
    }

    /** @brief - Sets the window that should have keyboard focus */
    virtual void SetCapture(const FGenericWindowRef& CaptureWindow);

    /** @brief - Sets the window that should be the active window */
    virtual void SetActiveWindow(const FGenericWindowRef& ActiveWindow);

    /** @return - Returns the window that is currently active */
    virtual FGenericWindowRef GetActiveWindow() const
    {
        return PlatformApplication->GetActiveWindow();
    }

    /** @return - Returns the window that currently is under the cursor */
    virtual FGenericWindowRef GetWindowUnderCursor() const
    {
        return PlatformApplication->GetActiveWindow();
    }

    /** @return - Returns the window that currently has the keyboard focus */
    virtual FGenericWindowRef GetCapture() const
    {
        return PlatformApplication->GetCapture();
    }

    /** @brief - Adds a InputHandler to the application, which gets processed before the main viewport */
    virtual void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority);

    /** @brief - Removes a InputHandler from the application */
    virtual void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /** @brief - Registers the main window of the application */
    virtual void RegisterMainViewport(const FGenericWindowRef& NewMainViewport);

    /** @brief - Register a window to add that should be drawn the next frame */
    virtual void AddWindow(const TSharedRef<FWindow>& NewWindow);

    /** @brief - Removes a window */
    virtual void RemoveWindow(const TSharedRef<FWindow>& Window);

    /** @brief - Draws a string in the viewport during the current frame, the strings are reset every frame */
    virtual void DrawString(const FString& NewString);

    /** @brief - Draw all InterfaceWindows */
    virtual void DrawWindows(class FRHICommandList& InCommandList);

    /** @brief - Sets the platform application used to dispatch messages from the platform */
    virtual void SetPlatformApplication(const TSharedPtr<FGenericApplication>& InFPlatformApplication);

    /** @brief - Adds a WindowMessageHandler to the application, which gets processed before the application module */
    virtual void AddWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority);

    /** @brief - Removes a WindowMessageHandler from the application */
    virtual void RemoveWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& WindowMessageHandler);

    /** @return - Returns the FPlatformApplication */
    virtual TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    /** @return - Returns the window registered as the main viewport */
    virtual FGenericWindowRef GetMainViewport() const { return MainViewport; }

    /** @return - Returns the cursor interface */
    virtual TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->Cursor; }

public:
    virtual void HandleKeyReleased(EKey KeyCode, FModifierKeyState ModierKeyState) override final;
    
    virtual void HandleKeyPressed(EKey KeyCode, bool IsRepeat, FModifierKeyState ModierKeyState) override final;
    
    virtual void HandleKeyChar(uint32 Character) override final;

    virtual void HandleMouseMove(int32 x, int32 y) override final;
    
    virtual void HandleMouseReleased(EMouseButton Button, FModifierKeyState ModierKeyState) override final;
    
    virtual void HandleMousePressed(EMouseButton Button, FModifierKeyState ModierKeyState)  override final;
    
    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) override final;

    virtual void HandleWindowResized(const FGenericWindowRef& Window, uint32 Width, uint32 Height) override final;
    
    virtual void HandleWindowMoved(const FGenericWindowRef& Window, int32 x, int32 y) override final;
    
    virtual void HandleWindowFocusChanged(const FGenericWindowRef& Window, bool HasFocus) override final;
    
    virtual void HandleWindowMouseLeft(const FGenericWindowRef& Window) override final;
    
    virtual void HandleWindowMouseEntered(const FGenericWindowRef& Window) override final;
    
    virtual void HandleWindowClosed(const FGenericWindowRef& Window) override final;

    virtual void HandleApplicationExit(int32 ExitCode) override final;

protected:
    template<typename MessageHandlerType>
    static void InsertMessageHandler(
        TArray<TPair<TSharedPtr<MessageHandlerType>, uint32>>& OutMessageHandlerArray,
        const TSharedPtr<MessageHandlerType>& NewMessageHandler,
        uint32 NewPriority);

    void HandleKeyEvent(const FKeyEvent& KeyEvent);
    void HandleMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent);
    void HandleWindowFrameMouseEvent(const FWindowFrameMouseEvent& WindowFrameMouseEvent);

    void RenderStrings();

    TSharedPtr<FGenericApplication> PlatformApplication;

    FGenericWindowRef MainViewport;
    FViewportRenderer Renderer;

    TArray<FString>             DebugStrings;
    TArray<TSharedRef<FWindow>> InterfaceWindows;
    TArray<TSharedPtr<FUser>>   RegisteredUsers;

    TArray<TPair<TSharedPtr<FInputHandler>, uint32>> InputHandlers;

    FExitEvent            ExitEvent;
    FViewportChangedEvent ViewportChangedEvent;

    
    // Is false when the platform application reports that the application should exit
    bool bIsRunning = true;

    struct ImGuiContext* Context = nullptr;

    static TSharedPtr<FApplication> GInstance;
};
