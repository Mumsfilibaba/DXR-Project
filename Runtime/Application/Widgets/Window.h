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

    virtual void FindChildrenContainingPoint(const FIntVector2& Point, FWidgetPath& OutParentWidgets) override final;

    // Sets a delegate that is called when the window is notified about being closed.
    void SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed);

    // Sets a delegate that is called when the window is notified about being moved.
    void SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved);

    // Sets a delegate that is called when the window is notified about being resized.
    void SetOnWindowResized(const FOnWindowResized& InOnWindowResized);

    // This function is called from the FApplicationInstance when the PlatformWindow is destroyed.
    void OnWindowDestroyed();

    // This function is called from the FApplicationInstance when the PlatformWindow activation 
    // is changed. This meaning that the user switches window which recieves focus or if the 
    // whole application looses focus. The behavior changes somewhat depending on the platoform 
    // but generally the function is called the window looses or gains focus.
    void OnWindowActivationChanged(bool bIsActive);

    // This function is called from the FApplicationInstance when the PlatformWindow is being resized.
    void OnWindowResize(const FIntVector2& InSize);

    // This function is called from the FApplicationInstance when the PlatformWindow is moved.
    void OnWindowMoved(const FIntVector2& InPosition);

    // Resizes the window with a new size. This function also sets the PlatformWindow's size and
    // changes the current cached size.
    void Resize(const FIntVector2& InSize);

    // Move the window to a new position. This function also sets the PlatformWindow's position and
    // changes the current cached position.
    void MoveTo(const FIntVector2& InPosition);
        
    // This function sets the cached window size and does not modify the PlatformWindow size.
    void SetSize(const FIntVector2& InSize);
    
    // This function sets the cached window position and does not modify the PlatformWindow position.
    void SetPosition(const FIntVector2& InPosition);

    // Returns the current cached window size
    FIntVector2 GetSize() const;
    
    // Returns the current cached window position
    FIntVector2 GetPosition() const;

    // Returns the current cached window width
    uint32 GetWidth() const;
    
    // Returns the current cached window height
    uint32 GetHeight() const;

    // Returns the current overlay-widget
    TSharedPtr<FWidget> GetOverlay() const;
    
    // Returns the current content-widget
    TSharedPtr<FWidget> GetContent() const;

    // Set the overlay-widget. This widget will recieve an event before the content does and allow
    // the overlay to respond to the event before the content recieves it.
    void SetOverlay(const TSharedPtr<FWidget>& InOverlay);
    
    // Set the content-widget.
    void SetContent(const TSharedPtr<FWidget>& InContent);

    // Show the window with the option to set the window focus to this window when starting to
    // display the window. This only happens if there is a valid PlatformWindow.
    void Show(bool bFocus = true);
    
    // Minimizes the window if there is a valid PlatformWindow.
    void Minimize();
    
    // Maximizes the window if there is a valid PlatformWindow.
    void Maximize();
    
    // Restore the window to the previous state (position and size) if the window has been minimized
    // or maximized.
    void Restore();

    // Returns true if the FWindow's PlatformWindow is the current active window
    bool IsActive() const;
    
    // Returns true if the FWindow's PlatformWindow is the currently minimized
    bool IsMinimized() const;
    
    // Returns true if the FWindow's PlatformWindow is the currently maximized
    bool IsMaximized() const;

    // Returns the DPI scale of the monitor that this FWindow is currently displayed on.
    float GetWindowDPIScale() const;

    // Sets the cached title and also set the PlatformWindow text if there currently is a valid PlatformWindow.
    void SetTitle(const FString& InTitle);
    
    // Sets the cached window-style and also set the PlatformWindow style if there currently is valid
    // PlatformWindow.
    void SetStyle(EWindowStyleFlags InStyleFlags);
    
    // Sets the PlatformWindow. This function also updates the current cached variables based on the new platform window.
    void SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow);
    
    // Sets this FWindow's PlatformWindow to be the window that has the focus.
    void SetFocus();
    
    // Sets the opacity of the PlatformWindow.
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
