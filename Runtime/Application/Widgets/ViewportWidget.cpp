#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Widgets/ViewportWidget.h"
#include "Application/Widgets/WindowWidget.h"

FViewportWidget::FViewportWidget()
    : FWidget()
    , ViewportInterface(nullptr)
    , Position()
    , Size()
{
}

FViewportWidget::~FViewportWidget()
{
}

void FViewportWidget::Initialize(const FInitializer& Initializer)
{
    ViewportInterface = Initializer.ViewportInterface;
}

void FViewportWidget::SetPosition(const FIntVector2& InPosition, EViewportPositionSpace InSpace)
{
    Position = InPosition;

    if (InSpace == EViewportPositionSpace::Screen)
    {
        TWeakPtr<FWidget> LocalParentWidget = GetParentWidget();
        CHECK(LocalParentWidget != nullptr);

        while (LocalParentWidget)
        {
            if (LocalParentWidget->IsWindow())
            {
                TSharedPtr<FWindowWidget> WindowWidget = StaticCastSharedPtr<FWindowWidget>(LocalParentWidget.ToSharedPtr());

                const FIntVector2 WindowPos = WindowWidget->GetPosition();
                Position.X -= WindowPos.X;
                Position.Y -= WindowPos.Y;
                break;
            }

            LocalParentWidget = LocalParentWidget->GetParentWidget();
        }
    }
}

void FViewportWidget::Tick(const FRectangle& AssignedBounds)
{
    if (Size.X != 0 && Size.Y != 0)
    {
        FRectangle Desired;
        Desired.Position = AssignedBounds.Position + Position;
        Desired.Width    = Size.X;
        Desired.Height   = Size.Y;

        const int32 ParentLeft    = AssignedBounds.Position.X;
        const int32 ParentTop     = AssignedBounds.Position.Y;
        const int32 ParentRight   = ParentLeft + AssignedBounds.Width;
        const int32 ParentBottom  = ParentTop + AssignedBounds.Height;

        const int32 DesiredLeft   = Desired.Position.X;
        const int32 DesiredTop    = Desired.Position.Y;
        const int32 DesiredRight  = DesiredLeft + Desired.Width;
        const int32 DesiredBottom = DesiredTop + Desired.Height;

        const int32 ClampedLeft   = Math::Max(DesiredLeft, ParentLeft);
        const int32 ClampedTop    = Math::Max(DesiredTop, ParentTop);
        const int32 ClampedRight  = Math::Min(DesiredRight, ParentRight);
        const int32 ClampedBottom = Math::Min(DesiredBottom, ParentBottom);

        FRectangle ViewportRectangle;
        ViewportRectangle.Position.X = ClampedLeft;
        ViewportRectangle.Position.Y = ClampedTop;
        ViewportRectangle.Width      = Math::Max(ClampedRight - ClampedLeft, 0);
        ViewportRectangle.Height     = Math::Max(ClampedBottom - ClampedTop, 0);

        SetContentRectangle(ViewportRectangle);
    }
    else
    {
        SetContentRectangle(AssignedBounds);
    }
}

FEventResponse FViewportWidget::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    return ViewportInterface ? ViewportInterface->OnAnalogGamepadChange(AnalogGamepadEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyDown(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyUp(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyChar(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseMove(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnHighPrecisionMouseInput(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnFocusLost()
{
    return ViewportInterface ? ViewportInterface->OnFocusLost() : FEventResponse::Unhandled();
}

FEventResponse FViewportWidget::OnFocusGained()
{
    return ViewportInterface ? ViewportInterface->OnFocusGained() : FEventResponse::Unhandled();
}
