#include "Application/Elements/Slider.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 VALUE_TEXT_SPACING = 2;

TSharedPtr<FSlider> FSlider::Create(const FDesc& Desc)
{
    TSharedPtr<FSlider> NewSlider = MakeSharedPtr<FSlider>();
    NewSlider->Initialize(Desc);
    return NewSlider;
}

FSlider::FSlider()
    : FInteractiveElement()
    , MinValue(0.0f)
    , MaxValue(1.0f)
    , Value(0.0f)
    , StepSize(0.0f)
    , Orientation(EOrientation::Horizontal)
    , HandleSize(12)
    , TrackThickness(4)
    , MinLength(96)
    , Precision(2)
    , bShowValueText(false)
    , Font(nullptr)
    , OnValueChangedDelegate()
    , OnValueCommittedDelegate()
{
}

FSlider::~FSlider() = default;

void FSlider::Initialize(const FDesc& Desc)
{
    MinValue                 = Desc.MinValue;
    MaxValue                 = Math::Max(Desc.MaxValue, Desc.MinValue);
    StepSize                 = Math::Max(Desc.StepSize, 0.0f);
    Orientation              = Desc.Orientation;
    HandleSize               = Math::Max(Desc.HandleSize, 1);
    TrackThickness           = Math::Max(Desc.TrackThickness, 1);
    MinLength                = Math::Max(Desc.MinLength, HandleSize);
    Precision                = Math::Clamp(Desc.Precision, 0, 9);
    bShowValueText           = Desc.bShowValueText;
    Font                     = Desc.Font;
    OnValueChangedDelegate   = Desc.OnValueChanged;
    OnValueCommittedDelegate = Desc.OnValueCommitted;

    Value = SanitizeValue(Desc.Value);
}

IntVector2 FSlider::ComputeDesiredSize() const
{
    int32 Breadth = Math::Max(HandleSize, TrackThickness);
    if (bShowValueText && Font && Orientation == EOrientation::Horizontal)
    {
        Breadth += VALUE_TEXT_SPACING + Font->GetCapHeight() + Font->GetDescent();
    }

    return Orientation == EOrientation::Horizontal ? IntVector2(MinLength, Breadth) : IntVector2(Breadth, MinLength);
}

int32 FSlider::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style = FUIStyle::GetDefault();
    const EInteractionState State = GetInteractionState();
    const FRectangle        TrackArea = ComputeInteractiveBounds(AllottedGeometry.Bounds);

    const FRectangle Handle = ComputeHandleBounds(AllottedGeometry.Bounds);

    FRectangle Groove = TrackArea;
    if (Orientation == EOrientation::Horizontal)
    {
        Groove.Height     = Math::Min(TrackThickness, TrackArea.Height);
        Groove.Position.Y = TrackArea.Position.Y + ((TrackArea.Height - Groove.Height) / 2);
    }
    else
    {
        Groove.Width      = Math::Min(TrackThickness, AllottedGeometry.Bounds.Width);
        Groove.Position.X = AllottedGeometry.Bounds.Position.X + ((AllottedGeometry.Bounds.Width - Groove.Width) / 2);
    }

    const FCornerRadii GrooveRadii(static_cast<float>(Math::Min(Groove.Width, Groove.Height)) * 0.5f);
    OutCommandList.AddBox(LayerId, Groove, Style.Colors.ControlNormal, GrooveRadii);

    FRectangle Filled = Groove;
    if (Orientation == EOrientation::Horizontal)
    {
        Filled.Width = Math::Max(Handle.GetCenter().X - Groove.Position.X, 0);
    }
    else
    {
        const int32 Bottom = Groove.GetBottom();
        Filled.Height      = Math::Max(Bottom - Handle.GetCenter().Y, 0);
        Filled.Position.Y  = Bottom - Filled.Height;
    }

    if (Filled.Width > 0 && Filled.Height > 0)
    {
        OutCommandList.AddBox(LayerId, Filled, IsEnabled() ? Style.Colors.Accent : Style.Colors.ControlDisabled, GrooveRadii);
    }

    const FCornerRadii HandleRadii(static_cast<float>(Math::Min(Handle.Width, Handle.Height)) * 0.5f);
    OutCommandList.AddBox(LayerId, Handle, Style.GetControlColor(State), HandleRadii);
    OutCommandList.AddBoxOutline(LayerId, Handle, Style.Colors.Border, Style.Metrics.BorderThickness, HandleRadii);

    if (bShowValueText && Font)
    {
        const String Text      = GetFormattedValue();
        const int32  TextWidth = Font->MeasureWidth(StringView(Text.Data(), Text.Length()));
        const int32  CapInset  = Math::Max(Font->GetAscent() - Font->GetCapHeight(), 0);
        const int32  TextX     = AllottedGeometry.Bounds.Position.X + ((AllottedGeometry.Bounds.Width - TextWidth) / 2);
        const int32  TextY     = TrackArea.GetBottom() + VALUE_TEXT_SPACING - CapInset;

        const FRectangle TextBounds(IntVector2(TextX, TextY), TextWidth, Font->GetTextBandHeight());

        OutCommandList.AddText(LayerId + 1, TextBounds, Text, Font.Get(), Style.GetTextColor(State));
        return LayerId + 1;
    }

    return LayerId;
}

FEventResponse FSlider::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response = FInteractiveElement::OnMouseButtonDown(CursorEvent);

    // A click anywhere on the track jumps the handle there, so the press doubles as the first drag step
    if (IsPressed())
    {
        SetValueFromPosition(CursorEvent.GetClientPosition());
    }

    return Response;
}

