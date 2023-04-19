#pragma once
#include "WidgetCreation.h"
#include "WidgetChildren.h"
#include "Core/Math/Vector2.h"
#include "Core/Containers/SharedPtr.h"
#include "Application/Core/Events.h"
#include "Application/Core/Rectangle.h"

template<typename WidgetType>
struct FWidgetInitializerBase
{
    using InitializerType = typename WidgetType::FInitializer;

    FWidgetInitializerBase()
        : Content(nullptr)
        , Visibility(Visibility_All)
    {
    }

    FINITIALIZER_ATTRIBUTE(TSharedPtr<FWidget>, Content);
    FINITIALIZER_ATTRIBUTE(EVisibility, Visibility);
};

class APPLICATION_API FWidget : public TEnableSharedFromThis<FWidget>
{
public:
    FWidget();
    virtual ~FWidget() = default;

    virtual void Paint(const FRectangle& AssignedBounds) = 0;

    virtual void ArrangeChildren(FFilteredWidgets& OutArrangedChildren);

    virtual void FindWidgetsUnderCursor(FIntVector2 ScreenPosition, FFilteredWidgets& OutChildren);

public: // Event Handling
    virtual FResponse OnControllerButtonDown  (const FControllerEvent& ControllerEvent);
    virtual FResponse OnControllerButtonUp    (const FControllerEvent& ControllerEvent);
    virtual FResponse OnControllerButtonAnalog(const FControllerEvent& ControllerEvent);

    virtual FResponse OnKeyDown(const FKeyEvent& KeyEvent);
    virtual FResponse OnKeyUp  (const FKeyEvent& KeyEvent);
    virtual FResponse OnKeyChar(const FKeyEvent& KeyEvent);

    virtual FResponse OnMouseMove       (const FMouseEvent& MouseEvent);
    virtual FResponse OnMouseButtonDown (const FMouseEvent& MouseEvent);
    virtual FResponse OnMouseButtonUp   (const FMouseEvent& MouseEvent);
    virtual FResponse OnMouseEntered    (const FMouseEvent& MouseEvent);
    virtual FResponse OnMouseLeft       (const FMouseEvent& MouseEvent);
    virtual FResponse OnMouseDoubleClick(const FMouseEvent& MouseEvent);

public:
    virtual FWidgetChildren* GetChildren()
    {
        return &Content;
    }

    FRectangle GetDesktopBounds() const
    {
        return DesktopBounds;
    }

    FRectangle GetLocalBounds() const
    {
        return LocalBounds;
    }

    void SetContent(const TSharedPtr<FWidget>& InContent)
    {
        if (InContent)
        {
            Content.AttachWidget(InContent);
        }
    }

    void SetVisibility(EVisibility InVisibility)
    {
        Visibility = InVisibility;
    }

    EVisibility GetVisibility() const
    {
        return Visibility;
    }

protected:
    EVisibility Visibility;

    TSingleWidgetChildrenSlot<FSlotBase> Content;

    FRectangle LocalBounds;
    FRectangle DesktopBounds;
};
