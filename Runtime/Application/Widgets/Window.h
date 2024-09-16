#pragma once
#include "Widget.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindow;

DECLARE_DELEGATE(FOnWindowMoved, const FIntVector2&);
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
            , ParentWindow()
        {
        }

        FString             Title;
        FIntVector2         Size;
        FIntVector2         Position;
        EWindowMode         WindowMode;
        TSharedPtr<FWindow> ParentWindow;
    };

    FWindow();
    virtual ~FWindow();

    // Construct the actual window
    void Initialize(const FInitializer& Initializer);

    // Enables us to find out that this is a window
    virtual bool IsWindow() const override final { return true; }

    // Set a delegate that is called when the window gets closed
    void SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed);

    // Set a delegate that is called when the window gets moved
    void SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved);

    // Called when the window is destroyed
    void NotifyWindowDestroyed();

    // Called when window activation changes
    void NotifyWindowActivationChanged(bool bIsActive);

    // Called when the window is moved
    void SetScreenPosition(const FIntVector2& NewPosition);

    // Called when the window is resized
    void SetScreenSize(const FIntVector2& NewSize);

    // Set the title of the window
    void SetTitle(const FString& InTitle);

    // Set the mode of the window
    void SetWindowMode(EWindowMode InWindowMode);

    // Retrieve the DPI Scale of the window
    float GetWindowDpiScale() const;

    // Retrieve the width of the window
    uint32 GetWidth() const;

    // Retrieve the height of the window
    uint32 GetHeight() const;

    // Retrieve the Window overlay
    TSharedPtr<FWidget> GetOverlay() const;

    // Retrieve the window content
    TSharedPtr<FWidget> GetContent() const;

    // Set the platform window
    void SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow);

    // Set overlay widget
    void SetOverlay(const TSharedPtr<FWidget>& InOverlay);

    // Set actual window content
    void SetContent(const TSharedPtr<FWidget>& InContent);

    // Minimize the window
    void Minimize();

    // Maximize the window
    void Maximize();

    // Restore the window from minimized or maximized state
    void Restore();

    // Return true if this is the active window
    bool IsActive() const;

    // Set the window position
    void SetPosition(int32 x, int32 y);

    // Retrieve the window position
    FIntVector2 GetPosition() const;

    // Return true if the window's state is currently minimized
    bool IsMinimized() const;

    // Return true if the window's state is currently maximized
    bool IsMaximized() const;

    // Makes a window the parent window
    void AddParentWindow(const TSharedPtr<FWindow>& InParentWindow);

    // Returns true if the specified window is a parent window to this window
    bool IsChildWindow(const TSharedPtr<FWindow>& ParentWindow) const;

    // Set the focus of the window
    void SetWindowFocus();

    // Set the window opacity
    void SetWindowOpacity(float Alpha);

    // Set the shape of the window
    void SetWidth(uint32 InWidth);

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
    FOnWindowActivationChanged  OnWindowActivationChanged;
    FIntVector2                 ScreenPosition;
    FIntVector2                 ScreenSize;
    TSharedPtr<FWidget>         Overlay;
    TSharedPtr<FWidget>         Content;
    TSharedRef<FGenericWindow>  PlatformWindow;
    EWindowMode                 WindowMode;

    TWeakPtr<FWindow>           ParentWindow;
    TArray<TSharedPtr<FWindow>> ChildWindows;
};