#pragma once
#include "ApplicationInterface.h"

class APPLICATION_API FStandardApplication 
    : public FApplicationInterface
{
public:
    FStandardApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    virtual ~FStandardApplication();
    
    bool CreateContext();
    
    virtual FGenericWindowRef CreateWindow() override final;

    virtual void Tick(FTimespan DeltaTime) override final;

    virtual bool IsRunning() const override final { return bIsRunning; }
    
    virtual void SetCursor(ECursor Cursor) override final;

    virtual void SetCursorPos(const FIntVector2& Position) override final;
    
    virtual void SetCursorPos(const FGenericWindowRef& RelativeWindow, const FIntVector2& Position) override final;

    virtual FIntVector2 GetCursorPos() const override final;
    
    virtual FIntVector2 GetCursorPos(const FGenericWindowRef& RelativeWindow) const override final;

    virtual void ShowCursor(bool bIsVisible) override final;

    virtual bool IsCursorVisibile() const override final;

    virtual bool SupportsHighPrecisionMouse() const override final
    {
        return PlatformApplication->SupportsHighPrecisionMouse();
    }

    virtual bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window) override final
    {
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window);
    }

    virtual void SetCapture(const FGenericWindowRef& CaptureWindow) override final;
    
    virtual void SetActiveWindow(const FGenericWindowRef& ActiveWindow) override final;

    virtual FGenericWindowRef GetActiveWindow() const override final
    {
        return PlatformApplication->GetActiveWindow();
    }
    
    virtual FGenericWindowRef GetWindowUnderCursor() const override final
    {
        return PlatformApplication->GetActiveWindow();
    }
    
    virtual FGenericWindowRef GetCapture() const override final
    {
        return PlatformApplication->GetCapture();
    }

    virtual void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority) override final;
    
    virtual void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler) override final;

    virtual void RegisterMainViewport(const FGenericWindowRef& NewMainViewport) override final;
    
    virtual FGenericWindowRef GetMainViewport() const override final { return MainViewport; }

    virtual void SetRenderer(const TSharedRef<IViewportRenderer>& NewRenderer) override final;
    
    virtual TSharedRef<IViewportRenderer> GetRenderer() const override final { return Renderer; }

    virtual void AddWindow(const TSharedRef<FWindow>& NewWindow) override final;
    
    virtual void RemoveWindow(const TSharedRef<FWindow>& Window) override final;

    virtual void DrawString(const FString& NewString) override final;
    
    virtual void DrawWindows(class FRHICommandList& InCommandList) override final;

    virtual void SetPlatformApplication(const TSharedPtr<FGenericApplication>& InFPlatformApplication) override final;
    
    virtual TSharedPtr<FGenericApplication> GetPlatformApplication() const override final { return PlatformApplication; }

    virtual void AddWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority) override final;
    
    virtual void RemoveWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& WindowMessageHandler) override final;
    
    virtual TSharedPtr<ICursor> GetCursor() const override final { return PlatformApplication->Cursor; }

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

public:
    virtual void RegisterUser(const TSharedPtr<FUser>& NewUser) override final { RegisteredUsers.Push(NewUser); }

    virtual uint32 GetNumUsers() const override final { return static_cast<uint32>(RegisteredUsers.GetSize()); }
     
    virtual TSharedPtr<FUser> GetFirstUser() const override final
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

    virtual TSharedPtr<FUser> GetUserFromIndex(uint32 UserIndex) const override final
    {
        if (UserIndex < (uint32)RegisteredUsers.GetSize())
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

    void HandleKeyEvent(const FKeyEvent& KeyEvent);
    void HandleMouseButtonEvent(const FMouseButtonEvent& MouseButtonEvent);
    void HandleWindowFrameMouseEvent(const FWindowFrameMouseEvent& WindowFrameMouseEvent);

    void RenderStrings();

    TSharedPtr<FGenericApplication>  PlatformApplication;

    FGenericWindowRef                MainViewport;
    TSharedRef<IViewportRenderer>    Renderer;

    TArray<FString>                  DebugStrings;
    TArray<TSharedRef<FWindow>>      InterfaceWindows;
    TArray<TSharedPtr<FUser>>        RegisteredUsers;

    TArray<TPair<TSharedPtr<FInputHandler>        , uint32>> InputHandlers;
    TArray<TPair<TSharedPtr<FWindowMessageHandler>, uint32>> WindowMessageHandlers;

    // Is false when the platform application reports that the application should exit
    bool bIsRunning = true;

    struct ImGuiContext* Context = nullptr;
};

