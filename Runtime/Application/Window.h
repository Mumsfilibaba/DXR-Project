#pragma once
#include "Viewport.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Event.h"
#include "Core/Math/IntVector2.h"
#include "CoreApplication/Generic/GenericWindow.h"

class APPLICATION_API FWindow
{
public:
    FWindow();
    virtual ~FWindow() = default;

    bool Create();

    void Show(bool bMaximized);
    void Close();

    void Minimize();
    void Maximize();
    void Restore();

    DECLARE_EVENT(FOnWindowClosedEvent, FWindow);
    FOnWindowClosedEvent& GetWindowClosedEvent() { return OnWindowClosedEvent; }

    DECLARE_EVENT(FOnWindowResizedEvent, FWindow, const FWindowResizedEvent&);
    FOnWindowResizedEvent& GetWindowResizedEvent() { return OnWindowResizeEvent; }

    DECLARE_EVENT(FOnWindowMovedEvent, FWindow, const FWindowResizedEvent&);
    FOnWindowMovedEvent& GetWindowMovedEvent() { return OnWindowMovedEvent; }

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent);
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent);
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent);

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent);
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent);
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent);
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent);
    virtual bool OnMouseEntered();
    virtual bool OnMouseLeft();

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent);
    virtual bool OnWindowMoved(const FWindowMovedEvent& InMoveEvent);
    virtual bool OnWindowFocusGained();
    virtual bool OnWindowFocusLost();
    virtual bool OnWindowClosed();

    bool IsActiveWindow() const;
    bool IsVisible()      const { return bIsVisible; }
    bool IsIsMaximized()  const { return bIsMaximized; }

    TSharedPtr<FViewport>       GetViewport()       { return Viewport; };
    TSharedPtr<const FViewport> GetViewport() const { return Viewport; };

    void SetViewport(const TSharedPtr<FViewport>& InViewport)
    { 
        Viewport = InViewport; 
    }

    TSharedRef<FGenericWindow> GetNativeWindow() 
    { 
        return NativeWindow; 
    }
    
    void SetWidth(uint32 InWidth);
    uint32 GetWidth()  const { return Width; }
    
    void SetHeight(uint32 InHeight);
    uint32 GetHeight() const { return Height; }

    void SetTitle(const FString& InTitle);
    const FString& GetTitle() const { return Title; }
    
    void SetWindowMode(EWindowMode InWindowMode);
    EWindowMode GetWindowMode() const { return WindowMode; }

private:
    TSharedPtr<FViewport>      Viewport;
    TSharedRef<FGenericWindow> NativeWindow;

    FString     Title;

    uint32      Width;
    uint32      Height;

    EWindowMode WindowMode;
    bool        bIsVisible;
    bool        bIsMaximized;

    FOnWindowClosedEvent  OnWindowClosedEvent;
    FOnWindowResizedEvent OnWindowResizeEvent;
    FOnWindowMovedEvent   OnWindowMovedEvent;
};