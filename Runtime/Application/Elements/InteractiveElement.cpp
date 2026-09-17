#include "Application/Elements/InteractiveElement.h"
#include "Application/Application.h"

FInteractiveElement::FInteractiveElement()
    : FCompoundElement()
    , bIsHovered(false)
    , bIsPressed(false)
    , bIsEnabled(true)
{
}

FInteractiveElement::~FInteractiveElement() = default;

FEventResponse FInteractiveElement::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    if (bIsEnabled)
    {
        SetHovered(true);
    }

    return FEventResponse::Unhandled();
}

FEventResponse FInteractiveElement::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    SetHovered(false);
    return FEventResponse::Unhandled();
}

FEventResponse FInteractiveElement::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!bIsEnabled)
    {
        return FEventResponse::Unhandled();
    }

    UpdateHoverFromCursor(CursorEvent);

    if (bIsPressed)
    {
        OnDragged(CursorEvent);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FInteractiveElement::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (!bIsEnabled || !AcceptsPressFromKey(CursorEvent.GetKey()))
    {
        return FEventResponse::Unhandled();
    }

    BeginPress();
    return FEventResponse::Handled();
}

FEventResponse FInteractiveElement::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!bIsPressed || !AcceptsPressFromKey(CursorEvent.GetKey()))
    {
        return FEventResponse::Unhandled();
    }

    EndPress(CursorEvent);
    return FEventResponse::Handled();
}

FEventResponse FInteractiveElement::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (!bIsEnabled)
    {
        return FEventResponse::Unhandled();
    }

    const FKey Key = KeyEvent.GetKey();
    if (Key == Keys::Space || Key == Keys::Enter || Key == Keys::KeypadEnter)
    {
        OnClicked();
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FInteractiveElement::OnFocusLost()
{
    const TSharedPtr<FVisualElement> Captor = FApplication::IsInitialized() 
        ? FApplication::Get().GetMouseCaptor()
        : nullptr;

    if (Captor.Get() != this)
    {
        CancelPress();
    }

    return FEventResponse::Unhandled();
}

bool FInteractiveElement::SupportsKeyboardFocus() const
{
    return bIsEnabled;
}

bool FInteractiveElement::IsInteractive() const
{
    return true;
}

bool FInteractiveElement::GetCursor(ECursor& OutCursor) const
{
    if (!bIsEnabled || !bIsHovered || !IsPressable())
    {
        return false;
    }

    OutCursor = ECursor::Hand;
    return true;
}

void FInteractiveElement::SetEnabled(bool bInIsEnabled)
{
    if (bIsEnabled == bInIsEnabled)
    {
        return;
    }

    bIsEnabled = bInIsEnabled;
    if (!bIsEnabled)
    {
        CancelPress();
        bIsHovered = false;
    }

    OnInteractionStateChanged();
}

EInteractionState FInteractiveElement::GetInteractionState() const
{
    if (!bIsEnabled)
    {
        return EInteractionState::Disabled;
    }

    if (bIsPressed)
    {
        return EInteractionState::Pressed;
    }

    return bIsHovered ? EInteractionState::Hovered : EInteractionState::Normal;
}

bool FInteractiveElement::IsPressable() const
{
    return true;
}

void FInteractiveElement::OnClicked()
{
}

void FInteractiveElement::OnInteractionStateChanged()
{
}

bool FInteractiveElement::AcceptsPressFromKey(FKey Key) const
{
    return Key == Keys::MouseButtonLeft;
}

void FInteractiveElement::OnDragged(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);
}

void FInteractiveElement::BeginPress()
{
    if (bIsPressed)
    {
        return;
    }

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    SetPressed(true);
}

void FInteractiveElement::CancelPress()
{
    if (!bIsPressed)
    {
        return;
    }

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    SetPressed(false);
}

void FInteractiveElement::EndPress(const FCursorEvent& CursorEvent)
{
    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    const bool bReleasedInside = GetContentRectangle().EncapsulatesPoint(CursorEvent.GetClientPosition());

    SetPressed(false);
    UpdateHoverFromCursor(CursorEvent);

    if (bReleasedInside)
    {
        OnClicked();
    }
}

void FInteractiveElement::UpdateHoverFromCursor(const FCursorEvent& CursorEvent)
{
    SetHovered(GetContentRectangle().EncapsulatesPoint(CursorEvent.GetClientPosition()));
}

void FInteractiveElement::SetHovered(bool bInIsHovered)
{
    if (bIsHovered == bInIsHovered)
    {
        return;
    }

    bIsHovered = bInIsHovered;
    OnInteractionStateChanged();
}

void FInteractiveElement::SetPressed(bool bInIsPressed)
{
    if (bIsPressed == bInIsPressed)
    {
        return;
    }

    bIsPressed = bInIsPressed;
    OnInteractionStateChanged();
}
