#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"

template<typename WidgetType>
inline TSharedPtr<WidgetType> CreateWidget(const typename WidgetType::FInitializer& Initializer)
{
    TSharedPtr<WidgetType> NewWidget = MakeShared<WidgetType>();
    if (NewWidget)
    {
        NewWidget->Initialize(Initializer);
    }

    return NewWidget;
}

class APPLICATION_API FWidget : public TSharedFromThis<FWidget>
{
public:
    FWidget();
    virtual ~FWidget();

    virtual void Tick();

    virtual FResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent);
    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent);
    virtual FResponse OnKeyUp(const FKeyEvent& KeyEvent);
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent);
    virtual FResponse OnMouseMove(const FCursorEvent& CursorEvent);
    virtual FResponse OnMouseButtonDown(const FCursorEvent& CursorEvent);
    virtual FResponse OnMouseButtonUp(const FCursorEvent& CursorEvent);
    virtual FResponse OnMouseScroll(const FCursorEvent& CursorEvent);
    virtual FResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent);
    virtual FResponse OnMouseLeft(const FCursorEvent& CursorEvent);
    virtual FResponse OnMouseEntered(const FCursorEvent& CursorEvent);
    virtual FResponse OnFocusLost();
    virtual FResponse OnFocusGained();

    virtual bool IsWindowWidget() const { return false; }

    void SetParentWidget(const TWeakPtr<FWidget>& InParentWidget)
    {
        ParentWidget = InParentWidget;
    }

    TWeakPtr<FWidget> GetParentWidget() const
    {
        return ParentWidget;
    }

private:
    TWeakPtr<FWidget> ParentWidget;
};