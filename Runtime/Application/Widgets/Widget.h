#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"

class FWidgetPath;

template<typename WidgetType>
inline TSharedPtr<WidgetType> CreateWidget(const typename WidgetType::FInitializer& Initializer)
{
    TSharedPtr<WidgetType> NewWidget = MakeSharedPtr<WidgetType>();
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

    bool EncapsualtesPoint(const FIntVector2& Point) const
    {
        return Point.x >= Position.x && Point.y >= Position.y && Point.x <= (Position.x + Width) && Point.y <= (Position.y + Height);
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

    virtual void Tick();
    virtual bool IsWindow() const;

    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent);
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent);
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent);
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent);
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent);
    virtual FEventResponse OnFocusLost();
    virtual FEventResponse OnFocusGained();

    virtual void FindParentWidgets(FWidgetPath& OutParentWidgets);
    virtual void FindChildrenUnderCursor(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutChildWidgets);

    bool IsVisible() const 
    { 
        return (Visibility & EVisibility::Visible) == EVisibility::None; 
    }

    EVisibility GetVisibility() const
    {
        return Visibility;
    }

    void SetVisibility(EVisibility InVisibility);
    void SetParentWidget(const TWeakPtr<FWidget>& InParentWidget);
    void SetScreenRectangle(const FRectangle& InBounds);

    TWeakPtr<FWidget> GetParentWidget() const
    {
        return ParentWidget;
    }

    const FRectangle& GetScreenRectangle() const
    {
        return ScreenRectangle;
    }

private:
    EVisibility       Visibility;
    FRectangle        ScreenRectangle;
    TWeakPtr<FWidget> ParentWidget;
};
