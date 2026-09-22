#include "Application/Elements/ProfilerTimeline.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/ScrollBar.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/ToolTipService.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Templates/NumericLimits.h"
#include "Core/Time/Time.h"

static constexpr int32 PROFILER_LANE_LABEL_WIDTH   = 96;
static constexpr int32 PROFILER_MIN_ROW_HEIGHT     = 16;
static constexpr int32 PROFILER_ROW_TEXT_MARGIN    = 3;
static constexpr int32 PROFILER_LANE_PADDING       = 5;
static constexpr int32 PROFILER_BAR_LABEL_PADDING  = 4;
static constexpr int32 PROFILER_RULER_LABEL_GAP    = 110;
static constexpr int32 PROFILER_SCROLL_RESOLUTION  = 1000000;
static constexpr float PROFILER_MIN_VIEW_NS        = 1000.0f;
static constexpr float PROFILER_WHEEL_PAN_FRACTION = 0.15f;

static FFloatColor FromBytes(int32 R, int32 G, int32 B, int32 A = 255)
{
    constexpr float Scale = 1.0f / 255.0f;
    return FFloatColor(static_cast<float>(R) * Scale, static_cast<float>(G) * Scale,
        static_cast<float>(B) * Scale, static_cast<float>(A) * Scale);
}

static uint32 HashScopeName(const CHAR* Name)
{
    uint32 Hash = 2166136261u;
    if (!Name)
    {
        return Hash;
    }

    while (*Name)
    {
        Hash ^= static_cast<uint8>(*Name++);
        Hash *= 16777619u;
    }

    return Hash;
}

static FFloatColor GetBarFill(const CHAR* Name, bool bIsGpu, int32 Depth)
{
    const float Hue          = static_cast<float>(HashScopeName(Name) % 360u) / 60.0f;
    const float Saturation   = bIsGpu ? 0.68f : 0.58f;
    const float Value        = Math::Clamp((bIsGpu ? 0.82f : 0.74f) - (static_cast<float>(Math::Min(Depth, 5)) * 0.025f), 0.55f, 0.85f);
    const float Chroma       = Value * Saturation;
    const float HueModuloTwo = Hue - static_cast<float>(static_cast<int32>(Hue / 2.0f) * 2);
    const float X            = Chroma * (1.0f - Math::Abs(HueModuloTwo - 1.0f));
    const float Match        = Value - Chroma;

    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;

    const int32 Sector = static_cast<int32>(Hue);
    switch (Sector)
    {
        case 0: 
            R = Chroma;
            G = X;
            break;

        case 1:
            R = X;
            G = Chroma;
            break;

        case 2:
            G = Chroma;
            B = X;
            break;

        case 3:
            G = X;
            B = Chroma;
            break;

        case 4:
            R = X;
            B = Chroma;
            break;

        default:
            R = Chroma;
            B = X;
            break;
    }

    return FFloatColor(R + Match, G + Match, B + Match, 1.0f);
}

static FFloatColor GetBarLabelColor(const FFloatColor& Fill)
{
    const float Luminance = (0.2126f * Fill.R) + (0.7152f * Fill.G) + (0.0722f * Fill.B);
    return (Luminance > 0.55f) ? FromBytes(16, 16, 18) : FromBytes(238, 238, 242);
}

static String FormatTimeLabel(double Nanoseconds, double StepNanoseconds)
{
    if (StepNanoseconds >= 1000000.0)
    {
        return String::Printf("%.1f ms", Time::ToMilliseconds(Nanoseconds));
    }

    if (StepNanoseconds >= 1000.0)
    {
        return String::Printf("%.0f us", Time::ToMicroseconds(Nanoseconds));
    }

    return String::Printf("%.0f ns", Nanoseconds);
}

TSharedPtr<FProfilerTimeline> FProfilerTimeline::Create(const FDesc& Desc)
{
    TSharedPtr<FProfilerTimeline> Timeline = MakeSharedPtr<FProfilerTimeline>();
    Timeline->Initialize(Desc);
    return Timeline;
}

