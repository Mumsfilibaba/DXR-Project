#include "Application/Elements/ColorPicker.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 COLOR_PICKER_BAND_WIDTH        = 2;
constexpr float COLOR_PICKER_CROSSHAIR_RADIUS  = 5.0f;
constexpr float COLOR_PICKER_CROSSHAIR_STROKE  = 1.5f;
constexpr int32 COLOR_PICKER_HUE_MARKER_HEIGHT = 3;

static FFloatColor MakeColorFromHSV(float Hue, float Saturation, float Brightness)
{
    // A hue of exactly one lands past the last sector, so it is folded back into it rather than wrapping
    const float Sector      = Math::Clamp(Hue, 0.0f, 1.0f) * 6.0f;
    const int32 SectorIndex = Math::Min(static_cast<int32>(Sector), 5);
    const float Remainder   = Sector - static_cast<float>(SectorIndex);

    const float P = Brightness * (1.0f - Saturation);
    const float Q = Brightness * (1.0f - (Saturation * Remainder));
    const float W = Brightness * (1.0f - (Saturation * (1.0f - Remainder)));

    switch (SectorIndex)
    {
        case 0:  return FFloatColor(Brightness, W, P, 1.0f);
        case 1:  return FFloatColor(Q, Brightness, P, 1.0f);
        case 2:  return FFloatColor(P, Brightness, W, 1.0f);
        case 3:  return FFloatColor(P, Q, Brightness, 1.0f);
        case 4:  return FFloatColor(W, P, Brightness, 1.0f);
        default: return FFloatColor(Brightness, P, Q, 1.0f);
    }
}

static void BreakColorIntoHSV(const FFloatColor& Color, float& OutHue, float& OutSaturation, float& OutBrightness)
{
    const float Maximum = Math::Max(Color.R, Math::Max(Color.G, Color.B));
    const float Minimum = Math::Min(Color.R, Math::Min(Color.G, Color.B));
    const float Delta   = Maximum - Minimum;

    OutBrightness = Maximum;
    OutSaturation = Maximum > 0.0f ? (Delta / Maximum) : 0.0f;

    if (Delta <= 0.0f)
    {
        OutHue = -1.0f;
        return;
    }

    float Hue = 0.0f;
    if (Maximum == Color.R)
    {
        Hue = (Color.G - Color.B) / Delta;
    }
    else if (Maximum == Color.G)
    {
        Hue = 2.0f + ((Color.B - Color.R) / Delta);
    }
    else
    {
        Hue = 4.0f + ((Color.R - Color.G) / Delta);
    }

    Hue /= 6.0f;
    OutHue = Hue < 0.0f ? (Hue + 1.0f) : Hue;
}

TSharedPtr<FColorPicker> FColorPicker::Create(const FDesc& Desc)
{
    TSharedPtr<FColorPicker> NewColorPicker = MakeSharedPtr<FColorPicker>();
    NewColorPicker->Initialize(Desc);
    return NewColorPicker;
}

FColorPicker::FColorPicker()
    : FInteractiveElement()
    , Hue(0.0f)
    , Saturation(0.0f)
    , Brightness(1.0f)
    , SquareExtent(160)
    , HueBarWidth(16)
    , Spacing(8)
    , bIsDraggingHue(false)
    , OnColorPickedDelegate()
{
}

FColorPicker::~FColorPicker() = default;

void FColorPicker::Initialize(const FDesc& Desc)
{
    SquareExtent          = Math::Max(1, Desc.SquareExtent);
    HueBarWidth           = Math::Max(1, Desc.HueBarWidth);
    Spacing               = Math::Max(0, Desc.Spacing);
    OnColorPickedDelegate = Desc.OnColorPicked;

    SetColor(Desc.Color);
}

IntVector2 FColorPicker::ComputeDesiredSize() const
{
    return IntVector2(SquareExtent + Spacing + HueBarWidth, SquareExtent);
}

