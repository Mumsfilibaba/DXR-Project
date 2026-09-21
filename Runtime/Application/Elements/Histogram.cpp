#include "Application/Elements/Histogram.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Core/Math/Math.h"

constexpr int32 HISTOGRAM_TEXT_PADDING = 4;

TSharedPtr<FHistogram> FHistogram::Create(const FDesc& Desc)
{
    TSharedPtr<FHistogram> NewHistogram = MakeSharedPtr<FHistogram>();
    NewHistogram->Initialize(Desc);
    return NewHistogram;
}

FHistogram::FHistogram()
    : FVisualElement()
    , Font(nullptr)
    , Label()
    , Samples()
    , BarColor(FUIStyle::GetDefault().Colors.Accent)
    , BackgroundColor(FUIStyle::GetDefault().Colors.ControlNormal)
    , WarningColor(0.85f, 0.35f, 0.25f, 1.0f)
    , MinValue(0.0f)
    , MaxValue(0.0f)
    , WarningThreshold(0.0f)
    , Capacity(128)
    , OldestSample(0)
    , NumSamples(0)
    , PreferredHeight(64)
    , HoveredSample(InvalidSampleIndex)
    , SelectedSample(InvalidSampleIndex)
    , SelectedBarColor(FUIStyle::GetDefault().Colors.TextSelectionBackground)
    , DrawMode(EHistogramDrawMode::Bars)
    , LineThickness(2.0f)
    , bAutoScale(true)
    , bIsScrubbing(false)
{
    Samples.Resize(Capacity);
    Samples.Fill(0.0f);
}

FHistogram::~FHistogram() = default;

void FHistogram::Initialize(const FDesc& Desc)
{
    Font             = Desc.Font;
    Label            = Desc.Label;
    BarColor         = Desc.BarColor;
    BackgroundColor  = Desc.BackgroundColor;
    WarningColor     = Desc.WarningColor;
    SelectedBarColor = Desc.SelectedBarColor;
    DrawMode         = Desc.DrawMode;
    LineThickness    = Math::Max(Desc.LineThickness, 1.0f);
    MinValue         = Desc.MinValue;
    MaxValue         = Math::Max(Desc.MaxValue, Desc.MinValue);
    WarningThreshold = Desc.WarningThreshold;
    Capacity         = Math::Max(Desc.Capacity, 1);
    PreferredHeight  = Math::Max(Desc.PreferredHeight, 1);
    bAutoScale       = Desc.bAutoScale;

    Samples.Resize(Capacity);
    Clear();
}

IntVector2 FHistogram::ComputeDesiredSize() const
{
    return IntVector2(Capacity, PreferredHeight);
}