FProfilerTimeline::FProfilerTimeline()
    : FInteractiveElement()
    , Font(nullptr)
    , HorizontalScrollBar(nullptr)
    , Lanes()
    , OnBarSelected()
    , OnGetBarContextMenu()
    , TotalSpanNanoseconds(0)
    , PreferredHeight(180)
    , SelectedLane(InvalidIndex)
    , SelectedBar(InvalidIndex)
    , HoveredLane(InvalidIndex)
    , HoveredBar(InvalidIndex)
    , ViewStartNs(0.0f)
    , ViewSpanNs(PROFILER_MIN_VIEW_NS)
    , DragStartView(0.0f)
    , DragOrigin()
    , bDragging(false)
    , bViewIsUserAdjusted(false)
{
}

FProfilerTimeline::~FProfilerTimeline() = default;

void FProfilerTimeline::Initialize(const FDesc& Desc)
{
    Font                = Desc.Font;
    OnBarSelected       = Desc.OnBarSelected;
    OnGetBarContextMenu = Desc.OnGetBarContextMenu;
    PreferredHeight     = Math::Max(Desc.PreferredHeight, 64);

    FScrollBar::FDesc ScrollDesc;
    ScrollDesc.Orientation     = EOrientation::Horizontal;
    ScrollDesc.OnOffsetChanged = FOnScrollBarOffsetChanged::CreateRaw(this, &FProfilerTimeline::OnScrollBarMoved);

    HorizontalScrollBar = FScrollBar::Create(ScrollDesc);
    if (HorizontalScrollBar)
    {
        HorizontalScrollBar->SetParentElement(AsWeakPtr());
    }
}

int32 FProfilerTimeline::GetLaneRowCount(const FProfilerTimelineLane& Lane)
{
    int32 MaxDepth = 0;
    for (const FProfilerTimelineBar& Bar : Lane.Bars)
    {
        MaxDepth = Math::Max(MaxDepth, Bar.Depth);
    }

    return MaxDepth + 1;
}

int32 FProfilerTimeline::GetRowHeight() const
{
    const int32 TextHeight = Font ? (Font->GetAscent() + Font->GetDescent()) : 0;
    return Math::Max(PROFILER_MIN_ROW_HEIGHT, TextHeight + PROFILER_ROW_TEXT_MARGIN);
}

int32 FProfilerTimeline::GetRulerHeight() const
{
    return GetRowHeight() + 2;
}

int32 FProfilerTimeline::GetLaneHeight(const FProfilerTimelineLane& Lane) const
{
    return (GetLaneRowCount(Lane) * GetRowHeight()) + PROFILER_LANE_PADDING;
}

IntVector2 FProfilerTimeline::ComputeDesiredSize() const
{
    int32 Height = GetRulerHeight() + PROFILER_LANE_PADDING;
    for (const FProfilerTimelineLane& Lane : Lanes)
    {
        Height += GetLaneHeight(Lane);
    }

    if (IsHorizontalScrollBarVisible())
    {
        Height += FUIStyle::GetDefault().Metrics.ScrollBarThickness;
    }

    return IntVector2(320, Math::Max(Height, PreferredHeight));
}

void FProfilerTimeline::OnArrange(const FRectangle& /* AllottedBounds */)
{
    if (HorizontalScrollBar)
    {
        SyncScrollBar();
    }
}

void FProfilerTimeline::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (HorizontalScrollBar)
    {
        OutChildren.Add(HorizontalScrollBar);
    }
}

