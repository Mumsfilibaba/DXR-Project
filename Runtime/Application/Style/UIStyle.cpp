#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

static FUIStyle GDefaultStyle;

int32 FUIStyleMetrics::ResolveButtonMinWidth(const IFontFace* Face, const FMargin& Padding) const
{
    if (!Face || !ButtonMinLabel)
    {
        return 0;
    }

    return Face->MeasureWidth(StringView(ButtonMinLabel)) + Padding.GetTotalHorizontal();
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