FEventResponse FSlider::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    const bool bWasPressed = IsPressed();

    const FEventResponse Response = FInteractiveElement::OnMouseButtonUp(CursorEvent);
    if (bWasPressed)
    {
        OnValueCommittedDelegate.ExecuteIfBound(Value);
    }

    return Response;
}

FEventResponse FSlider::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (!IsEnabled())
    {
        return FEventResponse::Unhandled();
    }

    const float Nudge = StepSize > 0.0f ? StepSize : (MaxValue - MinValue) * 0.01f;

    const FKey Key = KeyEvent.GetKey();
    if (Key == Keys::Left || Key == Keys::Down)
    {
        ApplyValue(Value - Nudge);
    }
    else if (Key == Keys::Right || Key == Keys::Up)
    {
        ApplyValue(Value + Nudge);
    }
    else if (Key == Keys::Home)
    {
        ApplyValue(MinValue);
    }
    else if (Key == Keys::End)
    {
        ApplyValue(MaxValue);
    }
    else
    {
        return FEventResponse::Unhandled();
    }

    OnValueCommittedDelegate.ExecuteIfBound(Value);
    return FEventResponse::Handled();
}

void FSlider::SetValue(float InValue)
{
    Value = SanitizeValue(InValue);
}

void FSlider::SetRange(float InMinValue, float InMaxValue)
{
    MinValue = InMinValue;
    MaxValue = Math::Max(InMaxValue, InMinValue);
    Value    = SanitizeValue(Value);
}

float FSlider::GetNormalizedValue() const
{
    const float Range = MaxValue - MinValue;
    return Range > 0.0f ? Math::Saturate((Value - MinValue) / Range) : 0.0f;
}

FRectangle FSlider::GetHandleBounds() const
{
    return ComputeHandleBounds(GetContentRectangle());
}

FRectangle FSlider::GetTrackBounds() const
{
    return ComputeTrackBounds(GetContentRectangle());
}

String FSlider::GetFormattedValue() const
{
    return String::Printf("%.*f", Precision, Value);
}

void FSlider::OnDragged(const FCursorEvent& CursorEvent)
{
    SetValueFromPosition(CursorEvent.GetClientPosition());
}

float FSlider::SanitizeValue(float InValue) const
{
    float Result = Math::Clamp(InValue, MinValue, MaxValue);

    if (StepSize > 0.0f)
    {
        const float Steps = Math::Round((Result - MinValue) / StepSize);
        Result            = Math::Clamp(MinValue + (Steps * StepSize), MinValue, MaxValue);
    }

    return Result;
}

void FSlider::ApplyValue(float InValue)
{
    const float NewValue = SanitizeValue(InValue);
    if (NewValue == Value)
    {
        return;
    }

    Value = NewValue;
    OnValueChangedDelegate.ExecuteIfBound(Value);
}

void FSlider::SetValueFromPosition(const IntVector2& ClientPosition)
{
    const FRectangle Track = GetTrackBounds();

    float Fraction = 0.0f;
    if (Orientation == EOrientation::Horizontal)
    {
        if (Track.Width > 0)
        {
            Fraction = static_cast<float>(ClientPosition.X - Track.Position.X) / static_cast<float>(Track.Width);
        }
    }
    else if (Track.Height > 0)
    {
        Fraction = static_cast<float>(Track.GetBottom() - ClientPosition.Y) / static_cast<float>(Track.Height);
    }

    ApplyValue(MinValue + (Math::Saturate(Fraction) * (MaxValue - MinValue)));
}

FRectangle FSlider::ComputeHandleBounds(const FRectangle& Bounds) const
{
    const FRectangle TrackArea = ComputeInteractiveBounds(Bounds);
    const FRectangle Track     = ComputeTrackBounds(Bounds);
    const int32      Distance  = Math::RoundToInt(GetNormalizedValue() * static_cast<float>(Orientation == EOrientation::Horizontal ? Track.Width : Track.Height));

    FRectangle Handle;
    if (Orientation == EOrientation::Horizontal)
    {
        Handle.Width      = HandleSize;
        Handle.Height     = Math::Min(HandleSize, TrackArea.Height);
        Handle.Position.X = Track.Position.X + Distance - (HandleSize / 2);
        Handle.Position.Y = TrackArea.Position.Y + ((TrackArea.Height - Handle.Height) / 2);
    }
    else
    {
        Handle.Width      = Math::Min(HandleSize, Bounds.Width);
        Handle.Height     = HandleSize;
        Handle.Position.X = Bounds.Position.X + ((Bounds.Width - Handle.Width) / 2);
        Handle.Position.Y = Track.GetBottom() - Distance - (HandleSize / 2);
    }

    return Handle;
}

FRectangle FSlider::ComputeTrackBounds(const FRectangle& Bounds) const
{
    const FRectangle TrackArea = ComputeInteractiveBounds(Bounds);
    const int32      Inset     = HandleSize / 2;

    FRectangle Track = TrackArea;
    if (Orientation == EOrientation::Horizontal)
    {
        Track.Position.X += Inset;
        Track.Width = Math::Max(TrackArea.Width - (Inset * 2), 0);
    }
    else
    {
        Track.Position.Y += Inset;
        Track.Height = Math::Max(TrackArea.Height - (Inset * 2), 0);
    }

    return Track;
}

FRectangle FSlider::ComputeInteractiveBounds(const FRectangle& Bounds) const
{
    if (!bShowValueText || !Font || Orientation != EOrientation::Horizontal)
    {
        return Bounds;
    }

    FRectangle TrackArea = Bounds;
    TrackArea.Height     = Math::Min(Math::Max(HandleSize, TrackThickness), Bounds.Height);
    return TrackArea;
}