void FProfilerTimeline::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (HorizontalScrollBar && IsHorizontalScrollBarVisible()
        && HorizontalScrollBar->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        HorizontalScrollBar->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

FRectangle FProfilerTimeline::GetTrackBounds(const FRectangle& Bounds) const
{
    FRectangle Track  = Bounds;
    Track.Position.X += PROFILER_LANE_LABEL_WIDTH;
    Track.Width       = Math::Max(Track.Width - PROFILER_LANE_LABEL_WIDTH, 1);
    return Track;
}

FRectangle FProfilerTimeline::GetLanesBounds(const FRectangle& Bounds) const
{
    const int32 RulerHeight = GetRulerHeight();

    FRectangle Rows  = Bounds;
    Rows.Position.Y += RulerHeight;
    Rows.Height      = Math::Max(Rows.Height - RulerHeight, 1);

    if (IsHorizontalScrollBarVisible())
    {
        Rows.Height = Math::Max(Rows.Height - FUIStyle::GetDefault().Metrics.ScrollBarThickness, 1);
    }

    return Rows;
}

FRectangle FProfilerTimeline::GetScrollBarBounds(const FRectangle& Bounds) const
{
    const int32 Thickness = FUIStyle::GetDefault().Metrics.ScrollBarThickness;

    FRectangle ScrollBounds = GetTrackBounds(Bounds);
    ScrollBounds.Position.Y = Bounds.GetBottom() - Thickness;
    ScrollBounds.Height     = IsHorizontalScrollBarVisible() ? Thickness : 0;
    return ScrollBounds;
}

int32 FProfilerTimeline::GetLaneTop(const FRectangle& Bounds, int32 LaneIndex) const
{
    int32 Top = GetLanesBounds(Bounds).Position.Y + PROFILER_LANE_PADDING;
    for (int32 Index = 0; Index < LaneIndex && Index < Lanes.Size(); ++Index)
    {
        Top += GetLaneHeight(Lanes[Index]);
    }

    return Top;
}

void FProfilerTimeline::SetLanes(TArray<FProfilerTimelineLane> InLanes)
{
    const IntVector2 PreviousDesiredSize = ComputeDesiredSize();
    Lanes = Move(InLanes);

    uint64 MinStart = TNumericLimits<uint64>::Max();
    uint64 MaxEnd   = 0;

    for (const FProfilerTimelineLane& Lane : Lanes)
    {
        for (const FProfilerTimelineBar& Bar : Lane.Bars)
        {
            MinStart = Math::Min(MinStart, Bar.StartNanoseconds);
            MaxEnd   = Math::Max(MaxEnd, Bar.EndNanoseconds);
        }
    }

    if (MinStart == TNumericLimits<uint64>::Max())
    {
        TotalSpanNanoseconds = 0;
    }
    else
    {
        for (FProfilerTimelineLane& Lane : Lanes)
        {
            for (FProfilerTimelineBar& Bar : Lane.Bars)
            {
                Bar.StartNanoseconds -= MinStart;
                Bar.EndNanoseconds   -= Math::Min(Bar.EndNanoseconds, MinStart);
            }
        }

        TotalSpanNanoseconds = MaxEnd - MinStart;
    }

    if (SelectedLane < 0 || SelectedLane >= Lanes.Size() || SelectedBar < 0 || SelectedBar >= Lanes[SelectedLane].Bars.Size())
    {
        SelectedLane = InvalidIndex;
        SelectedBar  = InvalidIndex;
    }

    HoveredLane = InvalidIndex;
    HoveredBar  = InvalidIndex;

    if (ComputeDesiredSize() != PreviousDesiredSize)
    {
        InvalidateDesiredSize();
    }

    if (bViewIsUserAdjusted)
    {
        ClampView();
    }
    else
    {
        FitView();
    }
}

void FProfilerTimeline::SetSelectedBar(int32 LaneIndex, int32 BarIndex)
{
    if (LaneIndex < 0 || LaneIndex >= Lanes.Size() || BarIndex < 0 || BarIndex >= Lanes[LaneIndex].Bars.Size())
    {
        SelectedLane = InvalidIndex;
        SelectedBar  = InvalidIndex;
        return;
    }

    SelectedLane = LaneIndex;
    SelectedBar  = BarIndex;
}

void FProfilerTimeline::FitView()
{
    ViewStartNs = 0.0f;
    ViewSpanNs  = Math::Max(static_cast<float>(TotalSpanNanoseconds), PROFILER_MIN_VIEW_NS);

    SyncScrollBar();
}

void FProfilerTimeline::ResetView()
{
    bViewIsUserAdjusted = false;
    FitView();
}

void FProfilerTimeline::ClampView()
{
    const float Total = Math::Max(static_cast<float>(TotalSpanNanoseconds), PROFILER_MIN_VIEW_NS);
    ViewSpanNs  = Math::Clamp(ViewSpanNs, PROFILER_MIN_VIEW_NS, Total);
    ViewStartNs = Math::Clamp(ViewStartNs, 0.0f, Math::Max(Total - ViewSpanNs, 0.0f));

    SyncScrollBar();
}

bool FProfilerTimeline::IsHorizontalScrollBarVisible() const
{
    return TotalSpanNanoseconds > 0 && ViewSpanNs < static_cast<float>(TotalSpanNanoseconds);
}

void FProfilerTimeline::SyncScrollBar()
{
    if (!HorizontalScrollBar)
    {
        return;
    }

    if (!IsHorizontalScrollBarVisible())
    {
        HorizontalScrollBar->SetScrollState(PROFILER_SCROLL_RESOLUTION, PROFILER_SCROLL_RESOLUTION, 0);
        HorizontalScrollBar->Tick(GetScrollBarBounds(GetContentRectangle()));
        return;
    }

    const float Total = static_cast<float>(TotalSpanNanoseconds);

    const int32 View = Math::Clamp(Math::RoundToInt((ViewSpanNs / Total) * static_cast<float>(PROFILER_SCROLL_RESOLUTION)),
        1, PROFILER_SCROLL_RESOLUTION);

    const int32 Offset = Math::Clamp(Math::RoundToInt((ViewStartNs / Total) * static_cast<float>(PROFILER_SCROLL_RESOLUTION)),
        0, PROFILER_SCROLL_RESOLUTION - View);

    HorizontalScrollBar->SetScrollState(PROFILER_SCROLL_RESOLUTION, View, Offset);
    HorizontalScrollBar->Tick(GetScrollBarBounds(GetContentRectangle()));
}

void FProfilerTimeline::OnScrollBarMoved(int32 NewOffset)
{
    if (!IsHorizontalScrollBarVisible())
    {
        return;
    }

    ViewStartNs         = (static_cast<float>(NewOffset) / static_cast<float>(PROFILER_SCROLL_RESOLUTION)) * static_cast<float>(TotalSpanNanoseconds);
    bViewIsUserAdjusted = true;

    ClampView();
}

bool FProfilerTimeline::ProjectBar(const FRectangle& Track, const FProfilerTimelineBar& Bar, int32& OutLeft, int32& OutWidth) const
{
    const float Duration = (Bar.bInstant || Bar.EndNanoseconds <= Bar.StartNanoseconds)
        ? 0.0f
        : static_cast<float>(Bar.EndNanoseconds - Bar.StartNanoseconds);

    const float Start    = (static_cast<float>(Bar.StartNanoseconds) - ViewStartNs) / ViewSpanNs;
    const float End      = Start + (Duration / ViewSpanNs);

    if (End < 0.0f || Start > 1.0f)
    {
        return false;
    }

    const int32 TrackWidth = Math::Max(Track.Width, 1);
    const int32 Left       = Track.Position.X + Math::RoundToInt(Math::Max(Start, 0.0f) * static_cast<float>(TrackWidth));
    const int32 Right      = Track.Position.X + Math::RoundToInt(Math::Min(End, 1.0f) * static_cast<float>(TrackWidth));

    OutLeft  = Left;
    OutWidth = Bar.bInstant ? 3 : (Right - Left);
    return Bar.bInstant || OutWidth >= 1;
}

FProfilerTimeline::FHit FProfilerTimeline::HitTest(const IntVector2& ClientPosition) const
{
    FHit Hit;

    const FRectangle Bounds = GetContentRectangle();
    if (!Bounds.EncapsulatesPoint(ClientPosition))
    {
        return Hit;
    }

    const FRectangle Track = GetTrackBounds(Bounds);
    if (ClientPosition.X < Track.Position.X)
    {
        return Hit;
    }

    for (int32 LaneIndex = 0; LaneIndex < Lanes.Size(); ++LaneIndex)
    {
        const FProfilerTimelineLane& Lane = Lanes[LaneIndex];

        const int32 LaneTop = GetLaneTop(Bounds, LaneIndex);
        const int32 Row     = (ClientPosition.Y - LaneTop) / GetRowHeight();

        if (ClientPosition.Y < LaneTop || Row >= GetLaneRowCount(Lane))
        {
            continue;
        }

        for (int32 BarIndex = Lane.Bars.Size() - 1; BarIndex >= 0; --BarIndex)
        {
            const FProfilerTimelineBar& Bar = Lane.Bars[BarIndex];
            if (Bar.Depth != Row)
            {
                continue;
            }

            int32 Left  = 0;
            int32 Width = 0;

            if (!ProjectBar(Track, Bar, Left, Width))
            {
                continue;
            }

            if (ClientPosition.X >= Left - 2 && ClientPosition.X < (Left + Width + 2))
            {
                Hit.Lane = LaneIndex;
                Hit.Bar  = BarIndex;
                return Hit;
            }
        }

        return Hit;
    }

    return Hit;
}

void FProfilerTimeline::DrawRuler(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&  Style = FUIStyle::GetDefault();
    const FRectangle Track = GetTrackBounds(Bounds);

    const double Target = static_cast<double>(ViewSpanNs) * 
        (static_cast<double>(PROFILER_RULER_LABEL_GAP) / static_cast<double>(Math::Max(Track.Width, 1)));

    if (Target <= 0.0)
    {
        return;
    }

    double Magnitude = 1.0;
    while ((Magnitude * 10.0) <= Target)
    {
        Magnitude *= 10.0;
    }

    double Step = Magnitude;
    if (Step < Target)
    {
        Step = Magnitude * 2.0;
    }

    if (Step < Target)
    {
        Step = Magnitude * 5.0;
    }

    if (Step < Target)
    {
        Step = Magnitude * 10.0;
    }

    const FFloatColor GridColor   = FromBytes(38, 38, 38);
    const double      ViewEnd     = static_cast<double>(ViewStartNs) + static_cast<double>(ViewSpanNs);
    const int32       RulerHeight = GetRulerHeight();
    const int32       RulerBottom = Bounds.Position.Y + RulerHeight;

    double Tick = Math::Ceil(static_cast<double>(ViewStartNs) / Step) * Step;
    for (; Tick <= ViewEnd; Tick += Step)
    {
        const double Fraction = (Tick - static_cast<double>(ViewStartNs)) / static_cast<double>(ViewSpanNs);
        const int32  X        = Track.Position.X + Math::RoundToInt(static_cast<float>(Fraction) * static_cast<float>(Track.Width));

        OutCommandList.AddLine(LayerId, FRectangle(IntVector2(X, Bounds.Position.Y + RulerHeight - 4), 1, Bounds.Height - RulerHeight + 4), GridColor);

        if (Font)
        {
            const String Text      = FormatTimeLabel(Tick, Step);
            const int32  TextWidth = Font->MeasureWidth(StringView(Text.Data(), Text.Length()));

            if ((X + 3 + TextWidth) <= Bounds.GetRight())
            {
                const FRectangle LabelBounds(IntVector2(X + 3, Bounds.Position.Y + 1), TextWidth, RulerHeight - 2);
                OutCommandList.AddText(LayerId, LabelBounds, Text, Font.Get(), Style.Colors.TextDisabled);
            }
        }
    }

    OutCommandList.AddLine(LayerId, FRectangle(IntVector2(Bounds.Position.X, RulerBottom), Bounds.Width, 1), GridColor);
}

int32 FProfilerTimeline::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&  Style  = FUIStyle::GetDefault();
    const FRectangle Bounds = AllottedGeometry.Bounds;
    const FRectangle Track  = GetTrackBounds(Bounds);

    OutCommandList.PushClip(LayerId, Bounds);
    OutCommandList.AddBox(LayerId, Bounds, Style.InnerFrame.Fill, FCornerRadii(Style.InnerFrame.CornerRadius));
    OutCommandList.AddBox(LayerId, FRectangle(Bounds.Position, PROFILER_LANE_LABEL_WIDTH, Bounds.Height), FromBytes(18, 18, 18));

    const int32 GridLayer   = LayerId + 1;
    const int32 BarLayer    = LayerId + 2;
    const int32 TextLayer   = LayerId + 3;
    const int32 ScrollLayer = LayerId + 4;

    DrawRuler(Bounds, OutCommandList, GridLayer);

    if (Lanes.IsEmpty() && Font)
    {
        const String     Empty      = String("Nothing was captured for this frame");
        const IntVector2 Size       = IntVector2(Font->MeasureWidth(StringView(Empty.Data(), Empty.Length())), Font->GetLineHeight());
        const FRectangle TextBounds = FRectangle::AlignInBounds(Bounds, Size, EHorizontalAlignment::Center, EVerticalAlignment::Center);

        OutCommandList.AddText(TextLayer, TextBounds, Empty, Font.Get(), Style.Colors.TextDisabled);
        OutCommandList.PopClip(TextLayer);
        return TextLayer;
    }

    const FFloatColor SelectedFill = Style.Colors.Accent;
    const FFloatColor HoverFill    = FromBytes(96, 96, 100);
    const FFloatColor LaneBand     = FromBytes(30, 30, 30);
    const int32       RowHeight    = GetRowHeight();
    int32             LaneTop      = GetLanesBounds(Bounds).Position.Y + PROFILER_LANE_PADDING;

    for (int32 LaneIndex = 0; LaneIndex < Lanes.Size(); ++LaneIndex)
    {
        const FProfilerTimelineLane& Lane = Lanes[LaneIndex];

        const int32 LaneRowCount = GetLaneRowCount(Lane);
        const int32 LaneHeight   = LaneRowCount * RowHeight;
        const int32 NextLaneTop  = LaneTop + LaneHeight + PROFILER_LANE_PADDING;

        const FRectangle  LaneBounds = FRectangle(IntVector2(Bounds.Position.X, LaneTop), Bounds.Width, LaneHeight);
        const FRectangle& ClipBounds = OutCommandList.GetCurrentClipRectangle();

        if (!ClipBounds.IsEmpty() && ClipBounds.Intersect(LaneBounds).IsEmpty())
        {
            LaneTop = NextLaneTop;
            continue;
        }

        if ((LaneIndex % 2) == 1)
        {
            OutCommandList.AddBox(GridLayer, FRectangle(IntVector2(Track.Position.X, LaneTop), Track.Width, LaneHeight), LaneBand);
        }

        if (Font)
        {
            const FRectangle LabelBounds(IntVector2(Bounds.Position.X + 6, LaneTop), PROFILER_LANE_LABEL_WIDTH - 10, RowHeight);
            OutCommandList.AddText(TextLayer, LabelBounds,
                Font->ElideText(StringView(Lane.Label.Data(), Lane.Label.Length()), LabelBounds.Width), Font.Get(), Style.Colors.Text);

            if (!Lane.SubLabel.IsEmpty() && LaneRowCount > 1)
            {
                const FRectangle SubBounds(IntVector2(Bounds.Position.X + 6, LaneTop + RowHeight),
                    PROFILER_LANE_LABEL_WIDTH - 10, RowHeight);
                OutCommandList.AddText(TextLayer, SubBounds,
                    Font->ElideText(StringView(Lane.SubLabel.Data(), Lane.SubLabel.Length()), SubBounds.Width), Font.Get(), Style.Colors.TextDisabled);
            }
        }

        for (int32 BarIndex = 0; BarIndex < Lane.Bars.Size(); ++BarIndex)
        {
            const FProfilerTimelineBar& Bar = Lane.Bars[BarIndex];

            int32 Left  = 0;
            int32 Width = 0;

            if (!ProjectBar(Track, Bar, Left, Width))
            {
                continue;
            }

            FRectangle BarBounds;
            BarBounds.Position.X = Left;
            BarBounds.Position.Y = LaneTop + (Bar.Depth * RowHeight);
            BarBounds.Width      = Width;
            BarBounds.Height     = RowHeight - 1;

            const bool bBarIsSelected = (LaneIndex == SelectedLane) && (BarIndex == SelectedBar);
            const bool bBarIsHovered  = (LaneIndex == HoveredLane) && (BarIndex == HoveredBar);

            FFloatColor Fill = bBarIsSelected ? SelectedFill : (bBarIsHovered ? HoverFill : GetBarFill(Bar.Name, Lane.bIsGpu, Bar.Depth));
            if (Bar.bDimmed && !bBarIsSelected)
            {
                Fill.A *= 0.22f;
            }

            OutCommandList.AddBox(BarLayer, BarBounds, Fill, FCornerRadii(Bar.bInstant ? 0.0f : 2.0f));

            if (Font && Bar.Name && !Bar.bInstant && !(Bar.bDimmed && !bBarIsSelected))
            {
                const FRectangle TextBounds = BarBounds.Deflate(FMargin(PROFILER_BAR_LABEL_PADDING, 0, PROFILER_BAR_LABEL_PADDING, 0));
                if (TextBounds.Width > 0 && Font->MeasureWidth(StringView(Bar.Name)) <= TextBounds.Width)
                {
                    OutCommandList.AddText(TextLayer, TextBounds, StringView(Bar.Name), Font.Get(), GetBarLabelColor(Fill));
                }
            }
        }

        LaneTop = NextLaneTop;
    }

    OutCommandList.AddLine(GridLayer, FRectangle(IntVector2(Track.Position.X, Bounds.Position.Y), 1, Bounds.Height), FromBytes(38, 38, 38));

    int32 MaxLayer = TextLayer;
    if (HorizontalScrollBar && IsHorizontalScrollBarVisible())
    {
        const FDrawGeometry ScrollGeometry(HorizontalScrollBar->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayer = HorizontalScrollBar->OnDraw(ScrollGeometry, OutCommandList, ScrollLayer);
    }

    OutCommandList.PopClip(MaxLayer);
    return MaxLayer;
}

FEventResponse FProfilerTimeline::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() == Keys::MouseButtonMiddle ||
        (CursorEvent.GetKey() == Keys::MouseButtonLeft && CursorEvent.GetModifierKeys().IsAltDown()))
    {
        bDragging     = true;
        DragOrigin    = CursorEvent.GetClientPosition();
        DragStartView = ViewStartNs;

        if (FApplication::IsInitialized())
        {
            FApplication::Get().CaptureMouse(AsSharedPtr());
        }

        return FEventResponse::Handled();
    }

    if (CursorEvent.GetKey() == Keys::MouseButtonRight)
    {
        const FHit Hit = HitTest(CursorEvent.GetClientPosition());
        if (Hit.Lane != InvalidIndex && Hit.Bar != InvalidIndex && OnGetBarContextMenu.IsBound()
            && FApplication::IsInitialized())
        {
            TSharedPtr<FVisualElement> Menu = OnGetBarContextMenu.Execute(Hit.Lane, Hit.Bar);
            if (Menu)
            {
                FMenuStack::Get().PushMenu(AsSharedPtr(), FRectangle(CursorEvent.GetScreenPosition(), 0, 0), EMenuPlacement::AtCursor, Menu);
                return FEventResponse::Handled();
            }
        }

        ResetView();
        return FEventResponse::Handled();
    }

    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const FHit Hit = HitTest(CursorEvent.GetClientPosition());
    SetSelectedBar(Hit.Lane, Hit.Bar);

    OnBarSelected.ExecuteIfBound(SelectedLane, SelectedBar);
    return FEventResponse::Handled();
}

