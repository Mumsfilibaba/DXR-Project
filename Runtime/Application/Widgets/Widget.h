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

enum class EVisibility
{
    None    = 0,
    Hidden  = BIT(1),
    Visible = BIT(2),
};

ENUM_CLASS_OPERATORS(EVisibility);

struct FRectangle
{
    FRectangle()
        : Width(0)
        , Height(0)
        , Position()
    {
    }

    bool operator==(const FRectangle& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    bool operator!=(const FRectangle& Other) const
    {
        return !(*this == Other);
    }

    int32       Width;
    int32       Height;
    FIntVector2 Position;
};

class APPLICATION_API FWidget : public TSharedFromThis<FWidget>
{
public:
    FWidget();
    virtual ~FWidget();

    virtual void Tick() { }

    virtual bool IsWindow() const { return false; }

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

    bool IsVisible() const { return (Visibility & EVisibility::Visible) == EVisibility::None; }

    void SetVisibility(EVisibility InVisibility) 
    { 
        Visibility = InVisibility;
    }

    void SetParentWidget(const TWeakPtr<FWidget>& InParentWidget)
    {
        ParentWidget = InParentWidget;
    }

    TWeakPtr<FWidget> GetParentWidget() const
    {
        return ParentWidget;
    }

    const FRectangle& GetBoundsRectangle() const
    {
        return Bounds;
    }

private:
    TWeakPtr<FWidget> ParentWidget;
    EVisibility       Visibility;
    FRectangle        Bounds;
};