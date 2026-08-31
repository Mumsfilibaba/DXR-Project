#include "Application/Elements/Histogram.h"
#include "Application/Draw/DrawCommandList.h"
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
    , BackgroundColor(FUIStyle::GetDefault().Colors.ControlPressed)
    , WarningColor(0.85f, 0.35f, 0.25f, 1.0f)
    , MinValue(0.0f)
    , MaxValue(0.0f)
    , WarningThreshold(0.0f)
    , Capacity(128)
    , OldestSample(0)
    , NumSamples(0)
    , PreferredHeight(64)
    , HoveredSample(InvalidSampleIndex)
    , bAutoScale(true)
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
            const bool bIsWarning = WarningThreshold > 0.0f && Value > WarningThreshold;
            OutCommandList.AddBox(LayerId + 1, Bar, bIsWarning ? WarningColor : BarColor);
        }
    }

    int32 MaxLayerId = LayerId + 1;

    if (Font)
    {
        const FRectangle TextBounds = Bounds.Deflate(FMargin(HISTOGRAM_TEXT_PADDING));
        const IntVector2 TextSize(TextBounds.Width, Font->GetLineHeight());

        if (!Label.IsEmpty())
        {
            const FRectangle LabelBounds = FRectangle::AlignInBounds(TextBounds, TextSize, EHorizontalAlignment::Left, EVerticalAlignment::Top);
            OutCommandList.AddText(LayerId + 2, LabelBounds, Label, Font.Get(), Style.Colors.Text);

            MaxLayerId = LayerId + 2;
        }

        if (HoveredSample != InvalidSampleIndex)
        {
            const String     Value = String::Printf("%.2f", GetSample(HoveredSample));
            const IntVector2 ValueSize(Font->MeasureWidth(StringView(Value.Data(), Value.Length())), Font->GetLineHeight());

            const FRectangle ValueBounds = FRectangle::AlignInBounds(TextBounds, ValueSize, EHorizontalAlignment::Right, EVerticalAlignment::Top);
            OutCommandList.AddText(LayerId + 2, ValueBounds, Value, Font.Get(), Style.Colors.Text);

            MaxLayerId = LayerId + 2;
        }
    }

    OutCommandList.PopClip(MaxLayerId);
    return MaxLayerId;
}

FEventResponse FHistogram::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const FRectangle Bounds         = GetContentRectangle();
    const IntVector2 ClientPosition = CursorEvent.GetClientPosition();

    HoveredSample = InvalidSampleIndex;

    if (Bounds.EncapsulatesPoint(ClientPosition))
    {
        const int32 Index = (ClientPosition.X - Bounds.Position.X) / ResolveColumnWidth(Bounds.Width);
        if (Index >= 0 && Index < NumSamples)
        {
            HoveredSample = Index;
        }
    }

    return FEventResponse::Unhandled();
}

FEventResponse FHistogram::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

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

    OldestSample  = 0;
    NumSamples    = 0;
    HoveredSample = InvalidSampleIndex;
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

float FHistogram::ResolveUpperBound() const
{
    const float UpperBound = bAutoScale ? Math::Max(MaxValue, GetMaximum()) : MaxValue;
    return Math::Max(UpperBound, MinValue);
}

int32 FHistogram::ResolveColumnWidth(int32 AvailableWidth) const
{
    return Math::Max(AvailableWidth / Math::Max(Capacity, 1), 1);
}
