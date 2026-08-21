#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Elements/ViewportElement.h"
#include "Application/Elements/WindowElement.h"

TSharedPtr<FViewportElement> FViewportElement::Create(const FInitializer& Initializer)
{
    TSharedPtr<FViewportElement> NewElement = MakeSharedPtr<FViewportElement>();
    if (NewElement)
    {
        NewElement->Initialize(Initializer);
    }

    return NewElement;
}

FViewportElement::FViewportElement()
    : FVisualElement()
    , ViewportInterface(nullptr)
    , Position()
    , Size()
{
}

FViewportElement::~FViewportElement()
{
}

void FViewportElement::Initialize(const FInitializer& Initializer)
{
    ViewportInterface = Initializer.ViewportInterface;
}

void FViewportElement::SetPosition(const IntVector2& InPosition, EViewportPositionSpace InSpace)
{
    Position = InPosition;

    if (InSpace == EViewportPositionSpace::Screen)
    {
        TWeakPtr<FVisualElement> LocalParentElement = GetParentElement();
        CHECK(LocalParentElement != nullptr);

        while (LocalParentElement)
        {
            if (LocalParentElement->IsWindow())
            {
                TSharedPtr<FWindowElement> WindowElement = StaticCastSharedPtr<FWindowElement>(LocalParentElement.ToSharedPtr());

                const IntVector2 WindowPos = WindowElement->GetPosition();
                Position.X -= WindowPos.X;
                Position.Y -= WindowPos.Y;
                break;
            }

            LocalParentElement = LocalParentElement->GetParentElement();
        }
    }
}

void FViewportElement::Tick(const FRectangle& AssignedBounds)
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

FEventResponse FViewportElement::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    return ViewportInterface ? ViewportInterface->OnAnalogGamepadChange(AnalogGamepadEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyDown(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyUp(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyChar(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseMove(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnHighPrecisionMouseInput(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnFocusLost()
{
    return ViewportInterface ? ViewportInterface->OnFocusLost() : FEventResponse::Unhandled();
}

FEventResponse FViewportElement::OnFocusGained()
{
    return ViewportInterface ? ViewportInterface->OnFocusGained() : FEventResponse::Unhandled();
}
