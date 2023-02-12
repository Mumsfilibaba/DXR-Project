#pragma once
#include "Events.h"

class APPLICATION_API FElement
{
public:
    FElement()
        : ParentElement(nullptr)
    { }

    FElement(const TWeakPtr<FElement>& InParentElement)
        : ParentElement(InParentElement)
    { }

    virtual ~FElement() = default;

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) { return false; };
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent) { return false; };
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) { return false; };

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent) { return false; };
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent) { return false; };
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent) { return false; };
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent) { return false; };
    virtual bool OnMouseEntered() { return false; };
    virtual bool OnMouseLeft() { return false; };

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent) { return false; };
    virtual bool OnWindowMove(const FWindowMovedEvent& InMoveEvent) { return false; };
    virtual bool OnWindowFocusGained() { return false; };
    virtual bool OnWindowFocusLost() { return false; };
    virtual bool OnWindowClosed() { return false; };
    
    virtual void Render() = 0;

    TWeakPtr<FElement>       GetParentElement()       { return ParentElement; }
    TWeakPtr<const FElement> GetParentElement() const { return ParentElement; }

private:
    TWeakPtr<FElement> ParentElement;
};
