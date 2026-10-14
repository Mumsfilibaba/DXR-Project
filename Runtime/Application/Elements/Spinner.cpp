#include "Application/Elements/Spinner.h"
#include "Application/Animation/UIAnimation.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

// How much of the ring the turning arc covers, as a fraction of a turn
static constexpr float ArcFraction = 0.3f;

TSharedPtr<FSpinner> FSpinner::Create(const FDesc& Desc)
{
    TSharedPtr<FSpinner> NewSpinner = MakeSharedPtr<FSpinner>();
    NewSpinner->Initialize(Desc);
    return NewSpinner;
}

FSpinner::FSpinner()
    : FVisualElement()
    , Extent(14)
    , Thickness(2.0f)
    , Color(FUIStyle::GetDefault().Colors.Accent)
    , TrackColor(FUIStyle::GetDefault().Colors.ControlNormal)
    , Period(1.0f)
    , bIsSpinning(true)
{
}

FSpinner::~FSpinner() = default;

void FSpinner::Initialize(const FDesc& Desc)
{
    Extent      = Math::Max(Desc.Extent, 4);
    Thickness   = Math::Max(Desc.Thickness, 1.0f);
    Color       = Desc.Color;
    TrackColor  = Desc.TrackColor;
    Period      = Math::Max(Desc.Period, 0.05f);
    bIsSpinning = Desc.bIsSpinning;
}

FIntVector2 FSpinner::ComputeDesiredSize() const
{
    return bIsSpinning ? FIntVector2(Extent, Extent) : FIntVector2(0, 0);
}

int32 FSpinner::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!bIsSpinning)
    {
        return LayerId;
    }

    const FRectangle Bounds = AllottedGeometry.Bounds;
    const float      Size   = static_cast<float>(Math::Min(Bounds.Width, Bounds.Height));
    const FVector2   Center(static_cast<float>(Bounds.Position.X) + static_cast<float>(Bounds.Width) * 0.5f, static_cast<float>(Bounds.Position.Y) + static_cast<float>(Bounds.Height) * 0.5f);
    const float      Radius = Math::Max((Size - Thickness) * 0.5f, 1.0f);

    const double Cycles = FUIFrameClock::Now() / static_cast<double>(Period);
    const float  Turn   = static_cast<float>(Cycles - static_cast<double>(static_cast<int64>(Cycles)));
    const float  Start  = Turn * Math::Constants::TwoPI;

    OutCommandList.AddCircle(LayerId, Center, Radius, TrackColor, Thickness);
    OutCommandList.AddArc(LayerId, Center, Radius, Start, Start + ArcFraction * Math::Constants::TwoPI, Color, Thickness);

    RequestContinuousPaint();
    return LayerId;
}

void FSpinner::SetSpinning(bool bInIsSpinning)
{
    if (bIsSpinning == bInIsSpinning)
    {
        return;
    }

    bIsSpinning = bInIsSpinning;
    InvalidateDesiredSize();
    InvalidatePaint();
}
