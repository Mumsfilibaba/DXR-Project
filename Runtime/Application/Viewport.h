#pragma once
#include "IViewport.h"
#include "Element.h"

class APPLICATION_API FViewportElement
    : public FElement
{
public:
    FViewportElement(const TWeakPtr<Element>& InParentElement);
    virtual ~FViewportElement() = default;

    virtual bool CreateRHI();
    virtual void DestroyRHI();

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
    virtual bool OnWindowMove(const FWindowMovedEvent& InMoveEvent) override;
    virtual bool OnWindowFocusGained() override;
    virtual bool OnWindowFocusLost() override;
    virtual bool OnWindowClosed() override;

    void SetViewportInterface(const TSharedPtr<IViewport>& InViewportInterface)
    {
        ViewportInterface = InViewportInterface;
    }

    TSharedPtr<IViewport>       GetViewportInterface()       { return ViewportInterface; }
    TSharedPtr<const IViewport> GetViewportInterface() const { return ViewportInterface; }

    FIntVector2 GetSize() const;

    FRHIViewportRef GetRHI() const 
    { 
        return ViewportRHI; 
    }

private:
    TSharedPtr<IViewport> ViewportInterface;
    FRHIViewportRef       ViewportRHI;
};