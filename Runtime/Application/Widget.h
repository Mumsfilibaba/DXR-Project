#pragma once
#include "Events.h"
#include "Core/Containers/Array"

class APPLICATION_API FInterfaceElement
{
public:
    FInterfaceElement(const TWeakPtr<FInterfaceElement>& InParentElement);
    virtual ~FInterfaceElement() = default;

    virtual void OnDraw();

    void AddChildElement(const TSharedPtr<FInterfaceElement>& InChildElement);
    void RemoveChildElement(const TSharedPtr<FInterfaceElement>& InChildElement);

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)  { return false; };
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)    { return false; };
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) { return false; };

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent)      { return false; };
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent)     { return false; };
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent)       { return false; };
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent) { return false; };
    virtual bool OnMouseEntered() { return false; };
    virtual bool OnMouseLeft()    { return false; };

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent) { return false; };
    virtual bool OnWindowMove(const FWindowMovedEvent& InMoveEvent)        { return false; };
    virtual bool OnWindowFocusGained() { return false; };
    virtual bool OnWindowFocusLost()   { return false; };
    virtual bool OnWindowClosed()      { return false; };

    bool IsActive() const { return bIsActive; }

    TWeakPtr<FInterfaceElement>       GetParentElement()       { return ParentElement; }
    TWeakPtr<const FInterfaceElement> GetParentElement() const { return ParentElement; }

private:
    TWeakPtr<FInterfaceElement>           ParentElement;
    TArray<TSharedPtr<FInterfaceElement>> ChildElements;

    bool bIsActive;
};

class FWindow;

class APPLICATION_API FWidget
{
public:
    FWidget(const TWeakPtr<FWindow>& InParentWindow)
        : ParentWindow(InParentWindow)
    { }

    virtual ~FWidget() = default;

    virtual void OnDraw() { };

    virtual bool OnKeyDown(const FKeyEvent& KeyEvent)  { return false; };
    virtual bool OnKeyUp(const FKeyEvent& KeyEvent)    { return false; };
    virtual bool OnKeyChar(FKeyCharEvent KeyCharEvent) { return false; };

    virtual bool OnMouseMove(const FMouseMovedEvent& MouseEvent)      { return false; };
    virtual bool OnMouseDown(const FMouseButtonEvent& MouseEvent)     { return false; };
    virtual bool OnMouseUp(const FMouseButtonEvent& MouseEvent)       { return false; };
    virtual bool OnMouseScroll(const FMouseScrolledEvent& MouseEvent) { return false; };
    virtual bool OnMouseEntered() { return false; };
    virtual bool OnMouseLeft()    { return false; };

    virtual bool OnWindowResized(const FWindowResizedEvent& InResizeEvent) { return false; };
    virtual bool OnWindowMove(const FWindowMovedEvent& InMoveEvent)        { return false; };
    virtual bool OnWindowFocusGained() { return false; };
    virtual bool OnWindowFocusLost()   { return false; };
    virtual bool OnWindowClosed()      { return false; };

    TWeakPtr<FWindow>       GetParentWindow()       { return ParentWindow; }
    TWeakPtr<const FWindow> GetParentWindow() const { return ParentWindow; }

private:
    TWeakPtr<FWindow> ParentWindow;
};