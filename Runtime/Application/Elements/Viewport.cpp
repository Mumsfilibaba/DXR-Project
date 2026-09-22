#include "Core/Misc/OutputDeviceManager.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Viewport.h"
#include "Application/Elements/Window.h"

TSharedPtr<FViewport> FViewport::Create(const FDesc& Desc)
{
    TSharedPtr<FViewport> NewInstance = MakeSharedPtr<FViewport>();
    if (NewInstance)
    {
        NewInstance->Initialize(Desc);
    }

    return NewInstance;
}

FViewport::FViewport()
    : FVisualElement()
    , ViewportInterface(nullptr)
    , SceneBrush()
    , Position()
    , Size()
{
}

FViewport::~FViewport()
{
}

void FViewport::Initialize(const FDesc& Desc)
{
    ViewportInterface = Desc.ViewportInterface;
}

int32 FViewport::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (SceneBrush.IsValid())
    {
        OutCommandList.AddImage(LayerId, AllottedGeometry.Bounds, SceneBrush, FFloatColor(1.0f, 1.0f, 1.0f, 1.0f));
    }

    return LayerId;
}

void FViewport::SetPosition(const IntVector2& InPosition, EViewportPositionSpace InSpace)
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
                TSharedPtr<FWindow> Window = StaticCastSharedPtr<FWindow>(LocalParentElement.ToSharedPtr());

                const IntVector2 WindowPos = Window->GetPosition();
                Position.X -= WindowPos.X;
                Position.Y -= WindowPos.Y;
                break;
            }

            LocalParentElement = LocalParentElement->GetParentElement();
        }
    }
}

void FViewport::Tick(const FRectangle& AssignedBounds)
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

FEventResponse FViewport::OnAnalogGamepadChange(const FAnalogGamepadEvent& AnalogGamepadEvent)
{
    return ViewportInterface ? ViewportInterface->OnAnalogGamepadChange(AnalogGamepadEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnKeyDown(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyDown(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnKeyUp(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyUp(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnKeyChar(const FKeyEvent& KeyEvent)
{
    return ViewportInterface ? ViewportInterface->OnKeyChar(KeyEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseMove(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseMove(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonDown(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseButtonUp(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseScroll(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseDoubleClick(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseDoubleClick(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseLeft(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnMouseEntered(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnHighPrecisionMouseInput(const FCursorEvent& CursorEvent)
{
    return ViewportInterface ? ViewportInterface->OnHighPrecisionMouseInput(CursorEvent) : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnFocusLost()
{
    return ViewportInterface ? ViewportInterface->OnFocusLost() : FEventResponse::Unhandled();
}

FEventResponse FViewport::OnFocusGained()
{
    return ViewportInterface ? ViewportInterface->OnFocusGained() : FEventResponse::Unhandled();
}

bool FViewport::SupportsKeyboardFocus() const
{
    return true;
}
