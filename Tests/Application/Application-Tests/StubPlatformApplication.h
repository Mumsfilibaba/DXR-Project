#pragma once
#include <Core/RefCountedBase.h>
#include <CoreApplication/PlatformInterface/IPlatformApplication.h>
#include <CoreApplication/PlatformInterface/IPlatformApplicationMessageHandler.h>
#include <Application/Application.h>
#include <Application/Elements/Window.h>

class FStubPlatformWindow final : public IPlatformWindow, public FRefCountedBase
{
public:
    FStubPlatformWindow()
        : Shape()
        , Title()
        , StyleFlags(EWindowStyleFlags::Default)
        , DPIScale(1.0f)
        , bAcceptsInput(true)
        , bIsDestroyed(false)
        , bIsMinimized(false)
        , bIsMaximized(false)
    {
    }

    // IRefCounted Interface Overrides
    virtual int32 AddRef() const override final { return FRefCountedBase::AddRef(); }
    virtual int32 Release() const override final { return FRefCountedBase::Release(); }
    virtual int32 GetRefCount() const override final { return FRefCountedBase::GetRefCount(); }

    // IPlatformWindow Interface Overrides
    virtual bool Initialize(const FPlatformWindowDesc& InDesc) override final
    {
        Shape         = FWindowShape(InDesc.Width, InDesc.Height, InDesc.Position.X, InDesc.Position.Y);
        Title         = InDesc.Title;
        StyleFlags    = InDesc.Style;
        bAcceptsInput = InDesc.bAcceptsInput;
        return true;
    }

    virtual void Destroy() override final { bIsDestroyed = true; }

    virtual void Show(bool) override final {}

    // The three window commands are remembered rather than ignored, so a caption button test can see one land
    virtual void Minimize() override final { bIsMinimized = true; bIsMaximized = false; }
    virtual void Maximize() override final { bIsMaximized = true; bIsMinimized = false; }
    virtual void Restore() override final { bIsMinimized = false; bIsMaximized = false; }
    virtual void ToggleFullscreen() override final {}
    virtual void SetWindowFocus() override final {}

    virtual bool IsValid() const override final { return !bIsDestroyed; }
    virtual bool IsActiveWindow() const override final { return true; }
    virtual bool IsMinimized() const override final { return bIsMinimized; }
    virtual bool IsMaximized() const override final { return bIsMaximized; }
    virtual bool IsChildWindow(const TSharedRef<IPlatformWindow>&) const override final { return false; }

    virtual void SetTitle(const String& InTitle) override final { Title = InTitle; }
    virtual void GetTitle(String& OutTitle) const override final { OutTitle = Title; }

    virtual void SetWindowPos(int32 x, int32 y) override final { Shape.Position = IntVector2(x, y); }
    virtual void SetWindowShape(const FWindowShape& InShape, bool) override final { Shape = InShape; }
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final { OutWindowShape = Shape; }

    virtual uint32 GetWidth() const override final { return Shape.Width; }
    virtual uint32 GetHeight() const override final { return Shape.Height; }

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final
    {
        OutWidth  = Shape.Width;
        OutHeight = Shape.Height;
    }

    virtual float GetWindowDPIScale() const override final { return DPIScale; }

    virtual void SetStyle(EWindowStyleFlags InStyle) override final { StyleFlags = InStyle; }
    virtual EWindowStyleFlags GetStyle() const override final { return StyleFlags; }

    virtual FWindowTitleBarMetrics GetTitleBarMetrics() const override final { return TitleBarMetrics; }
    virtual void SetTitleBarRegions(const FWindowTitleBarRegions& InRegions) override final { TitleBarRegions = InRegions; }

    virtual void SetWindowOpacity(float) override final {}
    virtual void SetAcceptsInput(bool bInAcceptsInput) override final { bAcceptsInput = bInAcceptsInput; }
    virtual bool GetAcceptsInput() const override final { return bAcceptsInput; }

    virtual void SetPlatformHandle(void*) override final {}
    virtual void* GetPlatformHandle() const override final { return nullptr; }

    void SetWindowDPIScale(float InDPIScale) { DPIScale = InDPIScale; }

    /** @brief Poses as a platform whose chrome has the given height and insets, so a title bar test can be either OS. */
    void SetTitleBarMetrics(const FWindowTitleBarMetrics& InMetrics) { TitleBarMetrics = InMetrics; }

