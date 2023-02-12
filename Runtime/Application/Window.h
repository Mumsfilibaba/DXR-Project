#pragma once
#include "Viewport.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Event.h"
#include "Core/Math/IntVector2.h"
#include "CoreApplication/Generic/GenericWindow.h"

struct FWindowInitializer
{
    EWindowMode WindowMode;
    uint32      Width;
    uint32      Height;
};

class APPLICATION_API FWindow
    : public FElement
{
public:
    FWindow();
    virtual ~FWindow() = default;

    bool Initialize(const FWindowInitializer& Initializer);

    void ShowWindow(bool bMaximized);
    void CloseWindow();

    void Minimize();
    void Maximize();
    void Restore();

    DECLARE_EVENT(FOnWindowClosedEvent, FWindow);
    FOnWindowClosedEvent& GetWindowClosedEvent() { return OnWindowClosedEvent; }

    DECLARE_EVENT(FOnWindowResizedEvent, FWindow, const FWindowResizedEvent&);
    FOnWindowResizedEvent& GetWindowResizedEvent() { return OnWindowResizeEvent; }

    DECLARE_EVENT(FOnWindowMovedEvent, FWindow, const FWindowResizedEvent&);
    FOnWindowMovedEvent& GetWindowMovedEvent() { return OnWindowMovedEvent; }

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) override;
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) override;

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent) override;
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent) override;
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent) override;
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent) override;
    virtual bool OnMouseEntered() override;
    virtual bool OnMouseLeft() override;

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent) override;
    virtual bool OnWindowMoved(const FWindowMovedEvent& InMoveEvent) override;
    virtual bool OnWindowFocusGained() override;
    virtual bool OnWindowFocusLost() override;
    virtual bool OnWindowClosed() override;

    bool IsActiveWindow() const 
    {
        return NativeWindow ? NativeWindow->IsActiveWindow() : false; 
    }

    bool IsVisible()     const { return bIsVisible; }
    bool IsIsMaximized() const { return bIsMaximized; }

    void SetContent(const TSharedPtr<FElement>& InContent)
    { 
        Content = InContent; 
    }

    TSharedPtr<FElement>       GetContent()       { return Content; };
    TSharedPtr<const FElement> GetContent() const { return Content; };

    TSharedRef<FGenericWindow> GetNativeWindow() 
    {
        return NativeWindow; 
    }
    
    void SetTitle(const FString& InTitle)
    {
        if (NativeWindow)
        {
            NativeWindow->SetTitle(Title);
        }

        Title = InTitle;
    }

    const FString& GetTitle() const 
    { 
        return Title; 
    }

    void SetExtent(const FIntVector2& InExtent);
    FIntVector2 GetExtent() const;
    
    void SetWindowMode(EWindowMode InWindowMode);
    
    EWindowMode GetWindowMode() const 
    { 
        return WindowMode; 
    }

private:
    TSharedPtr<FElement>       Content;
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