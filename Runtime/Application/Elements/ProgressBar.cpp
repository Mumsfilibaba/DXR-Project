#include "Application/Elements/ProgressBar.h"
#include "Application/Animation/UIAnimation.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

// How much of the track the sliding segment covers, and how long it takes to cross it, in seconds
static constexpr float  IndeterminateFraction = 0.3f;
static constexpr double IndeterminatePeriod   = 1.4;

TSharedPtr<FProgressBar> FProgressBar::Create(const FDesc& Desc)
{
    TSharedPtr<FProgressBar> NewProgressBar = MakeSharedPtr<FProgressBar>();
    NewProgressBar->Initialize(Desc);
    return NewProgressBar;
}

FProgressBar::FProgressBar()
    : FVisualElement()
    , OverlayText()
    , Font(nullptr)
    , FillColor(FUIStyle::GetDefault().Colors.Accent)
    , BackgroundColor(FUIStyle::GetDefault().Colors.ControlNormal)
    , TextColor(FUIStyle::GetDefault().Colors.Text)
    , CornerRadius(FUIStyle::GetDefault().Metrics.CornerRadius)
    , Percent(0.0f)
    , PreferredHeight(16)
    , bIndeterminate(false)
{
}

FProgressBar::~FProgressBar() = default;

void FProgressBar::Initialize(const FDesc& Desc)
{
    OverlayText     = Desc.OverlayText;
    Font            = Desc.Font;
    FillColor       = Desc.FillColor;
    BackgroundColor = Desc.BackgroundColor;
    TextColor       = Desc.TextColor;
    CornerRadius    = Desc.CornerRadius;
    PreferredHeight = Math::Max(Desc.PreferredHeight, 1);
    bIndeterminate  = Desc.bIndeterminate;

    Percent = Math::Saturate(Desc.Percent);
}

FIntVector2 FProgressBar::ComputeDesiredSize() const
{
    return FIntVector2(0, PreferredHeight);
}

int32 FProgressBar::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle Bounds = AllottedGeometry.Bounds;
    OutCommandList.AddBox(LayerId, Bounds, BackgroundColor, CornerRadius);

    if (bIndeterminate)
    {
        const double Cycles  = FUIFrameClock::Now() / IndeterminatePeriod;
        const float  Phase   = static_cast<float>(Cycles - static_cast<double>(static_cast<int64>(Cycles)));
        const int32  Segment = Math::Max(Math::RoundToInt(IndeterminateFraction * static_cast<float>(Bounds.Width)), 1);

        FRectangle Sliding = Bounds;
        Sliding.Position.X = Bounds.Position.X - Segment + Math::RoundToInt(Phase * static_cast<float>(Bounds.Width + Segment));
        Sliding.Width = Segment;

        OutCommandList.PushClip(LayerId, Bounds);
        OutCommandList.AddBox(LayerId, Sliding, FillColor, CornerRadius);
        OutCommandList.PopClip(LayerId);

        RequestContinuousPaint();
    }
    else
    {
        FRectangle Filled = Bounds;
        Filled.Width      = Math::Clamp(Math::RoundToInt(Percent * static_cast<float>(Bounds.Width)), 0, Bounds.Width);

        if (!Filled.IsEmpty())
        {
            OutCommandList.AddBox(LayerId, Filled, FillColor, CornerRadius);
        }
    }

    if (Font && !OverlayText.IsEmpty())
    {
        const FIntVector2 TextSize(Font->MeasureWidth(FStringView(OverlayText.Data(), OverlayText.Length())), Font->GetLineHeight());
        const FRectangle TextBounds = FLayout::AlignInBounds(Bounds, TextSize, EHorizontalAlignment::Center, EVerticalAlignment::Center);

        OutCommandList.AddText(LayerId + 1, TextBounds, OverlayText, Font.Get(), TextColor);
        return LayerId + 1;
    }

    return LayerId;
}

void FProgressBar::SetPercent(float InPercent)
{
    const float NewPercent = Math::Saturate(InPercent);
    if (Percent == NewPercent)
    {
        return;
    }

    Percent = NewPercent;
    InvalidatePaint();
}

void FProgressBar::SetIndeterminate(bool bInIndeterminate)
{
    if (bIndeterminate == bInIndeterminate)
    {
        return;
    }

    bIndeterminate = bInIndeterminate;
    InvalidatePaint();
}

void FProgressBar::SetOverlayText(const FString& InText)
{
    if (OverlayText == InText)
    {
        return;
    }

    OverlayText = InText;
    InvalidatePaint();
}

void FProgressBar::SetFillColor(const FFloatColor& InColor)
{
    FillColor = InColor;
    InvalidatePaint();
}
