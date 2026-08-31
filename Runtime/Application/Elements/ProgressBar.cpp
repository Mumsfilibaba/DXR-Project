#include "Application/Elements/ProgressBar.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

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
    , BackgroundColor(FUIStyle::GetDefault().Colors.ControlPressed)
    , TextColor(FUIStyle::GetDefault().Colors.Text)
    , CornerRadius()
    , Percent(0.0f)
    , PreferredHeight(16)
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

    Percent = Math::Saturate(Desc.Percent);
}

IntVector2 FProgressBar::ComputeDesiredSize() const
{
    return IntVector2(0, PreferredHeight);
}

int32 FProgressBar::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle Bounds = AllottedGeometry.Bounds;
    OutCommandList.AddBox(LayerId, Bounds, BackgroundColor, CornerRadius);

    FRectangle Filled = Bounds;
    Filled.Width      = Math::Clamp(Math::RoundToInt(Percent * static_cast<float>(Bounds.Width)), 0, Bounds.Width);

    if (!Filled.IsEmpty())
    {
        OutCommandList.AddBox(LayerId, Filled, FillColor, CornerRadius);
    }

    if (Font && !OverlayText.IsEmpty())
    {
        const IntVector2 TextSize(Font->MeasureWidth(StringView(OverlayText.Data(), OverlayText.Length())), Font->GetLineHeight());
        const FRectangle TextBounds = FRectangle::AlignInBounds(Bounds, TextSize, EHorizontalAlignment::Center, EVerticalAlignment::Center);

        OutCommandList.AddText(LayerId + 1, TextBounds, OverlayText, Font.Get(), TextColor);
        return LayerId + 1;
    }

    return LayerId;
}

void FProgressBar::SetPercent(float InPercent)
{
    Percent = Math::Saturate(InPercent);
}

void FProgressBar::SetOverlayText(const String& InText)
{
    OverlayText = InText;
}

void FProgressBar::SetFillColor(const FFloatColor& InColor)
{
    FillColor = InColor;
}
