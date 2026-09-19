#include "Application/Elements/ScrollBar.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FScrollBar> FScrollBar::Create(const FDesc& Desc)
{
    TSharedPtr<FScrollBar> NewScrollBar = MakeSharedPtr<FScrollBar>();
    NewScrollBar->Initialize(Desc);
    return NewScrollBar;
}

FScrollBar::FScrollBar()
    : FInteractiveElement()
    , Orientation(EOrientation::Vertical)
    , Thickness(12)
    , MinThumbLength(24)
    , TrackPadding()
    , Style(FUIStyle::GetDefault().ScrollBar)
    , ContentLength(0)
    , ViewLength(0)
    , Offset(0)
    , ThumbGrabOffset(0)
    , Opacity(1.0f)
    , OnOffsetChangedDelegate()
{
}

FScrollBar::~FScrollBar() = default;

void FScrollBar::Initialize(const FDesc& Desc)
{
    Orientation             = Desc.Orientation;
    Thickness               = Math::Max(Desc.Thickness, 1);
    MinThumbLength          = Math::Max(Desc.MinThumbLength, 1);
    TrackPadding            = Desc.TrackPadding;
    Style                   = Desc.Style;
    OnOffsetChangedDelegate = Desc.OnOffsetChanged;
}

IntVector2 FScrollBar::ComputeDesiredSize() const
{
    return Orientation == EOrientation::Vertical ? IntVector2(Thickness, 0) : IntVector2(0, Thickness);
}

int32 FScrollBar::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle Bounds = AllottedGeometry.Bounds;
    const FCornerRadii TrackRadii(Style.CornerRadius);

    OutCommandList.AddBox(LayerId, Bounds, ApplyOpacity(Style.Track));

    if (!IsScrollable())
    {
        return LayerId;
    }

    const bool         bIsGrabbed = IsHovered() || IsPressed();
    const FRectangle   Thumb      = ComputeThumbBounds(AllottedGeometry.Bounds);
    const FFloatColor& ThumbFill  = bIsGrabbed ? Style.GrabActive : Style.Grab;

    OutCommandList.AddBox(LayerId, Thumb, ApplyOpacity(ThumbFill), TrackRadii);

    return LayerId;
}

void FScrollBar::SetOpacity(float InOpacity)
{
    Opacity = Math::Clamp(InOpacity, 0.0f, 1.0f);
}

FFloatColor FScrollBar::ApplyOpacity(const FFloatColor& Color) const
{
    FFloatColor Result = Color;
    Result.A *= Opacity;

    return Result;
}

FEventResponse FScrollBar::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response = FInteractiveElement::OnMouseButtonDown(CursorEvent);
    if (!IsPressed())
    {
        return Response;
    }

    const FRectangle Thumb    = GetThumbBounds();
    const int32      Position = Orientation == EOrientation::Vertical ? CursorEvent.GetClientPosition().Y : CursorEvent.GetClientPosition().X;
    const int32      Start    = Orientation == EOrientation::Vertical ? Thumb.Position.Y : Thumb.Position.X;
    const int32      Extent   = Orientation == EOrientation::Vertical ? Thumb.Height : Thumb.Width;

    if (Thumb.EncapsulatesPoint(CursorEvent.GetClientPosition()))
    {
        ThumbGrabOffset = Position - Start;
    }
    else
    {
        ThumbGrabOffset = Extent / 2;
        SetOffsetFromPosition(CursorEvent.GetClientPosition());
    }

    return Response;
}

void FScrollBar::SetScrollState(int32 InContentLength, int32 InViewLength, int32 InOffset)
{
    ContentLength = Math::Max(InContentLength, 0);
    ViewLength    = Math::Max(InViewLength, 0);
    Offset        = Math::Clamp(InOffset, 0, GetMaxOffset());
}

void FScrollBar::SetOffset(int32 InOffset)
{
    Offset = Math::Clamp(InOffset, 0, GetMaxOffset());
}

int32 FScrollBar::GetMaxOffset() const
{
    return Math::Max(ContentLength - ViewLength, 0);
}

float FScrollBar::GetVisibleFraction() const
{
    if (ContentLength <= 0)
    {
        return 1.0f;
    }

    return Math::Saturate(static_cast<float>(ViewLength) / static_cast<float>(ContentLength));
}

bool FScrollBar::IsScrollable() const
{
    return GetMaxOffset() > 0;
}

FRectangle FScrollBar::GetThumbBounds() const
{
    return ComputeThumbBounds(GetContentRectangle());
}

void FScrollBar::OnDragged(const FCursorEvent& CursorEvent)
{
    SetOffsetFromPosition(CursorEvent.GetClientPosition());
}

FRectangle FScrollBar::ComputeTrackBounds(const FRectangle& Bounds) const
{
    return Bounds.Deflate(TrackPadding);
}

FRectangle FScrollBar::ComputeThumbBounds(const FRectangle& Bounds) const
{
    const FRectangle Track       = ComputeTrackBounds(Bounds);
    const int32      TrackExtent = Orientation == EOrientation::Vertical ? Track.Height : Track.Width;
    const int32      ThumbExtent = Math::Clamp(Math::RoundToInt(GetVisibleFraction() * static_cast<float>(TrackExtent)), Math::Min(MinThumbLength, TrackExtent), TrackExtent);
    const int32      MaxOffset   = GetMaxOffset();
    const int32      Travel      = Math::Max(TrackExtent - ThumbExtent, 0);
    const int32      Distance    = MaxOffset > 0 ? Math::RoundToInt((static_cast<float>(Offset) / static_cast<float>(MaxOffset)) * static_cast<float>(Travel)) : 0;

    FRectangle Thumb = Track;
    if (Orientation == EOrientation::Vertical)
    {
        Thumb.Height = ThumbExtent;
        Thumb.Position.Y += Distance;
    }
    else
    {
        Thumb.Width = ThumbExtent;
        Thumb.Position.X += Distance;
    }

    return Thumb;
}

int32 FScrollBar::GetThumbTravel(const FRectangle& Bounds) const
{
    const FRectangle Track = ComputeTrackBounds(Bounds);
    const FRectangle Thumb = ComputeThumbBounds(Bounds);

    return Orientation == EOrientation::Vertical ? Math::Max(Track.Height - Thumb.Height, 0) : Math::Max(Track.Width - Thumb.Width, 0);
}

void FScrollBar::ApplyOffset(int32 InOffset)
{
    const int32 NewOffset = Math::Clamp(InOffset, 0, GetMaxOffset());
    if (NewOffset == Offset)
    {
        return;
    }

    Offset = NewOffset;
    OnOffsetChangedDelegate.ExecuteIfBound(Offset);
}

void FScrollBar::SetOffsetFromPosition(const IntVector2& ClientPosition)
{
    const FRectangle Bounds = GetContentRectangle();
    const int32      Travel = GetThumbTravel(Bounds);

    if (Travel <= 0)
    {
        return;
    }

    const FRectangle Track = ComputeTrackBounds(Bounds);

    const int32 Position = (Orientation == EOrientation::Vertical
        ? ClientPosition.Y - Track.Position.Y
        : ClientPosition.X - Track.Position.X) - ThumbGrabOffset;

    ApplyOffset(Math::RoundToInt((static_cast<float>(Math::Clamp(Position, 0, Travel)) / static_cast<float>(Travel)) * static_cast<float>(GetMaxOffset())));
}
