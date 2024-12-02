#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/Events.h"

class FWidgetPath;

/**
 * @brief Enumeration for widget visibility states.
 */
enum class EVisibility
{
    None    = 0,      /** @brief No visibility flags set. */
    Hidden  = BIT(1), /** @brief Widget is hidden. */
    Visible = BIT(2), /** @brief Widget is visible. */
};

ENUM_CLASS_OPERATORS(EVisibility);

/**
 * @brief Creates a widget of the specified type.
 * 
 * @tparam WidgetType The type of the widget to create.
 * @param Initializer The initializer structure for the widget.
 * @return A shared pointer to the newly created widget.
 */
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
    /**
     * @brief Default constructor initializes width and height to zero.
     */
    FRectangle()
        : Width(0)
        , Height(0)
        , Position()
    {
    }

    /**
     * @brief Checks if the rectangle encapsulates a given point.
     * 
     * @param Point The point to check.
     * @return True if the point is within the rectangle; false otherwise.
     */
    bool EncapsulatesPoint(const FIntVector2& Point) const
    {
        return Point.X >= Position.X && Point.Y >= Position.Y && Point.X <= (Position.X + Width) && Point.Y <= (Position.Y + Height);
    }

    /**
     * @brief Equality operator.
     * 
     * @param Other The rectangle to compare with.
     * @return True if both rectangles are equal; false otherwise.
     */
    bool operator==(const FRectangle& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    /**
     * @brief Inequality operator.
     * 
     * @param Other The rectangle to compare with.
     * @return True if rectangles are not equal; false otherwise.
     */
    bool operator!=(const FRectangle& Other) const
    {
        return !(*this == Other);
    }

    /** @brief Width of the rectangle. */
    int32 Width;

    /** @brief Height of the rectangle. */
    int32 Height;

    /** @brief Position of the rectangle (x, y). */
    FIntVector2 Position;
};

class APPLICATION_API FWidget : public TSharedFromThis<FWidget>
{
public:

    /**
     * @brief Constructs a widget.
     */
    FWidget();

    /**
     * @brief Virtual destructor for FWidget.
     */
    virtual ~FWidget();

    /**
     * @brief Updates the widget.
     * 
     * This method updates the current bounds based on the parent widget and other factors.
     */
    virtual void Tick();
    
    /**
     * @brief Checks if the widget is a window.
     * 
     * This method returns true only if the widget is an FWindow, and false for all other widget types.
     * @return True if the widget is an FWindow; false otherwise.
     */
    virtual bool IsWindow() const;

    /**
     * @brief Handles analog gamepad input changes.
     * 
     * @param AnalogGamepadEvent The analog gamepad event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent);

    /**
     * @brief Handles key down events.
     * 
     * @param KeyEvent The key event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent);

    /**
     * @brief Handles key up events.
     * 
     * @param KeyEvent The key event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyUp(const FKeyEvent& KeyEvent);

    /**
     * @brief Handles character input events.
     * 
     * @param KeyEvent The key event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnKeyChar(const FKeyEvent& KeyEvent);

    /**
     * @brief Handles mouse movement events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse button down events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse button up events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse scroll events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse double-click events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse leave events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles mouse enter events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles high-precision mouse input events.
     * 
     * @param CursorEvent The cursor event.
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent);

    /**
     * @brief Handles focus lost events.
     * 
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnFocusLost();

    /**
     * @brief Handles focus gained events.
     * 
     * @return An event response indicating how the event was handled.
     */
    virtual FEventResponse OnFocusGained();

    /**
     * @brief Adds all parent widgets to a widget path.
     * 
     * This function adds the parent widgets starting from the first parent (should be an FWindow) up to the child widget.
     * The widget calling this function will be at the last position in the widget path.
     * 
     * @param OutParentWidgets The widget path to populate with parent widgets.
     */
    virtual void FindParentWidgets(FWidgetPath& OutParentWidgets);
    
    /**
     * @brief Adds all child widgets under a specified point to the widget path.
     * 
     * This function adds all child widgets of this widget that are under the specified position.
     * 
     * @param ScreenCursorPosition The screen position to check.
     * @param OutChildWidgets The widget path to populate with child widgets.
     */
    virtual void FindChildrenContainingPoint(const FIntVector2& ScreenCursorPosition, FWidgetPath& OutChildWidgets);

    /**
     * @brief Checks if the widget is visible.
     * 
     * @return True if the widget is visible; false otherwise.
     */
    bool IsVisible() const
    { 
        return (Visibility & EVisibility::Visible) != EVisibility::None; 
    }

    /**
     * @brief Gets the visibility state of the widget.
     * 
     * @return The visibility state.
     */
    EVisibility GetVisibility() const
    {
        return Visibility;
    }

    /**
     * @brief Sets the visibility state of the widget.
     * 
     * @param InVisibility The new visibility state.
     */
    void SetVisibility(EVisibility InVisibility);

    /**
     * @brief Sets the parent widget.
     * 
     * @param InParentWidget A weak pointer to the parent widget.
     */
    void SetParentWidget(const TWeakPtr<FWidget>& InParentWidget);

    /**
     * @brief Sets the content rectangle of the widget.
     * 
     * @param InContentRectangle The new content rectangle.
     */
    void SetContentRectangle(const FRectangle& InContentRectangle);

    /**
     * @brief Gets the parent widget.
     * 
     * @return A weak pointer to the parent widget.
     */
    TWeakPtr<FWidget> GetParentWidget() const
    {
        return ParentWidget;
    }

    /**
     * @brief Gets the content rectangle of the widget.
     * 
     * @return A constant reference to the content rectangle.
     */
    const FRectangle& GetContentRectangle() const
    {
        return ContentRectangle;
    }

private:

    /** @brief The visibility state of the widget. */
    EVisibility Visibility;

    /** @brief The content rectangle of the widget. */
    FRectangle ContentRectangle;

    /** @brief Weak pointer to the parent widget. */
    TWeakPtr<FWidget> ParentWidget;
};
