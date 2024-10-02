#pragma once
#include "Widget.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindow;

DECLARE_DELEGATE(FOnWindowMoved, const FIntVector2&);
DECLARE_DELEGATE(FOnWindowResized, const FIntVector2&);
DECLARE_DELEGATE(FOnWindowClosed);
DECLARE_DELEGATE(FOnWindowActivationChanged);

enum class EWindowMode : uint8
{
    None       = 0,
    Windowed   = 1,
    Borderless = 2,
    Fullscreen = 3,
};

class APPLICATION_API FWindow : public FWidget
{
public:
    struct FInitializer
    {
        FInitializer()
            : Title()
            , Size()
            , Position()
            , WindowMode(EWindowMode::Windowed)
        {
        }

        FString     Title;
        FIntVector2 Size;
        FIntVector2 Position;
        EWindowMode WindowMode;
    };

    FWindow();
    virtual ~FWindow();

    void Initialize(const FInitializer& Initializer);

    virtual void Tick() override final;
    virtual bool IsWindow() const override final;

    virtual void FindChildrenUnderCursor(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutParentWidgets) override final;

    void SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed);
    void SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved);
    void SetOnWindowResized(const FOnWindowResized& InOnWindowResized);

    void NotifyWindowDestroyed();
    void NotifyWindowActivationChanged(bool bIsActive);

    void SetScreenPosition(const FIntVector2& NewPosition);
    void SetScreenSize(const FIntVector2& NewSize);

    FIntVector2 GetScreenPosition() const;
    FIntVector2 GetScreenSize() const;

    uint32 GetWidth() const;
    uint32 GetHeight() const;

    TSharedPtr<FWidget> GetOverlay() const;
    TSharedPtr<FWidget> GetContent() const;

    void SetOverlay(const TSharedPtr<FWidget>& InOverlay);
    void SetContent(const TSharedPtr<FWidget>& InContent);

    void Minimize();
    void Maximize();
    void Restore();

    bool IsActive() const;
    bool IsMinimized() const;
    bool IsMaximized() const;

    float GetWindowDpiScale() const;

    void SetTitle(const FString& InTitle);
    void SetWindowMode(EWindowMode InWindowMode);
    void SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow);
    void SetWindowFocus();
    void SetWindowOpacity(float Alpha);

    TSharedRef<FGenericWindow>       GetPlatformWindow()       { return PlatformWindow; }
    TSharedRef<const FGenericWindow> GetPlatformWindow() const { return PlatformWindow; }

    const FString& GetTitle() const
    {
        return Title;
    }

    EWindowMode GetWindowMode() const
    {
        return WindowMode;
    }

private:
    FString                     Title;
    FOnWindowClosed             OnWindowClosed;
    FOnWindowMoved              OnWindowMoved;
    FOnWindowResized            OnWindowResized;
    FOnWindowActivationChanged  OnWindowActivationChanged;
    FIntVector2                 ScreenPosition;
    FIntVector2                 ScreenSize;
    TSharedPtr<FWidget>         Overlay;
    TSharedPtr<FWidget>         Content;
    TSharedRef<FGenericWindow>  PlatformWindow;
    EWindowMode                 WindowMode;
};