    /** @brief What the last SetTitleBarRegions call was handed, which is the whole platform contract a title bar has to meet. */
    const FWindowTitleBarRegions& GetTitleBarRegions() const { return TitleBarRegions; }

private:
    FWindowShape           Shape;
    String                 Title;
    EWindowStyleFlags      StyleFlags;
    FWindowTitleBarMetrics TitleBarMetrics;
    FWindowTitleBarRegions TitleBarRegions;
    float                  DPIScale;
    bool                   bAcceptsInput;
    bool                   bIsDestroyed;
    bool                   bIsMinimized;
    bool                   bIsMaximized;
};

class FStubPlatformCursor final : public IPlatformCursor
{
public:
    FStubPlatformCursor()
        : Position()
        , Cursor(ECursor::Arrow)
        , bIsVisible(true)
    {
    }

    // IPlatformCursor Interface Overrides
    virtual void SetCursor(ECursor InCursor) override final { Cursor = InCursor; }
    virtual ECursor GetCursor() const override final { return Cursor; }

    virtual void SetPosition(int32 x, int32 y) override final { Position = IntVector2(x, y); }
    virtual IntVector2 GetPosition() const override final { return Position; }

    virtual void SetVisibility(bool bInIsVisible) override final { bIsVisible = bInIsVisible; }
    virtual bool IsVisible() const override final { return bIsVisible; }

private:
    IntVector2 Position;
    ECursor    Cursor;
    bool       bIsVisible;
};

class FStubPlatformApplication final : public IPlatformApplication
{
public:
    FStubPlatformApplication()
        : MessageHandler(nullptr)
        , Cursor(MakeSharedPtr<FStubPlatformCursor>())
        , ActiveWindow(nullptr)
        , CaptureWindow(nullptr)
        , WindowUnderCursor(nullptr)
    {
    }

    // IPlatformApplication Interface Overrides
    virtual TSharedRef<IPlatformWindow> CreateWindow() override final
    {
        // The new window starts at one reference, which the returned reference takes over
        return TSharedRef<IPlatformWindow>(new FStubPlatformWindow());
    }

    virtual void Tick(float) override final {}
    virtual void ProcessEvents() override final {}
    virtual void ProcessDeferredEvents() override final {}
    virtual void UpdateInputDevices() override final {}
    virtual IPlatformInputDevice* GetInputDevice() override final { return nullptr; }

    virtual bool SupportsHighPrecisionMouse() const override final { return false; }
    virtual bool SetHighPrecisionMouseMode(const TSharedRef<IPlatformWindow>&, EHighPrecisionMouseMode) override final { return false; }
    virtual bool ConfineCursorToRect(const TSharedRef<IPlatformWindow>&, const IntVector2&, const IntVector2&) override final { return false; }
    virtual void ReleaseCursorConfinement() override final {}

    virtual FModifierKeyState GetModifierKeyState() const override final { return FModifierKeyState(); }

    virtual void SetActiveWindow(const TSharedRef<IPlatformWindow>& InWindow) override final { ActiveWindow = InWindow; }
    virtual TSharedRef<IPlatformWindow> GetActiveWindow() const override final { return ActiveWindow; }
    virtual void SetCapture(const TSharedRef<IPlatformWindow>& InWindow) override final { CaptureWindow = InWindow; }
    virtual TSharedRef<IPlatformWindow> GetCapture() const override final { return CaptureWindow; }
    virtual TSharedRef<IPlatformWindow> GetWindowUnderCursor() const override final { return WindowUnderCursor; }

    virtual void QueryMonitorInfo(TArray<FMonitorInfo>& OutMonitorInfo) const override final
    {
        FMonitorInfo MonitorInfo;
        MonitorInfo.DeviceName     = "Stub";
        MonitorInfo.MainSize       = IntVector2(1920, 1080);
        MonitorInfo.WorkSize       = IntVector2(1920, 1080);
        MonitorInfo.DisplayDPI     = 96;
        MonitorInfo.DisplayScaling = 1.0f;
        MonitorInfo.bIsPrimary     = true;

        OutMonitorInfo.Clear();
        OutMonitorInfo.Add(MonitorInfo);
    }

    virtual void SetMessageHandler(const TSharedPtr<IPlatformApplicationMessageHandler>& InMessageHandler) override final
    {
        MessageHandler = InMessageHandler;
    }

    virtual TSharedPtr<IPlatformApplicationMessageHandler> GetMessageHandler() const override final { return MessageHandler; }
    virtual TSharedPtr<IPlatformCursor> GetCursor() const override final { return Cursor; }

