#pragma once
#include "Widget.h"
#include "Application/IViewport.h"

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

    virtual void Tick() override final;

    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent) override final;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent) override final;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent) override final;
    virtual FEventResponse OnFocusLost() override final;
    virtual FEventResponse OnFocusGained() override final;

    void SetViewportInterface(const TSharedPtr<IViewport>& InViewportInterface)
    {
        ViewportInterface = InViewportInterface;
    }

    TSharedPtr<IViewport>       GetViewportInterface()       { return ViewportInterface; }
    TSharedPtr<const IViewport> GetViewportInterface() const { return ViewportInterface; }

private:
    TSharedPtr<IViewport> ViewportInterface;
};
