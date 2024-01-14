#pragma once
#include "IViewport.h"
#include "ApplicationEventHandler.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/SharedPtr.h"
#include "RHI/RHIResources.h"

class FGenericWindow;

struct FViewportInitializer
{
    FViewportInitializer()
        : Window(nullptr)
        , Width(0)
        , Height(0)
    {
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

class APPLICATION_API FViewport : public FApplicationEventHandler, public TSharedFromThis<FViewport>
{
public:
    FViewport();
    ~FViewport();

    bool InitializeRHI(const FViewportInitializer& Initializer);
    
    void ReleaseRHI();

    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override final;

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;
    
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent) override final;
    
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) override final;

    virtual FResponse OnMouseMove(const FCursorEvent& MouseEvent) override final;
    
    virtual FResponse OnMouseButtonDown(const FCursorEvent& MouseEvent) override final;
    
    virtual FResponse OnMouseButtonUp(const FCursorEvent& MouseEvent) override final;
    
    virtual FResponse OnMouseScroll(const FCursorEvent& MouseEvent) override final;
    
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& MouseEvent) override final;

	virtual FResponse OnWindowResized(const FWindowEvent& WindowEvent) override final;
    
    virtual FResponse OnWindowFocusLost(const FWindowEvent& WindowEvent) override final;
    
    virtual FResponse OnWindowFocusGained(const FWindowEvent& WindowEvent) override final;
    
    virtual FResponse OnMouseLeft(const FWindowEvent& WindowEvent) override final;
    
    virtual FResponse OnMouseEntered(const FWindowEvent& WindowEvent) override final;
	
    virtual FResponse OnWindowClosed(const FWindowEvent& WindowEvent) override final;

    FRHIViewportRef GetRHIViewport() const
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
    FRHIViewportRef            RHIViewport;
    TSharedRef<FGenericWindow> Window;
    TSharedPtr<IViewport>      ViewportInterface;
};