    /**
     * @brief Puts the cursor over a window, which is what the routing reads to resolve a screen point onto elements.
     *
     * @param InWindow The window the cursor is over, or null for none.
     */
    void SetWindowUnderCursor(const TSharedRef<IPlatformWindow>& InWindow) { WindowUnderCursor = InWindow; }

private:
    TSharedPtr<IPlatformApplicationMessageHandler> MessageHandler;
    TSharedPtr<FStubPlatformCursor>                Cursor;
    TSharedRef<IPlatformWindow>                    ActiveWindow;
    TSharedRef<IPlatformWindow>                    CaptureWindow;
    TSharedRef<IPlatformWindow>                    WindowUnderCursor;
};

inline TSharedPtr<FApplication> CreateStubApplication()
{
    TSharedPtr<FStubPlatformApplication> PlatformApplication = MakeSharedPtr<FStubPlatformApplication>();

    TSharedPtr<FApplication> NewApplication = MakeSharedPtr<FApplication>(PlatformApplication);
    PlatformApplication->SetMessageHandler(NewApplication);
    return NewApplication;
}

inline TSharedPtr<FWindow> CreateStubWindow(const TSharedPtr<FApplication>& Application, const IntVector2& Size)
{
    FWindow::FDesc Desc;
    Desc.Title = "Stub";
    Desc.Size  = Size;

    TSharedPtr<FWindow> Window = FWindow::Create(Desc);
    Application->CreateWindow(Window);
    return Window;
}

class FScopedStubApplication
{
public:
    FScopedStubApplication()
        : PlatformApplication(MakeSharedPtr<FStubPlatformApplication>())
    {
        FApplication::Initialize(PlatformApplication);
    }

    ~FScopedStubApplication()
    {
        FApplication::Release();
    }

    FScopedStubApplication(const FScopedStubApplication&) = delete;
    FScopedStubApplication& operator=(const FScopedStubApplication&) = delete;

    FApplication& GetApplication() const
    {
        return FApplication::Get();
    }

    const TSharedPtr<FStubPlatformApplication>& GetPlatformApplication() const
    {
        return PlatformApplication;
    }

    /**
     * @brief Creates a window, registers it and puts the cursor over it, which is what routing needs.
     *
     * @param Size       The client size of the window.
     * @param Position   Where the window sits on the desktop.
     * @param StyleFlags The style to create it with, which a title bar test needs a say in.
     * @return The new window.
     */
    TSharedPtr<FWindow> CreateWindow(const IntVector2& Size, const IntVector2& Position = IntVector2(0, 0), EWindowStyleFlags StyleFlags = EWindowStyleFlags::Default)
    {
        FWindow::FDesc Desc;
        Desc.Title      = "Stub";
        Desc.Size       = Size;
        Desc.Position   = Position;
        Desc.StyleFlags = StyleFlags;

        TSharedPtr<FWindow> Window = FWindow::Create(Desc);
        GetApplication().CreateWindow(Window);

        PlatformApplication->SetWindowUnderCursor(Window->GetPlatformWindow());
        return Window;
    }

    /**
     * @brief Moves the cursor and delivers the move, so hover runs through the real enter and leave path.
     *
     * @param ScreenPosition Where the cursor moved to, in screen coordinates.
     */
    void MoveCursor(const IntVector2& ScreenPosition)
    {
        PlatformApplication->GetCursor()->SetPosition(ScreenPosition.X, ScreenPosition.Y);
        GetApplication().OnMouseMove(ScreenPosition.X, ScreenPosition.Y);
    }

    /**
     * @brief Presses a mouse button where the cursor already is.
     *
     * @param Window The window the press belongs to.
     * @param Button The button that went down.
     */
    void PressMouseButton(const TSharedPtr<FWindow>& Window, EMouseButtonName::Type Button = EMouseButtonName::Left)
    {
        GetApplication().OnMouseButtonDown(Window->GetPlatformWindow(), Button, FModifierKeyState());
    }

    /**
     * @brief Releases a mouse button where the cursor already is.
     *
     * @param Button The button that came up.
     */
    void ReleaseMouseButton(EMouseButtonName::Type Button = EMouseButtonName::Left)
    {
        GetApplication().OnMouseButtonUp(Button, FModifierKeyState());
    }

private:
    TSharedPtr<FStubPlatformApplication> PlatformApplication;
};