FEventResponse FProfilerTimeline::OnMouseButtonUp(const FCursorEvent& /* CursorEvent */)
{
    bDragging = false;
    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FProfilerTimeline::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (bDragging)
    {
        const FRectangle Track = GetTrackBounds(GetContentRectangle());
        const int32 DeltaX     = CursorEvent.GetClientPosition().X - DragOrigin.X;
        const float NsPerPixel = ViewSpanNs / static_cast<float>(Math::Max(Track.Width, 1));

        ViewStartNs         = DragStartView - (static_cast<float>(DeltaX) * NsPerPixel);
        bViewIsUserAdjusted = true;

        ClampView();
        return FEventResponse::Handled();
    }

    const FHit Hit = HitTest(CursorEvent.GetClientPosition());
    HoveredLane = Hit.Lane;
    HoveredBar  = Hit.Bar;

    UpdateBarToolTip(CursorEvent);
    return FInteractiveElement::OnMouseMove(CursorEvent);
}

FEventResponse FProfilerTimeline::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    const float Delta = CursorEvent.GetScrollDelta();
    if (Delta == 0.0f)
    {
        return FEventResponse::Unhandled();
    }

    if (CursorEvent.GetModifierKeys().IsShortcutChordDown())
    {
        ZoomAt(CursorEvent.GetClientPosition(), Delta > 0.0f ? 0.8f : 1.25f);
        return FEventResponse::Handled();
    }

    if (!IsHorizontalScrollBarVisible())
    {
        return FEventResponse::Unhandled();
    }

    ViewStartNs         -= Delta * ViewSpanNs * PROFILER_WHEEL_PAN_FRACTION;
    bViewIsUserAdjusted  = true;

    ClampView();
    return FEventResponse::Handled();
}

