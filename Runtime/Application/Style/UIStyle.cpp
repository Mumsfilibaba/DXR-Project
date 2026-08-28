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

const FFloatColor& FUIStyle::GetTextColor(EInteractionState State) const
{
    return State == EInteractionState::Disabled ? Colors.TextDisabled : Colors.Text;
}
