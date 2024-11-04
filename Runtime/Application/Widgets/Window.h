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

class APPLICATION_API FWindow : public FWidget
{
public:
    struct FInitializer
    {
        FInitializer()
            : Title()
            , Size()
            , Position()
            , StyleFlags(EWindowStyleFlags::Default)
        {
        }

        FString           Title;
        FIntVector2       Size;
        FIntVector2       Position;
        EWindowStyleFlags StyleFlags;
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

    void OnWindowDestroyed();
    void OnWindowActivationChanged(bool bIsActive);
    void OnWindowMoved(const FIntVector2& InPosition);
    void OnWindowResize(const FIntVector2& InSize);

    void MoveTo(const FIntVector2& InPosition);
    void Resize(const FIntVector2& InSize);
    
    void SetSize(const FIntVector2& InSize);
    void SetPosition(const FIntVector2& InPosition);

    FIntVector2 GetSize() const;
    FIntVector2 GetPosition() const;

    uint32 GetWidth() const;
    uint32 GetHeight() const;

    TSharedPtr<FWidget> GetOverlay() const;
    TSharedPtr<FWidget> GetContent() const;

    void SetOverlay(const TSharedPtr<FWidget>& InOverlay);
    void SetContent(const TSharedPtr<FWidget>& InContent);

    void Show(bool bFocus = true);
    void Minimize();
    void Maximize();
    void Restore();

    bool IsActive() const;
    bool IsMinimized() const;
    bool IsMaximized() const;

    float GetWindowDpiScale() const;

    void SetTitle(const FString& InTitle);
    void SetStyle(EWindowStyleFlags InStyleFlags);
    void SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow);
    void SetFocus();
    void SetOpacity(float Alpha);

    TSharedRef<FGenericWindow>       GetPlatformWindow()       { return PlatformWindow; }
    TSharedRef<const FGenericWindow> GetPlatformWindow() const { return PlatformWindow; }

    const FString& GetTitle() const
    {
        return Title;
    }

    EWindowStyleFlags GetStyle() const
    {
        return StyleFlags;
    }

private:
    FString                     Title;
    FOnWindowClosed             OnWindowClosedDelegate;
    FOnWindowMoved              OnWindowMovedDelegate;
    FOnWindowResized            OnWindowResizedDelegate;
    FOnWindowActivationChanged  OnWindowActivationChangedDelegate;
    FIntVector2                 CachedPosition;
    FIntVector2                 CachedSize;
    TSharedPtr<FWidget>         Overlay;
    TSharedPtr<FWidget>         Content;
    TSharedRef<FGenericWindow>  PlatformWindow;
    EWindowStyleFlags           StyleFlags;
};