int32 FHistogram::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&  Style  = FUIStyle::GetDefault();
    const FRectangle Bounds = AllottedGeometry.Bounds;

    OutCommandList.PushClip(LayerId, Bounds);
    OutCommandList.AddBox(LayerId, Bounds, BackgroundColor);

    const int32 ColumnWidth = ResolveColumnWidth(Bounds.Width);
    const float Range       = ResolveUpperBound() - MinValue;

    int32 MaxLayerId = LayerId + 1;

    if (DrawMode == EHistogramDrawMode::Line)
    {
        if (SelectedSample != InvalidSampleIndex)
        {
            const int32 Left = Bounds.Position.X + (SelectedSample * ColumnWidth);
            const FRectangle Selection(IntVector2(Left, Bounds.Position.Y),
                Math::Min(ColumnWidth, Math::Max(Bounds.GetRight() - Left, 0)), Bounds.Height);
            if (!Selection.IsEmpty())
            {
                OutCommandList.AddBox(LayerId + 1, Selection, SelectedBarColor);
            }
        }

        TArray<Vector2> Points;
        Points.Reserve(NumSamples);

        for (int32 Index = 0; Index < NumSamples; ++Index)
        {
            const int32 Left = Bounds.Position.X + (Index * ColumnWidth);
            if (Left >= Bounds.GetRight())
            {
                break;
            }

            const float Value    = GetSample(Index);
            const float Fraction = Range > 0.0f ? Math::Saturate((Value - MinValue) / Range) : 0.0f;
            const float X        = static_cast<float>(Math::Min(Left + (ColumnWidth / 2), Bounds.GetRight() - 1));
            const float Y        = static_cast<float>(Bounds.GetBottom() - 1) - (Fraction * static_cast<float>(Math::Max(Bounds.Height - 1, 0)));

            Points.Add(Vector2(X, Y));
        }

        if (Points.Size() >= 2)
        {
            OutCommandList.AddPolyline(LayerId + 2, Points, BarColor, LineThickness);
            MaxLayerId = LayerId + 2;
        }
        else if (Points.Size() == 1)
        {
            OutCommandList.AddCircleFilled(LayerId + 2, Points[0], Math::Max(LineThickness, 2.0f), BarColor, 8);
            MaxLayerId = LayerId + 2;
        }

        if (WarningThreshold > MinValue && WarningThreshold < ResolveUpperBound())
        {
            const float WarningFraction = (WarningThreshold - MinValue) / Range;
            const int32 WarningY        = Bounds.GetBottom() - 1 - 
                Math::RoundToInt(WarningFraction * static_cast<float>(Math::Max(Bounds.Height - 1, 0)));
            
            OutCommandList.AddLine(LayerId + 2, FRectangle(IntVector2(Bounds.Position.X, WarningY), Bounds.Width, 1), WarningColor);
            MaxLayerId = LayerId + 2;
        }
    }
    else
    {
        for (int32 Index = 0; Index < NumSamples; ++Index)
        {
            const int32 Left = Bounds.Position.X + (Index * ColumnWidth);
            if (Left >= Bounds.GetRight())
            {
                break;
            }

            const float Value    = GetSample(Index);
            const float Fraction = Range > 0.0f ? Math::Saturate((Value - MinValue) / Range) : 0.0f;

            FRectangle Bar;
            Bar.Width      = ColumnWidth;
            Bar.Height     = Math::RoundToInt(Fraction * static_cast<float>(Bounds.Height));
            Bar.Position.X = Left;
            Bar.Position.Y = Bounds.GetBottom() - Bar.Height;

            if (!Bar.IsEmpty())
            {
                const bool bIsSelected = Index == SelectedSample;
                const bool bIsWarning  = WarningThreshold > 0.0f && Value > WarningThreshold;

                const FFloatColor& Fill = bIsSelected ? SelectedBarColor : (bIsWarning ? WarningColor : BarColor);
                OutCommandList.AddBox(LayerId + 1, Bar, Fill);
            }
        }
    }

    if (Font)
    {
        const FRectangle TextBounds = Bounds.Deflate(FMargin(HISTOGRAM_TEXT_PADDING));

        String Value;

        int32 ValueWidth = 0;
        if (HoveredSample != InvalidSampleIndex)
        {
            Value      = String::Printf("%.2f", GetSample(HoveredSample));
            ValueWidth = Font->MeasureWidth(StringView(Value.Data(), Value.Length()));
        }

        if (!Value.IsEmpty() && ValueWidth <= TextBounds.Width)
        {
            const FRectangle ValueBounds = FRectangle::AlignInBounds(TextBounds, IntVector2(ValueWidth, Font->GetLineHeight()),
                EHorizontalAlignment::Right, EVerticalAlignment::Top);
            OutCommandList.AddText(LayerId + 3, ValueBounds, Value, Font.Get(), Style.Colors.Text);

            MaxLayerId = LayerId + 3;
        }

        if (!Label.IsEmpty())
        {
            const int32  LabelWidth = ValueWidth > 0 ? (TextBounds.Width - ValueWidth - HISTOGRAM_TEXT_PADDING) : TextBounds.Width;
            const String Elided     = Font->ElideText(StringView(Label.Data(), Label.Length()), LabelWidth);

            if (!Elided.IsEmpty())
            {
                const FRectangle LabelBounds = FRectangle::AlignInBounds(TextBounds, IntVector2(LabelWidth, Font->GetLineHeight()),
                    EHorizontalAlignment::Left, EVerticalAlignment::Top);
                OutCommandList.AddText(LayerId + 3, LabelBounds, Elided, Font.Get(), Style.Colors.Text);

                MaxLayerId = LayerId + 3;
            }
        }
    }

    OutCommandList.PopClip(MaxLayerId);
    return MaxLayerId;
}

