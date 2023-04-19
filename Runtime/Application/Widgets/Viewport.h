#pragma once
#include "IViewport.h"
#include "Application/Widget.h"
#include "RHI/RHIResources.h"

class APPLICATION_API FViewportWidget : public FWidget
{
public:
    FINITIALIZER_START(FViewportWidget)
        FINITIALIZER_ATTRIBUTE(uint32, Width);
        FINITIALIZER_ATTRIBUTE(uint32, Height);
        FINITIALIZER_ATTRIBUTE(TSharedPtr<IViewport>, ViewportInterface);
    FINITIALIZER_END();

    FViewportWidget();
    virtual ~FViewportWidget() = default;

    void Initialize(const FInitializer& Initializer);

    virtual bool CreateRHI();
    virtual void DestroyRHI();

    virtual void Paint(const FRectangle& AssignedBounds) override final;

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

    FIntVector2 GetSize() const;

    FRHIViewportRef GetRHI() const 
    { 
        return ViewportRHI; 
    }

private:
    TSharedPtr<IViewport> ViewportInterface;
    uint32                Width;
    uint32                Height;
    FRHIViewportRef       ViewportRHI;
};