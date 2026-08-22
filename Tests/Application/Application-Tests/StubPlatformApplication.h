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
    virtual void Minimize() override final {}
    virtual void Maximize() override final {}
    virtual void Restore() override final {}
    virtual void ToggleFullscreen() override final {}
    virtual void SetWindowFocus() override final {}

    virtual bool IsValid() const override final { return !bIsDestroyed; }
    virtual bool IsActiveWindow() const override final { return true; }
    virtual bool IsMinimized() const override final { return false; }
    virtual bool IsMaximized() const override final { return false; }
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

    virtual FWindowTitleBarMetrics GetTitleBarMetrics() const override final { return FWindowTitleBarMetrics(); }
    virtual void SetTitleBarRegions(const FWindowTitleBarRegions&) override final {}

    virtual void SetWindowOpacity(float) override final {}
    virtual void SetAcceptsInput(bool bInAcceptsInput) override final { bAcceptsInput = bInAcceptsInput; }
    virtual bool GetAcceptsInput() const override final { return bAcceptsInput; }

    virtual void SetPlatformHandle(void*) override final {}
    virtual void* GetPlatformHandle() const override final { return nullptr; }

    void SetWindowDPIScale(float InDPIScale) { DPIScale = InDPIScale; }

private:
    FWindowShape      Shape;
    String            Title;
    EWindowStyleFlags StyleFlags;
    float             DPIScale;
    bool              bAcceptsInput;
    bool              bIsDestroyed;
};

class FStubPlatformApplication final : public IPlatformApplication
{
public:
    FStubPlatformApplication()
        : MessageHandler(nullptr)
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

    virtual void SetActiveWindow(const TSharedRef<IPlatformWindow>&) override final {}
    virtual TSharedRef<IPlatformWindow> GetActiveWindow() const override final { return nullptr; }
    virtual void SetCapture(const TSharedRef<IPlatformWindow>&) override final {}
    virtual TSharedRef<IPlatformWindow> GetCapture() const override final { return nullptr; }
    virtual TSharedRef<IPlatformWindow> GetWindowUnderCursor() const override final { return nullptr; }

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
    virtual TSharedPtr<IPlatformCursor> GetCursor() const override final { return nullptr; }

private:
    TSharedPtr<IPlatformApplicationMessageHandler> MessageHandler;
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
