#include "Application/Style/UIStyle.h"

static FUIStyle GDefaultStyle;

FUIStyle::FUIStyle()
    : Colors()
    , Metrics()
    , NormalFont(nullptr)
    , MonospaceFont(nullptr)
{
}

const FUIStyle& FUIStyle::GetDefault()
{
    return GDefaultStyle;
}

void FUIStyle::SetDefault(const FUIStyle& InStyle)
{
    GDefaultStyle = InStyle;
}

void FUIStyle::ResetDefault()
{
    GDefaultStyle = FUIStyle();
}

const FFloatColor& FUIStyle::GetControlColor(EInteractionState State) const
{
    switch (State)
    {
        case EInteractionState::Hovered:  return Colors.ControlHovered;
        case EInteractionState::Pressed:  return Colors.ControlPressed;
        case EInteractionState::Disabled: return Colors.ControlDisabled;
        default:                          return Colors.ControlNormal;
    }
}

const FFloatColor& FUIStyle::GetButtonColor(EInteractionState State, bool bIsSelected) const
{
    if (State == EInteractionState::Disabled)
    {
        return Colors.ControlDisabled;
    }

    const bool bIsLit = State == EInteractionState::Hovered || State == EInteractionState::Pressed;

    if (bIsSelected)
    {
        return bIsLit ? Colors.AccentHovered : Colors.Accent;
    }

    return bIsLit ? (State == EInteractionState::Pressed ? Colors.ButtonPressed : Colors.ButtonHovered) : Colors.ButtonNormal;
}

const FFloatColor& FUIStyle::GetTextColor(EInteractionState State) const
{
    return State == EInteractionState::Disabled ? Colors.TextDisabled : Colors.Text;
}