int32 FColorPicker::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&  Style  = FUIStyle::GetDefault();
    const FRectangle Square = GetSquareRectangle(AllottedGeometry.Bounds);
    const FRectangle HueBar = GetHueBarRectangle(AllottedGeometry.Bounds);

    OutCommandList.AddBox(LayerId, Square, MakeColorFromHSV(Hue, 1.0f, 1.0f));

    for (int32 OffsetX = 0; OffsetX < Square.Width; OffsetX += COLOR_PICKER_BAND_WIDTH)
    {
        const float Fraction = static_cast<float>(OffsetX) / static_cast<float>(Math::Max(1, Square.Width - 1));

        const FRectangle Band(IntVector2(Square.Position.X + OffsetX, Square.Position.Y), Math::Min(COLOR_PICKER_BAND_WIDTH, Square.Width - OffsetX), Square.Height);
        OutCommandList.AddBox(LayerId, Band, FFloatColor(1.0f, 1.0f, 1.0f, 1.0f - Fraction));
    }

    for (int32 OffsetY = 0; OffsetY < Square.Height; OffsetY += COLOR_PICKER_BAND_WIDTH)
    {
        const float Fraction = static_cast<float>(OffsetY) / static_cast<float>(Math::Max(1, Square.Height - 1));

        const FRectangle Band(IntVector2(Square.Position.X, Square.Position.Y + OffsetY), Square.Width, Math::Min(COLOR_PICKER_BAND_WIDTH, Square.Height - OffsetY));
        OutCommandList.AddBox(LayerId, Band, FFloatColor(0.0f, 0.0f, 0.0f, Fraction));
    }

    OutCommandList.AddBoxOutline(LayerId, Square, Style.Colors.InputFieldBorder, Style.Metrics.BorderThickness);

    for (int32 OffsetY = 0; OffsetY < HueBar.Height; OffsetY += COLOR_PICKER_BAND_WIDTH)
    {
        const float Fraction = static_cast<float>(OffsetY) / static_cast<float>(Math::Max(1, HueBar.Height - 1));

        const FRectangle Band(IntVector2(HueBar.Position.X, HueBar.Position.Y + OffsetY), HueBar.Width, Math::Min(COLOR_PICKER_BAND_WIDTH, HueBar.Height - OffsetY));
        OutCommandList.AddBox(LayerId, Band, MakeColorFromHSV(Fraction, 1.0f, 1.0f));
    }

    OutCommandList.AddBoxOutline(LayerId, HueBar, Style.Colors.InputFieldBorder, Style.Metrics.BorderThickness);

    const Vector2 Crosshair(
        static_cast<float>(Square.Position.X) + (Saturation * static_cast<float>(Square.Width)),
        static_cast<float>(Square.Position.Y) + ((1.0f - Brightness) * static_cast<float>(Square.Height)));

    OutCommandList.AddCircle(LayerId, Crosshair, COLOR_PICKER_CROSSHAIR_RADIUS, FFloatColor(0.0f, 0.0f, 0.0f, 1.0f), COLOR_PICKER_CROSSHAIR_STROKE);
    OutCommandList.AddCircle(LayerId, Crosshair, COLOR_PICKER_CROSSHAIR_RADIUS - COLOR_PICKER_CROSSHAIR_STROKE, FFloatColor::White, COLOR_PICKER_CROSSHAIR_STROKE);

    const int32 MarkerY = HueBar.Position.Y + static_cast<int32>(Hue * static_cast<float>(HueBar.Height)) - (COLOR_PICKER_HUE_MARKER_HEIGHT / 2);

    const FRectangle Marker(IntVector2(HueBar.Position.X, MarkerY), HueBar.Width, COLOR_PICKER_HUE_MARKER_HEIGHT);
    OutCommandList.AddBoxOutline(LayerId, Marker, FFloatColor::White, 1.0f);

    return LayerId;
}

FEventResponse FColorPicker::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FInteractiveElement::OnMouseButtonDown(CursorEvent);
    }

    // Which region the press started in owns the whole drag, so sliding off the square does not grab the bar
    bIsDraggingHue = GetHueBarRectangle(GetContentRectangle()).EncapsulatesPoint(CursorEvent.GetClientPosition());

    ApplyCursor(CursorEvent.GetClientPosition());
    return FInteractiveElement::OnMouseButtonDown(CursorEvent);
}

void FColorPicker::OnDragged(const FCursorEvent& CursorEvent)
{
    ApplyCursor(CursorEvent.GetClientPosition());
}

void FColorPicker::SetColor(const FFloatColor& InColor)
{
    float NewHue = 0.0f;
    BreakColorIntoHSV(InColor, NewHue, Saturation, Brightness);

    if (NewHue >= 0.0f)
    {
        Hue = NewHue;
    }
}

FFloatColor FColorPicker::GetColor() const
{
    return MakeColorFromHSV(Hue, Saturation, Brightness);
}

FRectangle FColorPicker::GetSquareRectangle(const FRectangle& Bounds) const
{
    const int32 Extent = Math::Max(1, Math::Min(Bounds.Height, Bounds.Width - Spacing - HueBarWidth));
    return FRectangle(Bounds.Position, Extent, Extent);
}

FRectangle FColorPicker::GetHueBarRectangle(const FRectangle& Bounds) const
{
    const FRectangle Square = GetSquareRectangle(Bounds);
    return FRectangle(IntVector2(Square.GetRight() + Spacing, Bounds.Position.Y), HueBarWidth, Square.Height);
}

void FColorPicker::ApplyCursor(const IntVector2& ClientPosition)
{
    const FRectangle Bounds = GetContentRectangle();

    if (bIsDraggingHue)
    {
        const FRectangle HueBar = GetHueBarRectangle(Bounds);
        Hue = Math::Clamp(static_cast<float>(ClientPosition.Y - HueBar.Position.Y) / static_cast<float>(Math::Max(1, HueBar.Height)), 0.0f, 1.0f);
    }
    else
    {
        const FRectangle Square = GetSquareRectangle(Bounds);

        Saturation = Math::Clamp(static_cast<float>(ClientPosition.X - Square.Position.X) / static_cast<float>(Math::Max(1, Square.Width)), 0.0f, 1.0f);
        Brightness = 1.0f - Math::Clamp(static_cast<float>(ClientPosition.Y - Square.Position.Y) / static_cast<float>(Math::Max(1, Square.Height)), 0.0f, 1.0f);
    }

    NotifyColorPicked();
}

void FColorPicker::NotifyColorPicked()
{
    OnColorPickedDelegate.ExecuteIfBound(GetColor());
}
