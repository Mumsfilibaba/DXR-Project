#include "Application/Elements/VisualElement.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"

FVisualElement::FVisualElement()
    : TSharedFromThis<FVisualElement>()
    , Visibility(EVisibility::Visible)
    , ActivationPolicy(EElementActivationPolicy::DoNotAutoFocusOnWindowActivate)
    , ContentRectangle()
    , CachedDesiredSize()
    , ParentElement()
{
}

FVisualElement::~FVisualElement()
{
}

void FVisualElement::Tick(const FRectangle& AssignedBounds)
{
    SetContentRectangle(AssignedBounds);
    OnArrange(AssignedBounds);
}

bool FVisualElement::IsWindow() const
{
    return false;
}

bool FVisualElement::IsInteractive() const
{
    return false;
}

bool FVisualElement::CapturesAllInput() const
{
    return false;
}

bool FVisualElement::SupportsKeyboardFocus() const
{
    return false;
}

bool FVisualElement::WantsTextInput() const
{
    return false;
}

TSharedPtr<FVisualElement> FVisualElement::GetFocusTarget()
{
    return AsSharedPtr();
}

FEventResponse FVisualElement::OnAnalogGamepadChange(const FAnalogGamepadEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyDown(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyUp(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyChar(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseMove(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseButtonDown(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseButtonUp(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseScroll(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseDoubleClick(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseLeft(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseEntered(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnHighPrecisionMouseInput(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

bool FVisualElement::GetCursor(ECursor&) const
{
    return false;
}

FEventResponse FVisualElement::OnFocusLost()
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnFocusGained()
{
    return FEventResponse::Unhandled();
}

IntVector2 FVisualElement::ComputeDesiredSize() const
{
    return IntVector2(0, 0);
}

void FVisualElement::OnArrange(const FRectangle& /*AllottedBounds*/)
{
}

void FVisualElement::GetChildren(TArray<TSharedPtr<FVisualElement>>& /*OutChildren*/) const
{
}

int32 FVisualElement::OnDraw(const FDrawGeometry& /*AllottedGeometry*/, FDrawCommandList& /*OutCommandList*/, int32 LayerId) const
{
    return LayerId;
}

void FVisualElement::SetOuterCornerRadius(float /*InCornerRadius*/)
{
}

int32 FVisualElement::GetContentTopInset() const
{
    return 0;
}

IntVector2 FVisualElement::PrepareDesiredSize()
{
    TArray<TSharedPtr<FVisualElement>> Children;
    GetChildren(Children);

    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (Child)
        {
            Child->PrepareDesiredSize();
        }
    }

    CachedDesiredSize = ComputeDesiredSize();
    return CachedDesiredSize;
}

void FVisualElement::FindParentElements(FElementPath& OutRootPath)
{
    if (ParentElement.IsValid())
    {
        ParentElement->FindParentElements(OutRootPath);
    }

    OutRootPath.Add(Visibility, AsSharedPtr());
}

void FVisualElement::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    if (ContentRectangle.EncapsulatesPoint(ClientPosition))
    {
        OutChildElements.Add(Visibility, AsSharedPtr());
    }
}

void FVisualElement::SetVisibility(EVisibility InVisibility)
{
    Visibility = InVisibility;
}

void FVisualElement::SetParentElement(const TWeakPtr<FVisualElement>& InParentElement)
{
    ParentElement = InParentElement;
}

void FVisualElement::SetContentRectangle(const FRectangle& InContentRectangle)
{
    ContentRectangle = InContentRectangle;
}
