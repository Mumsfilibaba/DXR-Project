#pragma once
#include "IViewport.h"
#include "ApplicationEventHandler.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/SharedPtr.h"

class FGenericWindow;
class FRHIViewport;

struct FViewportInitializer
{
    FViewportInitializer()
        : Window(nullptr)
        , Width(0)
        , Height(0)
    {
    }

    FViewportInitializer& SetWindow(FGenericWindow* InWindow)
    {
        Window = InWindow;
        return *this;
    }

    FViewportInitializer& SetWidth(int32 InWidth)
    {
        Width = InWidth;
        return *this;
    }

    FViewportInitializer& SetHeight(int32 InHeight)
    {
        Height = InHeight;
        return *this;
    }

    bool operator==(const FViewportInitializer& Other) const
    {
        return Window == Other.Window && Width == Other.Width && Height == Other.Height;
    }

    bool operator!=(const FViewportInitializer& Other) const
    {
        return !(*this == Other);
    }

    FGenericWindow* Window;
    int32           Width;
    int32           Height;
};

class FViewport : public FApplicationEventHandler, public TSharedFromThis<FViewport>
{
public:
    FViewport();
    ~FViewport();

    bool InitializeRHI(const FViewportInitializer& Initializer);
    void ReleaseRHI();

    virtual FResponse OnControllerButtonUp    (const FControllerEvent& ControllerEvent) override final;
    virtual FResponse OnControllerButtonDown  (const FControllerEvent& ControllerEvent) override final;
    virtual FResponse OnControllerButtonAnalog(const FControllerEvent& ControllerEvent) override final;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual FResponse OnKeyUp  (const FKeyEvent& KeyEvent) override final;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) override final;

    virtual FResponse OnMouseMove       (const FMouseEvent& MouseEvent) override final;
    virtual FResponse OnMouseButtonDown (const FMouseEvent& MouseEvent) override final;
    virtual FResponse OnMouseButtonUp   (const FMouseEvent& MouseEvent) override final;
    virtual FResponse OnMouseEntered    (const FMouseEvent& MouseEvent) override final;
    virtual FResponse OnMouseLeft       (const FMouseEvent& MouseEvent) override final;
    virtual FResponse OnMouseDoubleClick(const FMouseEvent& MouseEvent) override final;

    virtual FResponse OnWindowResized(const FWindowEvent& WindowEvent) override final;
    virtual FResponse OnWindowClosed (const FWindowEvent& WindowEvent) override final;

    TSharedRef<FRHIViewport> GetRHIViewport() const
    {
        return RHIViewport;
    }

    TSharedRef<FGenericWindow> GetWindow() const
    {
        return Window;
    }

    void SetViewportInterface(const TSharedPtr<IViewport>& InViewportInterface)
    {
        ViewportInterface = InViewportInterface;
    }

    TSharedPtr<IViewport> GetViewportInterface()
    { 
        return ViewportInterface;
    }

    TSharedPtr<const IViewport> GetViewportInterface() const
    { 
        return ViewportInterface;
    }

private:
    TSharedRef<FRHIViewport>   RHIViewport;
    TSharedRef<FGenericWindow> Window;
    TSharedPtr<IViewport>      ViewportInterface;
};