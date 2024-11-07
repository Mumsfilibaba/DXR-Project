#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"

class FWidgetPath;

enum class EVisibility
{
    None    = 0,
    Hidden  = BIT(1),
    Visible = BIT(2),
};

ENUM_CLASS_OPERATORS(EVisibility);

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

    // Update the widget. This updates the current bounds based on the parent widget etc.
    virtual void Tick();
    
    // Helper function to check if the widget is a window. This only returns true of FWindow, and false for
    // all other widget-types.
    virtual bool IsWindow() const;

    // EventHandling
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

    // Adds all parent widgets to a FWidgetPath. This function adds the first parent-widget (Should be a FWindow)
    // following the childwidget. This results in the widget being called being at the last position in the FWidgetPath.
    virtual void FindParentWidgets(FWidgetPath& OutParentWidgets);
    
    // This function adds all child-widgets to this widget that also is under the specified position. 
    virtual void FindChildrenContainingPoint(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutChildWidgets);

    // Returns true if the visibility is set to true
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
    void SetContentRectangle(const FRectangle& InContentRectangle);

    TWeakPtr<FWidget> GetParentWidget() const
    {
        return ParentWidget;
    }

    const FRectangle& GetContentRectangle() const
    {
        return ContentRectangle;
    }

private:
    EVisibility       Visibility;
    FRectangle        ContentRectangle;
    TWeakPtr<FWidget> ParentWidget;
};
