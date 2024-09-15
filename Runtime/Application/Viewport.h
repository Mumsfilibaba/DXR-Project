#pragma once
#include "IViewport.h"
#include "Widget.h"

class APPLICATION_API FViewport : public FWidget
{
public:
    struct FInitializer
    {
        TSharedPtr<IViewport> ViewportInterface;
    };

    FViewport();
    virtual ~FViewport();

    void Initialize(const FInitializer& Initializer);

    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override final;
    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent) override final;
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent) override final;
    virtual FResponse OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnMouseScroll(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnMouseLeft(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnMouseEntered(const FCursorEvent& CursorEvent) override final;
    virtual FResponse OnFocusLost() override final;
    virtual FResponse OnFocusGained() override final;

    void SetViewportInterface(const TSharedPtr<IViewport>& InViewportInterface)
    {
        ViewportInterface = InViewportInterface;
    }

    TSharedPtr<IViewport>       GetViewportInterface()       { return ViewportInterface; }
    TSharedPtr<const IViewport> GetViewportInterface() const { return ViewportInterface; }

private:
    TSharedPtr<IViewport> ViewportInterface;
};