FEventResponse FHistogram::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const IntVector2 ClientPosition = CursorEvent.GetClientPosition();

    HoveredSample = ResolveSampleAt(ClientPosition, false);

    if (bIsScrubbing)
    {
        const int32 Index = ResolveSampleAt(ClientPosition, true);
        if (Index != InvalidSampleIndex)
        {
            SelectedSample = Index;
        }

        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FHistogram::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Index = ResolveSampleAt(CursorEvent.GetClientPosition(), false);
    if (Index == InvalidSampleIndex)
    {
        return FEventResponse::Unhandled();
    }

    HoveredSample  = Index;
    SelectedSample = Index;
    bIsScrubbing   = true;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FHistogram::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft || !bIsScrubbing)
    {
        return FEventResponse::Unhandled();
    }

    bIsScrubbing = false;
    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FHistogram::OnMouseLeft(const FCursorEvent& /* CursorEvent */)
{
    HoveredSample = InvalidSampleIndex;
    return FEventResponse::Unhandled();
}

void FHistogram::AddSample(float InValue)
{
    Samples[(OldestSample + NumSamples) % Capacity] = InValue;

    if (NumSamples < Capacity)
    {
        ++NumSamples;
    }
    else
    {
        OldestSample = (OldestSample + 1) % Capacity;
    }
}

void FHistogram::Clear()
{
    Samples.Fill(0.0f);

    OldestSample   = 0;
    NumSamples     = 0;
    HoveredSample  = InvalidSampleIndex;
    SelectedSample = InvalidSampleIndex;
}

float FHistogram::GetSample(int32 Index) const
{
    if (Index < 0 || Index >= NumSamples)
    {
        return 0.0f;
    }

    return Samples[(OldestSample + Index) % Capacity];
}

float FHistogram::GetLatest() const
{
    return GetSample(NumSamples - 1);
}

float FHistogram::GetAverage() const
{
    if (NumSamples <= 0)
    {
        return 0.0f;
    }

    float Total = 0.0f;
    for (int32 Index = 0; Index < NumSamples; ++Index)
    {
        Total += GetSample(Index);
    }

    return Total / static_cast<float>(NumSamples);
}

float FHistogram::GetMaximum() const
{
    if (NumSamples <= 0)
    {
        return 0.0f;
    }

    float Maximum = GetSample(0);
    for (int32 Index = 1; Index < NumSamples; ++Index)
    {
        Maximum = Math::Max(Maximum, GetSample(Index));
    }

    return Maximum;
}

void FHistogram::SetRange(float InMin, float InMax)
{
    MinValue = InMin;
    MaxValue = Math::Max(InMax, InMin);
}

void FHistogram::SetAutoScale(bool bInAutoScale)
{
    bAutoScale = bInAutoScale;
}

void FHistogram::SetSelectedSample(int32 Index)
{
    SelectedSample = (Index >= 0 && Index < NumSamples) ? Index : InvalidSampleIndex;
}

float FHistogram::ResolveUpperBound() const
{
    const float UpperBound = bAutoScale ? Math::Max(MaxValue, GetMaximum()) : MaxValue;
    return Math::Max(UpperBound, MinValue);
}

int32 FHistogram::ResolveColumnWidth(int32 AvailableWidth) const
{
    return Math::Max(AvailableWidth / Math::Max(Capacity, 1), 1);
}

int32 FHistogram::ResolveSampleAt(const IntVector2& ClientPosition, bool bClampToStrip) const
{
    if (NumSamples <= 0)
    {
        return InvalidSampleIndex;
    }

    const FRectangle Bounds = GetContentRectangle();
    const int32      Offset = ClientPosition.X - Bounds.Position.X;
    const int32      Index  = Offset >= 0 ? (Offset / ResolveColumnWidth(Bounds.Width)) : -1;

    if (bClampToStrip)
    {
        return Math::Clamp(Index, 0, NumSamples - 1);
    }

    if (!Bounds.EncapsulatesPoint(ClientPosition) || Index < 0 || Index >= NumSamples)
    {
        return InvalidSampleIndex;
    }

    return Index;
}
