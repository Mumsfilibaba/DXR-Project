#pragma once
#include "Viewport.h"
#include "Overlay.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Event.h"
#include "Core/Math/IntVector2.h"
#include "CoreApplication/Generic/GenericWindow.h"

class APPLICATION_API FWindow : public FWidget
{
    DECLARE_WIDGET(FWindow, FWidget);

public:
    FINITIALIZER_START(FWindow)
        FInitializer()
            : Title("Window")
            , Width(0)
            , Height(0)
            , WindowMode(EWindowMode::Windowed)
        {
        }

        FINITIALIZER_ATTRIBUTE(FString, Title);
        FINITIALIZER_ATTRIBUTE(uint32, Width);
        FINITIALIZER_ATTRIBUTE(uint32, Height);
        FINITIALIZER_ATTRIBUTE(EWindowMode, WindowMode);
    FINITIALIZER_END();

public:
    FWindow();
    virtual ~FWindow() = default;

    bool Initialize(const FInitializer& Initializer);

    virtual void Paint(const FRectangle& AssignedBounds) override;

    FOverlay::FScopedSlotInitilizer AddOverlaySlot();

    void RemoveOverlayWidget(const TSharedPtr<FWidget>& InWidget);

    void ShowWindow(bool bMaximized);
    
    void CloseWindow();

    void Minimize();
    
    void Maximize();

    void Restore();

public:
    DECLARE_EVENT(FOnWindowClosedEvent, FWindow);
    FOnWindowClosedEvent& GetWindowClosedEvent() { return OnWindowClosedEvent; }

    DECLARE_EVENT(FOnWindowResizedEvent, FWindow, const FWindowResizedEvent&);
    FOnWindowResizedEvent& GetWindowResizedEvent() { return OnWindowResizeEvent; }

    DECLARE_EVENT(FOnWindowMovedEvent, FWindow, const FWindowMovedEvent&);
    FOnWindowMovedEvent& GetWindowMovedEvent() { return OnWindowMovedEvent; }

    FResponse OnWindowResized(const FWindowResizedEvent& ResizeEvent);
    
    FResponse OnWindowMoved(const FWindowMovedEvent& InMoveEvent);
    
    FResponse OnWindowClosed();

public:
    bool IsActiveWindow() const 
    {
        return NativeWindow ? NativeWindow->IsActiveWindow() : false; 
    }

    bool IsVisible() const 
    { 
        return bIsVisible; 
    }

    bool IsIsMaximized() const 
    { 
        return bIsMaximized; 
    }

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
    
    uint32 GetWidth() const
    {
        return Width;
    }
    
    uint32 GetHeight() const
    {
        return Height;
    }

    void SetWindowMode(EWindowMode InWindowMode);
    
    EWindowMode GetWindowMode() const 
    { 
        return WindowMode; 
    }

private:
    TSharedRef<FGenericWindow> NativeWindow;
    TSharedPtr<FOverlay>       Overlay;

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