FEventResponse FProfilerTimeline::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    bDragging   = false;
    HoveredLane = InvalidIndex;
    HoveredBar  = InvalidIndex;

    FToolTipService::Get().CancelToolTip(AsSharedPtr());
    return FInteractiveElement::OnMouseLeft(CursorEvent);
}

String FProfilerTimeline::FormatBarToolTip(const FProfilerTimelineBar& Bar)
{
    const String Name = Bar.Name ? String(Bar.Name) : String("<unnamed>");
    if (Bar.bInstant)
    {
        return Name + "\nevent";
    }

    return Name + String::Printf("\n%.3f ms",
        Time::ToMilliseconds(static_cast<double>(Bar.EndNanoseconds - Bar.StartNanoseconds)));
}

void FProfilerTimeline::UpdateBarToolTip(const FCursorEvent& CursorEvent)
{
    FToolTipService& ToolTips = FToolTipService::Get();

    if (HoveredLane == InvalidIndex || HoveredBar == InvalidIndex)
    {
        ToolTips.CancelToolTip(AsSharedPtr());
        return;
    }

    ToolTips.NotifyCursorMoved(CursorEvent.GetScreenPosition());
    ToolTips.RequestTextToolTip(AsSharedPtr(), FormatBarToolTip(Lanes[HoveredLane].Bars[HoveredBar]), Font);
}

void FProfilerTimeline::ZoomAt(const IntVector2& ClientPosition, float Factor)
{
    const FRectangle Track   = GetTrackBounds(GetContentRectangle());
    const float      Pivot   = Math::Saturate(static_cast<float>(ClientPosition.X - Track.Position.X) / static_cast<float>(Math::Max(Track.Width, 1)));
    const float      PivotNs = ViewStartNs + (Pivot * ViewSpanNs);

    ViewSpanNs          = Math::Max(ViewSpanNs * Factor, PROFILER_MIN_VIEW_NS);
    ViewStartNs         = PivotNs - (Pivot * ViewSpanNs);
    bViewIsUserAdjusted = true;

    ClampView();

    if (ViewSpanNs >= static_cast<float>(TotalSpanNanoseconds))
    {
        ResetView();
    }
}
