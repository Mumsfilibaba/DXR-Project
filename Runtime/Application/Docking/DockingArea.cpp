#include "Application/Application.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockLayoutFile.h"
#include "Application/Docking/DockWindowManager.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Docking/Splitter.h"
#include "Application/Docking/TabStrip.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/DrawCache.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/IApplicationRenderer.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 DROP_INDICATOR_INSET   = 2;
constexpr int32 DROP_ZONE_CHIP_SIZE    = 96;
constexpr int32 DROP_ZONE_CHIP_GAP     = 8;
constexpr int32 DROP_ZONE_CHIP_MINIMUM = 12;

// The drop zones are tinted with the selection blue, see DrawDropZones, at these opacities
constexpr float DROP_ZONE_CHIP_OPACITY          = 0.20f;
constexpr float DROP_ZONE_HOVER_OPACITY         = 0.40f;
constexpr float DROP_ZONE_CHIP_OUTLINE_OPACITY  = 0.50f;
constexpr float DROP_ZONE_HOVER_OUTLINE_OPACITY = 0.80f;
constexpr float DROP_ZONE_GHOST_OPACITY         = 0.75f;
constexpr float DROP_ZONE_LANDING_OPACITY       = 0.20f;

// How far the corners of a drop zone chip are rounded, the landing preview using the panel's own radius
constexpr float DROP_ZONE_CHIP_CORNER_RADIUS = 6.0f;

// How long the drop preview takes to animate, in seconds
constexpr float DROP_LANDING_FADE_SECONDS = 0.12f;
constexpr float DROP_LANDING_MOVE_SECONDS = 0.15f;
constexpr float DROP_SETTLE_SECONDS       = 0.30f;
constexpr float DROP_CHIPS_FADE_SECONDS   = 0.12f;
constexpr float DROP_CHIP_HOVER_SECONDS   = 0.10f;
constexpr float DROP_TEAR_OUT_SECONDS     = 0.20f;

// How far a landing preview swings past a new target before settling on it
constexpr float DROP_LANDING_OVERSHOOT = 1.2f;

static FRectangle LerpRectangle(const FRectangle& From, const FRectangle& To, float Alpha)
{
    const int32 Left   = Math::RoundToInt(Math::Lerp(static_cast<float>(From.Position.X), static_cast<float>(To.Position.X), Alpha));
    const int32 Top    = Math::RoundToInt(Math::Lerp(static_cast<float>(From.Position.Y), static_cast<float>(To.Position.Y), Alpha));
    const int32 Right  = Math::RoundToInt(Math::Lerp(static_cast<float>(From.GetRight()), static_cast<float>(To.GetRight()), Alpha));
    const int32 Bottom = Math::RoundToInt(Math::Lerp(static_cast<float>(From.GetBottom()), static_cast<float>(To.GetBottom()), Alpha));
    return FRectangle(IntVector2(Left, Top), Math::Max(Right - Left, 0), Math::Max(Bottom - Top, 0));
}

constexpr int32 PANEL_OUTLINE_ARC_SEGMENTS = 8;

struct FPanelOutline
{
    /** @brief The body of the panel, which starts on the strip's bottom row so the outline runs along that row. */
    FRectangle BodyBounds;

    /** @brief How far each corner of the body is rounded, a top corner being square where the active tab sits flush with it. */
    FCornerRadii BodyRadii;

    /** @brief The part of the active tab that shows, or empty when the panel shows no strip or no active tab. */
    FRectangle TabBounds;

    /** @brief The panel's border thickness in whole pixels, which is how far the outline sits outside the tab. */
    int32 Border = 0;

    /** @brief How far the tab's flares reach out from its bottom corners, zero on a side where the tab is flush with the body. */
    int32 FlareLeft  = 0;
    int32 FlareRight = 0;
};

static FPanelOutline ComputePanelOutline(const FRectangle& FrameBounds, const FTabStrip* Strip, const FUITabStyle& TabStyle, const FUIPanelChromeStyle& PanelStyle)
{
    FPanelOutline Outline;
    Outline.BodyBounds = FrameBounds;
    Outline.BodyRadii  = FCornerRadii(PanelStyle.CornerRadius);
    Outline.Border     = Math::Max(Math::RoundToInt(PanelStyle.BorderThickness), 0);

    if (!Strip)
    {
        return Outline;
    }

    const FRectangle StripBounds = Strip->GetContentRectangle();
    if (StripBounds.IsEmpty())
    {
        return Outline;
    }

    const int32 BodyTop = Math::Clamp(StripBounds.GetBottom() - Outline.Border, StripBounds.Position.Y, FrameBounds.GetBottom());
    Outline.BodyBounds = FRectangle(IntVector2(FrameBounds.Position.X, BodyTop), FrameBounds.Width, FrameBounds.GetBottom() - BodyTop);

    const FRectangle TabBounds = Strip->GetActiveTabRectangle();
    if (TabBounds.IsEmpty())
    {
        return Outline;
    }

    const int32 Flare      = Math::Max(Math::RoundToInt(TabStyle.FlareRadius), 0);
    const int32 CornerRoom = Math::RoundToInt(PanelStyle.CornerRadius);
    const int32 BodyLeft   = Outline.BodyBounds.Position.X;
    const int32 BodyRight  = Outline.BodyBounds.GetRight();

    Outline.TabBounds = TabBounds;

    if (TabBounds.Position.X - Outline.Border - Flare < BodyLeft + CornerRoom)
    {
        Outline.BodyRadii.TopLeft = 0.0f;
    }
    else
    {
        Outline.FlareLeft = Flare;
    }

    if (TabBounds.GetRight() + Outline.Border + Flare > BodyRight - CornerRoom)
    {
        Outline.BodyRadii.TopRight = 0.0f;
    }
    else
    {
        Outline.FlareRight = Flare;
    }

    return Outline;
}

static void AppendOutlinePoint(TArray<Vector2>& OutPoints, const Vector2& Point)
{
    if (OutPoints.IsEmpty() || (OutPoints.Last() - Point).GetLengthSquared() > 1.0e-4f)
    {
        OutPoints.Add(Point);
    }
}

static void AppendOutlineArc(TArray<Vector2>& OutPoints, const Vector2& Center, float Radius, float StartAngle, float EndAngle)
{
    if (Radius <= 0.0f)
    {
        AppendOutlinePoint(OutPoints, Center);
        return;
    }

    for (int32 Index = 0; Index <= PANEL_OUTLINE_ARC_SEGMENTS; ++Index)
    {
        const float Angle = StartAngle + (EndAngle - StartAngle) * (static_cast<float>(Index) / static_cast<float>(PANEL_OUTLINE_ARC_SEGMENTS));
        AppendOutlinePoint(OutPoints, Center + Vector2(Math::Cos(Angle), Math::Sin(Angle)) * Radius);
    }
}

static void BuildPanelOutline(const FPanelOutline& Outline, float TabCornerRadius, float Thickness, TArray<Vector2>& OutPoints)
{
    OutPoints.Clear();

    constexpr float QuarterTurn = Math::Constants::HalfPI;

    const float HalfThickness = Thickness * 0.5f;
    const float Border        = static_cast<float>(Outline.Border);

    const FRectangle&   Body   = Outline.BodyBounds;
    const FCornerRadii& Radius = Outline.BodyRadii;

    const float Left   = static_cast<float>(Body.Position.X);
    const float Top    = static_cast<float>(Body.Position.Y);
    const float Right  = static_cast<float>(Body.GetRight());
    const float Bottom = static_cast<float>(Body.GetBottom());

    const float InnerLeft   = Left + HalfThickness;
    const float InnerTop    = Top + HalfThickness;
    const float InnerRight  = Right - HalfThickness;
    const float InnerBottom = Bottom - HalfThickness;

    if (Radius.TopLeft > 0.0f)
    {
        AppendOutlineArc(OutPoints, Vector2(Left + Radius.TopLeft, Top + Radius.TopLeft), Radius.TopLeft - HalfThickness, 2.0f * QuarterTurn, 3.0f * QuarterTurn);
    }
    else
    {
        AppendOutlinePoint(OutPoints, Vector2(InnerLeft, InnerTop));
    }

    const FRectangle& Tab = Outline.TabBounds;
    if (!Tab.IsEmpty())
    {
        const float TabLeft     = static_cast<float>(Tab.Position.X);
        const float TabTop      = static_cast<float>(Tab.Position.Y);
        const float TabRight    = static_cast<float>(Tab.GetRight());
        const float StripBottom = Top + Border;

        if (Outline.FlareLeft > 0)
        {
            const float Flare = static_cast<float>(Outline.FlareLeft);
            AppendOutlineArc(OutPoints, Vector2(TabLeft - Flare, StripBottom - Flare), Flare - Border + HalfThickness, QuarterTurn, 0.0f);
        }
        else
        {
            AppendOutlinePoint(OutPoints, Vector2(TabLeft - Border + HalfThickness, InnerTop));
        }

        const float CornerRadius  = Math::Max(TabCornerRadius, 0.0f);
        const float OutlineRadius = CornerRadius + Border - HalfThickness;
        AppendOutlineArc(OutPoints, Vector2(TabLeft + CornerRadius, TabTop + CornerRadius), OutlineRadius, 2.0f * QuarterTurn, 3.0f * QuarterTurn);
        AppendOutlineArc(OutPoints, Vector2(TabRight - CornerRadius, TabTop + CornerRadius), OutlineRadius, 3.0f * QuarterTurn, 4.0f * QuarterTurn);

        if (Outline.FlareRight > 0)
        {
            const float Flare = static_cast<float>(Outline.FlareRight);
            AppendOutlineArc(OutPoints, Vector2(TabRight + Flare, StripBottom - Flare), Flare - Border + HalfThickness, 2.0f * QuarterTurn, QuarterTurn);
        }
        else
        {
            AppendOutlinePoint(OutPoints, Vector2(TabRight + Border - HalfThickness, InnerTop));
        }
    }

    if (Radius.TopRight > 0.0f)
    {
        AppendOutlineArc(OutPoints, Vector2(Right - Radius.TopRight, Top + Radius.TopRight), Radius.TopRight - HalfThickness, 3.0f * QuarterTurn, 4.0f * QuarterTurn);
    }
    else
    {
        AppendOutlinePoint(OutPoints, Vector2(InnerRight, InnerTop));
    }

    if (Radius.BottomRight > 0.0f)
    {
        AppendOutlineArc(OutPoints, Vector2(Right - Radius.BottomRight, Bottom - Radius.BottomRight), Radius.BottomRight - HalfThickness, 0.0f, QuarterTurn);
    }
    else
    {
        AppendOutlinePoint(OutPoints, Vector2(InnerRight, InnerBottom));
    }

    if (Radius.BottomLeft > 0.0f)
    {
        AppendOutlineArc(OutPoints, Vector2(Left + Radius.BottomLeft, Bottom - Radius.BottomLeft), Radius.BottomLeft - HalfThickness, QuarterTurn, 2.0f * QuarterTurn);
    }
    else
    {
        AppendOutlinePoint(OutPoints, Vector2(InnerLeft, InnerBottom));
    }

    if (OutPoints.Size() > 1 && (OutPoints.Last() - OutPoints[0]).GetLengthSquared() <= 1.0e-4f)
    {
        OutPoints.Pop();
    }
}

static String GetActiveTabId(const FDockNode& Node)
{
    if (Node.TabIds.IsEmpty())
    {
        return String();
    }

    return Node.TabIds[Math::Clamp(Node.ActiveTabIndex, 0, Node.TabIds.Size() - 1)];
}

bool FDockingArea::IsShowingStrip(const FLeafGeometry& Leaf) const
{
    return Leaf.Strip && !(bSuppressRootTabStrip && Leaf.Path.IsEmpty());
}

bool FDockingArea::GetWindowPosition(IntVector2& OutPosition) const
{
    if (!FApplication::IsInitialized())
    {
        return false;
    }

    const TSharedPtr<FWindow> Window = FApplication::Get().FindWindow(const_cast<FDockingArea*>(this)->AsSharedPtr());
    if (!Window)
    {
        return false;
    }

    OutPosition = Window->GetPosition();
    return true;
}

bool FDockingArea::GetDragClientPosition(IntVector2& OutClientPosition) const
{
    IntVector2 WindowPosition;
    if (!GetWindowPosition(WindowPosition))
    {
        return false;
    }

    OutClientPosition = FDockDragState::Get().GetCursorPosition() - WindowPosition;
    return true;
}

FRectangle FDockingArea::GetDecoratorClientBounds() const
{
    IntVector2 WindowPosition;
    if (!FDockWindowManager::IsInitialized() || !GetWindowPosition(WindowPosition))
    {
        return FRectangle();
    }

    FRectangle Bounds = FDockWindowManager::Get().GetDecoratorScreenBounds();
    if (!Bounds.IsEmpty())
    {
        Bounds.Position -= WindowPosition;
    }

    return Bounds;
}

FRectangle FDockingArea::GetPanelScreenBounds(const String& PanelId) const
{
    IntVector2 WindowPosition;
    if (!Root.FindTabsNode(PanelId) || !GetWindowPosition(WindowPosition))
    {
        return FRectangle();
    }

    FRectangle Bounds = ComputeDropBounds(PanelId, EDockDirection::Center);
    if (!Bounds.IsEmpty())
    {
        Bounds.Position += WindowPosition;
    }

    return Bounds;
}

void FDockingArea::BeginTearOut(const FRectangle& FromScreenBounds, const FRHITextureRef& Picture)
{
    IntVector2 WindowPosition;
    if (FromScreenBounds.IsEmpty() || !Picture || !GetWindowPosition(WindowPosition))
    {
        return;
    }

    RetireTearOutPicture();

    TearOutFrom          = FromScreenBounds;
    TearOutFrom.Position -= WindowPosition;
    TearOutPicture       = Picture;

    TearOutMove.Start(DROP_TEAR_OUT_SECONDS, 0.0f, 1.0f);
    InvalidatePaint();
}

FRectangle FDockingArea::GetDisplayedTearOutBounds() const
{
    const FRectangle Decorator = GetDecoratorClientBounds();
    return LerpRectangle(TearOutFrom, Decorator.IsEmpty() ? TearOutFrom : Decorator, TearOutMove.EvaluateEaseOut());
}

void FDockingArea::RetireTearOutPicture()
{
    if (TearOutPicture && FApplication::IsInitialized())
    {
        if (TSharedPtr<IApplicationRenderer> Renderer = FApplication::Get().GetRenderer())
        {
            Renderer->RetireTexture(TearOutPicture);
        }
    }

    TearOutPicture = nullptr;
}

FRectangle FDockingArea::GetDisplayedLandingBounds() const
{
    FRectangle Destination = LandingTo;
    if (!SettlingPanelId.IsEmpty())
    {
        const FRectangle DroppedBounds = ComputeDropBounds(SettlingPanelId, EDockDirection::Center);
        if (!DroppedBounds.IsEmpty())
        {
            Destination = DroppedBounds;
        }
    }

    if (bLandingTracksDecorator)
    {
        const FRectangle Decorator = GetDecoratorClientBounds();
        if (!Decorator.IsEmpty())
        {
            Destination = Decorator;
        }
    }

    const float Alpha = bLandingOvershoots ? LandingMove.EvaluateEaseOutBack(DROP_LANDING_OVERSHOOT) : LandingMove.EvaluateEaseOut();
    return LerpRectangle(LandingFrom, Destination, Alpha);
}

void FDockingArea::SettleDrop(const String& PanelId)
{
    if (!bHasLanding && LandingFade.EvaluateEaseOut() <= 0.0f)
    {
        return;
    }

    LandingFrom             = GetDisplayedLandingBounds();
    LandingTo               = LandingFrom;
    SettlingPanelId         = PanelId;
    bHasLanding             = false;
    bLandingTracksDecorator = false;
    bLandingOvershoots      = false;

    LandingMove.Start(DROP_SETTLE_SECONDS, 0.0f, 1.0f);
    LandingFade.Start(DROP_SETTLE_SECONDS, LandingFade.Evaluate(), 0.0f);
    InvalidatePaint();
}

void FDockingArea::UpdateDropPreview()
{
    const FDockDragState& DragState = FDockDragState::Get();

    const bool bIsTarget = bIsDropTarget && DragState.IsDragging() && DragState.GetTargetArea() == this;
    if (bIsTarget)
    {
        const FRectangle Landing = ComputeDropBounds(DragState.GetTargetPanelId(), DragState.GetTargetDirection());
        if (!bHasLanding)
        {
            const FRectangle Decorator = GetDecoratorClientBounds();
            if (LandingFade.Evaluate() > 0.0f)
            {
                LandingFrom = GetDisplayedLandingBounds();
            }
            else
            {
                LandingFrom = Decorator.IsEmpty() ? Landing : Decorator;
            }

            LandingTo = Landing;
            LandingMove.Start(DROP_LANDING_MOVE_SECONDS, 0.0f, 1.0f);
            LandingFade.Start(DROP_LANDING_FADE_SECONDS, LandingFade.Evaluate(), 1.0f);

            SettlingPanelId.Clear();
            bHasLanding             = true;
            bLandingTracksDecorator = false;
            bLandingOvershoots      = true;
        }
        else if (!(Landing == LandingTo))
        {
            LandingFrom = GetDisplayedLandingBounds();
            LandingTo   = Landing;
            LandingMove.Start(DROP_LANDING_MOVE_SECONDS, 0.0f, 1.0f);

            bLandingOvershoots = true;
        }
    }
    else if (bHasLanding)
    {
        LandingFrom = GetDisplayedLandingBounds();
        LandingTo   = LandingFrom;

        if (DragState.IsDragging() && !GetDecoratorClientBounds().IsEmpty())
        {
            LandingMove.Start(DROP_LANDING_MOVE_SECONDS, 0.0f, 1.0f);
            bLandingTracksDecorator = true;
        }
        else
        {
            LandingMove.Settle(1.0f);
            bLandingTracksDecorator = false;
        }

        LandingFade.Start(DROP_LANDING_MOVE_SECONDS, LandingFade.Evaluate(), 0.0f);

        bHasLanding        = false;
        bLandingOvershoots = false;
    }

    if (!TearOutMove.IsRunning())
    {
        RetireTearOutPicture();
    }

    TArray<FDropZone> Zones;

    IntVector2 ClientPosition;
    if (bIsDropTarget && DragState.IsDragging() && IsInCursorWindow() && GetDragClientPosition(ClientPosition)
        && GetContentRectangle().EncapsulatesPoint(ClientPosition) && !IsOverLiftedTabStrip(ClientPosition))
    {
        GatherDropZones(ClientPosition, Zones);
    }

    if (Zones.IsEmpty())
    {
        bHasChips = false;
    }
    else
    {
        if (!bHasChips || Zones[0].TargetPanelId != ChipsTargetPanelId)
        {
            ChipsTargetPanelId       = Zones[0].TargetPanelId;
            HoveredChipIndex         = -1;
            PreviousHoveredChipIndex = -1;

            ChipsFade.Start(DROP_CHIPS_FADE_SECONDS, 0.0f, 1.0f);
            ChipHoverFade.Settle(1.0f);
            bHasChips = true;
        }

        int32 HoveredIndex = -1;
        for (int32 Index = 0; Index < Zones.Size(); ++Index)
        {
            if (Zones[Index].Bounds.EncapsulatesPoint(ClientPosition))
            {
                HoveredIndex = Index;
                break;
            }
        }

        if (HoveredIndex != HoveredChipIndex)
        {
            PreviousHoveredChipIndex = HoveredChipIndex;
            HoveredChipIndex         = HoveredIndex;
            ChipHoverFade.Start(DROP_CHIP_HOVER_SECONDS, 0.0f, 1.0f);
        }
    }

    UpdateDropTabPreviews();
}

void FDockingArea::UpdateDropTabPreviews()
{
    const FDockDragState& DragState = FDockDragState::Get();

    const FDockNode* TargetNode = nullptr;
    if (DragState.IsDragging() && DragState.GetTargetArea() == this && DragState.GetTargetDirection() == EDockDirection::Center)
    {
        TargetNode = DragState.GetTargetPanelId().IsEmpty() ? &Root : Root.FindTabsNode(DragState.GetTargetPanelId());
    }

    const String Label = DragState.GetDraggedPanel().Label;

    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (!Leaf.Strip)
        {
            continue;
        }

        if (TargetNode && TargetNode->Kind == EDockNodeKind::Tabs && Root.FindByPath(Leaf.Path) == TargetNode)
        {
            Leaf.Strip->SetPreviewTab(Label, DragState.GetTargetTabIndex());
        }
        else
        {
            Leaf.Strip->ClearPreviewTab();
        }
    }
}

FPanelOutline FDockingArea::ComputeLeafOutline(const FLeafGeometry& Leaf) const
{
    // A strip lifted into the title bar still frames the panel, as long as it has been placed there
    const FTabStrip* Strip = (Leaf.Strip && Leaf.Strip->GetParentElement().IsValid()) ? Leaf.Strip.Get() : nullptr;

    return ComputePanelOutline(Leaf.Frame->GetContentRectangle(), Strip, TabStyle, FUIStyle::GetDefault().Panel);
}

TSharedPtr<FDockingArea> FDockingArea::Create(const FDesc& Desc)
{
    TSharedPtr<FDockingArea> NewArea = MakeSharedPtr<FDockingArea>();
    NewArea->Initialize(Desc);
    return NewArea;
}

FDockingArea::FDockingArea()
    : FCompoundElement()
    , Root()
    , PanelsById()
    , Leaves()
    , Font(nullptr)
    , TabFont(nullptr)
    , bAllowTearOut(true)
    , bIsDropTarget(true)
    , bNeedsRebuild(false)
    , bSuppressRootTabStrip(false)
    , LandingFrom()
    , LandingTo()
    , LandingMove()
    , LandingFade()
    , SettlingPanelId()
    , bHasLanding(false)
    , bLandingTracksDecorator(false)
    , bLandingOvershoots(false)
    , TearOutFrom()
    , TearOutPicture(nullptr)
    , TearOutMove()
    , ChipsTargetPanelId()
    , ChipsFade()
    , ChipHoverFade()
    , HoveredChipIndex(-1)
    , PreviousHoveredChipIndex(-1)
    , bHasChips(false)
    , OnPanelTornOutDelegate()
    , OnPanelClosedDelegate()
{
}

FDockingArea::~FDockingArea()
{
    RetireTearOutPicture();

    if (FDockDragState::IsInitialized())
    {
        FDockDragState::Get().UnregisterArea(this);
    }
}

void FDockingArea::Initialize(const FDesc& Desc)
{
    Font                   = Desc.Font;
    TabFont                = Desc.TabFont ? Desc.TabFont : Desc.Font;
    TabStyle               = Desc.TabStyle;
    TabCloseIcon           = Desc.TabCloseIcon;
    bAllowTearOut          = Desc.bAllowTearOut;
    bIsDropTarget          = Desc.bIsDropTarget;
    OnPanelTornOutDelegate = Desc.OnPanelTornOut;
    OnPanelClosedDelegate  = Desc.OnPanelClosed;

    DrawCacheEpoch::Advance();

    if (bIsDropTarget)
    {
        FDockDragState::Get().RegisterArea(this);
    }
}

IntVector2 FDockingArea::PrepareDesiredSize()
{
    FlushPendingRebuild();
    return FCompoundElement::PrepareDesiredSize();
}

const FVisualElement* FDockingArea::FindFocusedFrame() const
{
    if (!FApplication::IsInitialized())
    {
        return nullptr;
    }

    const TSharedPtr<FVisualElement> FocusLeaf = FApplication::Get().GetFocusElementLeaf();
    for (const FVisualElement* Element = FocusLeaf.Get(); Element; Element = Element->GetParentElement().Get())
    {
        for (const FLeafGeometry& Leaf : Leaves)
        {
            if (Leaf.Frame.Get() == Element)
            {
                return Leaf.Frame.Get();
            }
        }
    }

    return nullptr;
}

int32 FDockingArea::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();
    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.WindowBackground);

    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Leaf.Frame)
        {
            const FPanelOutline Outline = ComputeLeafOutline(Leaf);
            OutCommandList.AddBox(LayerId, Outline.BodyBounds, Style.Panel.Fill, Outline.BodyRadii);
        }
    }

    const int32 NextLayerId = FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);

    TArray<Vector2> OutlinePoints;

    const FVisualElement* FocusedFrame = FindFocusedFrame();
    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (!Leaf.Frame)
        {
            continue;
        }

        const bool          bIsFocused = (FocusedFrame == Leaf.Frame.Get());
        const FPanelOutline Outline    = ComputeLeafOutline(Leaf);

        OutCommandList.AddPanelChrome(NextLayerId, Outline.BodyBounds, Outline.BodyRadii, 0.0f, FFloatColor(0.0f, 0.0f, 0.0f, 0.0f), Style.Colors.WindowBackground);

        if (Style.Panel.BorderThickness > 0.0f)
        {
            BuildPanelOutline(Outline, TabStyle.CornerRadius, Style.Panel.BorderThickness, OutlinePoints);
            OutCommandList.AddPolyline(NextLayerId, OutlinePoints, bIsFocused ? Style.Panel.BorderFocused : Style.Panel.Border, Style.Panel.BorderThickness, true);
        }
    }

    return DrawDropZones(OutCommandList, NextLayerId + 1) + 1;
}

void FDockingArea::RegisterPanel(const String& PanelId, const String& Label, const TSharedPtr<FVisualElement>& Panel)
{
    if (PanelId.IsEmpty())
    {
        return;
    }

    FPanelEntry Entry;
    Entry.Label = Label.IsEmpty() ? PanelId : Label;
    Entry.Panel = Panel;

    PanelsById.Add(PanelId, Entry);
}

void FDockingArea::UnregisterPanel(const String& PanelId)
{
    UndockPanel(PanelId);
    PanelsById.Remove(PanelId);
}

void FDockingArea::RestoreLayout(const FDockNode& RootNode)
{
    Root = RootNode;

    TArray<String> SavedPanelIds;
    Root.GatherPanelIds(SavedPanelIds);

    for (const String& PanelId : SavedPanelIds)
    {
        if (!PanelsById.Contains(PanelId))
        {
            UndockPanel(PanelId);
        }
    }

    Root.CollapseDegenerateNodes();
    RequestRebuild();
}

bool FDockingArea::SaveLayoutToFile(const String& Filename) const
{
    FDockWindowLayout Layout;
    Layout.Root = Root;

    TArray<FDockWindowLayout> Windows;
    Windows.Add(Layout);

    return FDockLayoutFile::Save(Filename, Windows);
}

bool FDockingArea::RestoreLayoutFromFile(const String& Filename)
{
    TArray<FDockWindowLayout> Windows;
    if (!FDockLayoutFile::Load(Filename, Windows))
    {
        return false;
    }

    RestoreLayout(Windows[0].Root);
    return true;
}

void FDockingArea::DockPanel(const String& PanelId, const String& TargetPanelId, EDockDirection Direction, int32 TabIndex)
{
    if (PanelId.IsEmpty() || !PanelsById.Contains(PanelId))
    {
        return;
    }

    UndockPanel(PanelId);

    if (Root.IsEmpty())
    {
        Root = FDockNode::CreateTabs({ PanelId });
        RequestRebuild();
        return;
    }

    FDockNode* TargetNode = TargetPanelId.IsEmpty() ? &Root : Root.FindTabsNode(TargetPanelId);
    if (!TargetNode)
    {
        TargetNode = &Root;
    }

    DockAgainstNode(*TargetNode, PanelId, Direction, TabIndex);

    Root.CollapseDegenerateNodes();
    RequestRebuild();
}

void FDockingArea::UndockPanel(const String& PanelId)
{
    FDockNode* TabsNode = Root.FindTabsNode(PanelId);
    if (!TabsNode)
    {
        return;
    }

    for (int32 Index = 0; Index < TabsNode->TabIds.Size(); ++Index)
    {
        if (TabsNode->TabIds[Index] == PanelId)
        {
            TabsNode->TabIds.RemoveAt(Index);

            if (TabsNode->ActiveTabIndex >= Index)
            {
                TabsNode->ActiveTabIndex = Math::Max(0, TabsNode->ActiveTabIndex - 1);
            }

            break;
        }
    }

    Root.CollapseDegenerateNodes();
    RequestRebuild();
}

void FDockingArea::GatherDropZones(const IntVector2& ClientPosition, TArray<FDropZone>& OutZones) const
{
    OutZones.Clear();

    if (IsOverLiftedTabStrip(ClientPosition))
    {
        FDropZone StripZone;
        StripZone.Bounds        = GetLiftedTabStripBounds();
        StripZone.TargetPanelId = GetActiveTabId(Root);
        StripZone.bIsTabStrip   = true;
        StripZone.Strip         = GetRootTabStrip().Get();

        OutZones.Add(StripZone);
        return;
    }

    const int32 LeafIndex = FindLeafAt(ClientPosition);
    if (LeafIndex < 0)
    {
        if (Root.IsEmpty())
        {
            FDropZone WholeArea;
            WholeArea.Bounds = GetContentRectangle();

            OutZones.Add(WholeArea);
        }

        return;
    }

    const FLeafGeometry& Leaf = Leaves[LeafIndex];

    const FDockNode* Node = Root.FindByPath(Leaf.Path);
    if (!Node || Node->TabIds.IsEmpty() || !Leaf.Frame)
    {
        return;
    }

    const String TargetPanelId = GetActiveTabId(*Node);

    const bool bIsStripLifted = bSuppressRootTabStrip && Leaf.Path.IsEmpty();
    if (Leaf.Strip && !bIsStripLifted)
    {
        FDropZone StripZone;
        StripZone.Bounds        = Leaf.Strip->GetContentRectangle();
        StripZone.TargetPanelId = TargetPanelId;
        StripZone.bIsTabStrip   = true;
        StripZone.Strip         = Leaf.Strip.Get();

        OutZones.Add(StripZone);
    }

    const FRectangle LeafBounds = Leaf.Frame->GetContentRectangle();

    const int32 ChipSize = Math::Min(DROP_ZONE_CHIP_SIZE, (Math::Min(LeafBounds.Width, LeafBounds.Height) - (2 * DROP_ZONE_CHIP_GAP)) / 3);
    if (ChipSize < DROP_ZONE_CHIP_MINIMUM)
    {
        return;
    }

    const IntVector2 Center = LeafBounds.GetCenter();
    const int32      Reach  = (ChipSize / 2) + DROP_ZONE_CHIP_GAP;

    const auto AddChip = [&](const IntVector2& Position, EDockDirection Direction)
    {
        FDropZone Chip;
        Chip.Bounds        = FRectangle(Position, ChipSize, ChipSize);
        Chip.TargetPanelId = TargetPanelId;
        Chip.Direction     = Direction;

        OutZones.Add(Chip);
    };

    AddChip(IntVector2(Center.X - Reach - ChipSize, Center.Y - (ChipSize / 2)),   EDockDirection::Left);
    AddChip(IntVector2(Center.X + Reach,            Center.Y - (ChipSize / 2)),   EDockDirection::Right);
    AddChip(IntVector2(Center.X - (ChipSize / 2),   Center.Y - Reach - ChipSize), EDockDirection::Top);
    AddChip(IntVector2(Center.X - (ChipSize / 2),   Center.Y + Reach),            EDockDirection::Bottom);

    if (bIsStripLifted)
    {
        AddChip(IntVector2(Center.X - (ChipSize / 2), Center.Y - (ChipSize / 2)), EDockDirection::Center);
    }
}

bool FDockingArea::HitTestDropTarget(const IntVector2& ScreenPosition, String& OutTargetPanelId, EDockDirection& OutDirection, int32& OutTabIndex) const
{
    OutTargetPanelId.Clear();
    OutDirection = EDockDirection::Center;
    OutTabIndex  = -1;

    if (!FApplication::IsInitialized())
    {
        return false;
    }

    TSharedPtr<FWindow> Window = FApplication::Get().FindWindow(const_cast<FDockingArea*>(this)->AsSharedPtr());
    if (!Window)
    {
        return false;
    }

    const IntVector2 ClientPosition = ScreenPosition - Window->GetPosition();
    if (!GetContentRectangle().EncapsulatesPoint(ClientPosition) && !IsOverLiftedTabStrip(ClientPosition))
    {
        return false;
    }

    TArray<FDropZone> Zones;
    GatherDropZones(ClientPosition, Zones);

    for (const FDropZone& Zone : Zones)
    {
        if (Zone.Bounds.EncapsulatesPoint(ClientPosition))
        {
            OutTargetPanelId = Zone.TargetPanelId;
            OutDirection     = Zone.Direction;
            OutTabIndex      = (Zone.bIsTabStrip && Zone.Strip) ? Zone.Strip->FindDropIndex(ClientPosition.X) : -1;
            return true;
        }
    }

    return false;
}

void FDockingArea::SetActivePanel(const String& PanelId)
{
    FDockNode* TabsNode = Root.FindTabsNode(PanelId);
    if (!TabsNode)
    {
        return;
    }

    for (int32 Index = 0; Index < TabsNode->TabIds.Size(); ++Index)
    {
        if (TabsNode->TabIds[Index] == PanelId)
        {
            TabsNode->ActiveTabIndex = Index;
            break;
        }
    }

    RequestRebuild();
}

TArray<String> FDockingArea::GetDockedPanelIds() const
{
    TArray<String> PanelIds;
    Root.GatherPanelIds(PanelIds);
    return PanelIds;
}

TSharedPtr<FTabStrip> FDockingArea::GetRootTabStrip() const
{
    if (Root.Kind != EDockNodeKind::Tabs || Leaves.IsEmpty())
    {
        return nullptr;
    }

    return Leaves[0].Strip;
}

void FDockingArea::SetSuppressRootTabStrip(bool bInSuppress)
{
    if (bSuppressRootTabStrip == bInSuppress)
    {
        return;
    }

    bSuppressRootTabStrip = bInSuppress;
    RequestRebuild();
}

TSharedPtr<FTabStrip> FDockingArea::FindPanelTabStrip(const String& PanelId) const
{
    if (PanelId.IsEmpty())
    {
        return Leaves.IsEmpty() ? nullptr : Leaves[0].Strip;
    }

    const FDockNode* TargetNode = Root.FindTabsNode(PanelId);
    if (!TargetNode)
    {
        return nullptr;
    }

    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Root.FindByPath(Leaf.Path) == TargetNode)
        {
            return Leaf.Strip;
        }
    }

    return nullptr;
}

bool FDockingArea::IsPanelDocked(const String& PanelId) const
{
    return Root.FindTabsNode(PanelId) != nullptr;
}

String FDockingArea::GetMiddlePanelId() const
{
    const FRectangle& Bounds = GetContentRectangle();

    const FDockNode* MiddleNode = Root.FindMostCentralTabsNode(static_cast<float>(Bounds.Width), static_cast<float>(Bounds.Height));
    if (!MiddleNode || MiddleNode->TabIds.IsEmpty())
    {
        return String();
    }

    return MiddleNode->TabIds[Math::Clamp(MiddleNode->ActiveTabIndex, 0, MiddleNode->TabIds.Size() - 1)];
}

bool FDockingArea::IsPanelVisible(const String& PanelId) const
{
    const FDockNode* Node = Root.FindTabsNode(PanelId);
    if (!Node || !Node->TabIds.IsValidIndex(Node->ActiveTabIndex))
    {
        return false;
    }

    return Node->TabIds[Node->ActiveTabIndex] == PanelId;
}

TArray<String> FDockingArea::GetRegisteredPanelIds() const
{
    TArray<String> PanelIds;
    for (const auto& Pair : PanelsById)
    {
        PanelIds.Add(Pair.First);
    }

    return PanelIds;
}

bool FDockingArea::GetPanelRegistration(const String& PanelId, String& OutLabel, TSharedPtr<FVisualElement>& OutPanel) const
{
    const FPanelEntry* Entry = PanelsById.Find(PanelId);
    if (!Entry)
    {
        return false;
    }

    OutLabel = Entry->Label;
    OutPanel = Entry->Panel;
    return true;
}

void FDockingArea::RequestRebuild()
{
    bNeedsRebuild = true;

    InvalidateDesiredSize();
}

void FDockingArea::FlushPendingRebuild()
{
    if (!bNeedsRebuild)
    {
        return;
    }

    bNeedsRebuild = false;
    Leaves.Clear();

    if (Root.IsEmpty())
    {
        SetContent(nullptr);
        return;
    }

    const int32 Gap = FUIStyle::GetDefault().Panel.Gap;

    FBorder::FDesc OutsetDesc;
    OutsetDesc.Padding = bSuppressRootTabStrip ? FMargin(Gap, 0, Gap, Gap) : FMargin(Gap);
    OutsetDesc.Content = BuildNode(Root, TArray<int32>());

    SetContent(FBorder::Create(OutsetDesc));
}

TSharedPtr<FVisualElement> FDockingArea::BuildNode(FDockNode& Node, const TArray<int32>& Path)
{
    if (Node.Kind == EDockNodeKind::Tabs)
    {
        FTabStrip::FDesc StripDesc;
        StripDesc.Font           = TabFont ? TabFont : Font;
        StripDesc.Style          = TabStyle;
        StripDesc.CloseIcon      = TabCloseIcon;
        StripDesc.bAllowTearOut  = bAllowTearOut;
        StripDesc.OnTabActivated = FOnTabActivated::CreateRaw(this, &FDockingArea::OnTabActivated);
        StripDesc.OnTabClosed    = FOnTabClosed::CreateRaw(this, &FDockingArea::OnTabClosed);

        const TArray<int32> LeafPath = Path;
        StripDesc.OnTabReordered     = FOnTabReordered::CreateLambda([this, LeafPath](const String& PanelId, int32 NewIndex)
        {
            OnTabReordered(LeafPath, PanelId, NewIndex);
        });

        if (bAllowTearOut)
        {
            StripDesc.OnTabDragDetached = FOnTabDragDetached::CreateRaw(this, &FDockingArea::OnTabDetached);
            StripDesc.OnTabDragMoved    = FOnTabDragMoved::CreateRaw(this, &FDockingArea::OnTabDragMoved);
            StripDesc.OnTabDragFinished = FOnTabDragFinished::CreateRaw(this, &FDockingArea::OnTabDragFinished);
        }

        TSharedPtr<FTabStrip> Strip = FTabStrip::Create(StripDesc);

        for (const String& TabId : Node.TabIds)
        {
            const FPanelEntry* Entry = PanelsById.Find(TabId);
            Strip->AddTab(TabId, Entry ? Entry->Label : TabId, true);
        }

        const int32  ActiveIndex   = Math::Clamp(Node.ActiveTabIndex, 0, Math::Max(0, Node.TabIds.Size() - 1));
        const String ActivePanelId = Node.TabIds.IsEmpty() ? String() : Node.TabIds[ActiveIndex];

        Strip->SetActiveTab(ActivePanelId);

        const FPanelEntry* ActiveEntry = PanelsById.Find(ActivePanelId);
        TSharedPtr<FVisualElement> Body = ActiveEntry ? ActiveEntry->Panel : nullptr;

        TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
        if (!(bSuppressRootTabStrip && Path.IsEmpty()))
        {
            Column->AddSlot(Strip);
        }
        Column->AddSlot(Body).SetFillCoefficient(1.0f);

        const FUIStyle& Style = FUIStyle::GetDefault();

        FBorder::FDesc FrameDesc;
        FrameDesc.BackgroundColor = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
        FrameDesc.CornerRadius    = FCornerRadii(Style.Panel.CornerRadius);
        FrameDesc.Padding         = FMargin(static_cast<int32>(Style.Panel.BorderThickness));
        FrameDesc.Content         = Column;

        TSharedPtr<FBorder> Frame = FBorder::Create(FrameDesc);

        FLeafGeometry Leaf;
        Leaf.Strip  = Strip;
        Leaf.Column = Column;
        Leaf.Frame  = Frame;
        Leaf.Path   = Path;

        Leaves.Add(Leaf);
        return Frame;
    }

    FSplitter::FDesc SplitterDesc;
    SplitterDesc.Orientation     = Node.Orientation;
    SplitterDesc.HandleThickness = FUIStyle::GetDefault().Panel.Gap;

    const TArray<int32> SplitterPath = Path;
    SplitterDesc.OnFractionsChanged  = FOnSplitterFractionsChanged::CreateLambda([this, SplitterPath](const TArray<float>& NewFractions)
    {
        OnSplitterFractionsChanged(SplitterPath, NewFractions);
    });

    TSharedPtr<FSplitter> Splitter = FSplitter::Create(SplitterDesc);
    for (int32 Index = 0; Index < Node.Children.Size(); ++Index)
    {
        TArray<int32> ChildPath = Path;
        ChildPath.Add(Index);

        Splitter->AddChild(BuildNode(Node.Children[Index], ChildPath), Node.Children[Index].ComputeMinimumSize());
    }

    Splitter->SetFractions(Node.ChildFractions);
    return Splitter;
}

int32 FDockingArea::FindLeafAt(const IntVector2& ClientPosition) const
{
    for (int32 Index = 0; Index < Leaves.Size(); ++Index)
    {
        if (Leaves[Index].Frame && Leaves[Index].Frame->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            return Index;
        }
    }

    return -1;
}

FRectangle FDockingArea::GetLiftedTabStripBounds() const
{
    if (!bSuppressRootTabStrip || Root.Kind != EDockNodeKind::Tabs || Root.TabIds.IsEmpty())
    {
        return FRectangle();
    }

    const TSharedPtr<FTabStrip> Strip = GetRootTabStrip();
    return Strip ? Strip->GetContentRectangle() : FRectangle();
}

bool FDockingArea::IsOverLiftedTabStrip(const IntVector2& ClientPosition) const
{
    const FRectangle StripBounds = GetLiftedTabStripBounds();
    return !StripBounds.IsEmpty() && StripBounds.EncapsulatesPoint(ClientPosition);
}

FRectangle FDockingArea::ComputeDropBounds(const String& TargetPanelId, EDockDirection Direction) const
{
    const FDockNode* TargetNode = TargetPanelId.IsEmpty() ? &Root : Root.FindTabsNode(TargetPanelId);

    FRectangle LeafBounds = GetContentRectangle();
    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Root.FindByPath(Leaf.Path) == TargetNode && Leaf.Frame)
        {
            LeafBounds = Leaf.Frame->GetContentRectangle();

            if (Direction == EDockDirection::Center && IsShowingStrip(Leaf))
            {
                const int32 BodyTop = Math::Clamp(Leaf.Strip->GetContentRectangle().GetBottom(), LeafBounds.Position.Y, LeafBounds.GetBottom());
                LeafBounds = FRectangle(IntVector2(LeafBounds.Position.X, BodyTop), LeafBounds.Width, LeafBounds.GetBottom() - BodyTop);
            }

            break;
        }
    }

    LeafBounds = LeafBounds.Deflate(FMargin(DROP_INDICATOR_INSET, DROP_INDICATOR_INSET));
    if (LeafBounds.IsEmpty())
    {
        return FRectangle();
    }

    FRectangle DropBounds = LeafBounds;
    switch (Direction)
    {
        case EDockDirection::Left:
            DropBounds.Width = LeafBounds.Width / 2;
            break;

        case EDockDirection::Right:
            DropBounds.Width = LeafBounds.Width / 2;
            DropBounds.Position.X += LeafBounds.Width - DropBounds.Width;
            break;

        case EDockDirection::Top:
            DropBounds.Height = LeafBounds.Height / 2;
            break;

        case EDockDirection::Bottom:
            DropBounds.Height = LeafBounds.Height / 2;
            DropBounds.Position.Y += LeafBounds.Height - DropBounds.Height;
            break;

        case EDockDirection::Center:
            break;
    }

    return DropBounds;
}

bool FDockingArea::IsInCursorWindow() const
{
    const FWindow* CursorWindow = FDockDragState::Get().GetCursorWindow();
    if (!CursorWindow || !FApplication::IsInitialized())
    {
        return false;
    }

    return FApplication::Get().FindWindow(const_cast<FDockingArea*>(this)->AsSharedPtr()).Get() == CursorWindow;
}

bool FDockingArea::DoesDropJoinTabStrip(const String& TargetPanelId, EDockDirection Direction) const
{
    if (Direction != EDockDirection::Center)
    {
        return false;
    }

    const FDockNode* TargetNode = TargetPanelId.IsEmpty() ? &Root : Root.FindTabsNode(TargetPanelId);
    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Root.FindByPath(Leaf.Path) == TargetNode)
        {
            return Leaf.Strip != nullptr;
        }
    }

    return false;
}

bool FDockingArea::GetDropPreviewBounds(const String& TargetPanelId, EDockDirection Direction, FRectangle& OutBounds) const
{
    OutBounds = FRectangle();

    if (!FApplication::IsInitialized())
    {
        return false;
    }

    TSharedPtr<FWindow> Window = FApplication::Get().FindWindow(const_cast<FDockingArea*>(this)->AsSharedPtr());
    if (!Window)
    {
        return false;
    }

    const FRectangle DropBounds = ComputeDropBounds(TargetPanelId, Direction);
    if (DropBounds.IsEmpty())
    {
        return false;
    }

    OutBounds = DropBounds;
    OutBounds.Position += Window->GetPosition();

    return true;
}

int32 FDockingArea::DrawDropZones(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FDockDragState& DragState = FDockDragState::Get();
    if (!bIsDropTarget)
    {
        return LayerId;
    }

    if (LandingMove.IsRunning() || LandingFade.IsRunning() || ChipsFade.IsRunning() || ChipHoverFade.IsRunning() || TearOutMove.IsRunning())
    {
        RequestContinuousPaint();
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    if (TearOutPicture && TearOutMove.IsRunning())
    {
        const FRectangle TearOutBounds = GetDisplayedTearOutBounds();
        if (!TearOutBounds.IsEmpty())
        {
            const float Opacity = Math::Lerp(1.0f, FDockWindowManager::DecoratorOpacity, TearOutMove.EvaluateEaseOut());
            OutCommandList.AddImage(LayerId, TearOutBounds, FUIBrush(TearOutPicture.Get()), FFloatColor(1.0f, 1.0f, 1.0f, Opacity), FCornerRadii(Style.Panel.CornerRadius));
        }
    }

    const auto SelectionTint = [&Style](float Opacity)
    {
        FFloatColor Tint = Style.TreeRow.SelectedFill;
        Tint.A *= Opacity;
        return Tint;
    };

    const float LandingOpacity = LandingFade.EvaluateEaseOut();
    if (LandingOpacity > 0.0f)
    {
        const FRectangle LandingBounds = GetDisplayedLandingBounds();
        if (!LandingBounds.IsEmpty())
        {
            const FCornerRadii LandingRadii(Style.Panel.CornerRadius);

            const bool         bShowsGhost = bHasLanding && DragState.IsDragging() && DragState.GetTargetArea() == this && FDockWindowManager::IsInitialized();
            FRHITexture* const Ghost       = bShowsGhost ? FDockWindowManager::Get().GetDropPreviewTexture() : nullptr;
            if (Ghost)
            {
                OutCommandList.AddImage(LayerId, LandingBounds, FUIBrush(Ghost), FFloatColor(1.0f, 1.0f, 1.0f, DROP_ZONE_GHOST_OPACITY * LandingOpacity), LandingRadii);
            }
            else
            {
                OutCommandList.AddBox(LayerId, LandingBounds, SelectionTint(DROP_ZONE_LANDING_OPACITY * LandingOpacity), LandingRadii);
            }

            OutCommandList.AddBoxOutline(LayerId, LandingBounds, SelectionTint(LandingOpacity), Style.Metrics.BorderThickness, LandingRadii);
        }
    }

    const int32 ChipLayerId = LayerId + 1;

    IntVector2 ClientPosition;
    if (!DragState.IsDragging() || !IsInCursorWindow() || !GetDragClientPosition(ClientPosition))
    {
        return ChipLayerId;
    }

    if (!GetContentRectangle().EncapsulatesPoint(ClientPosition) || IsOverLiftedTabStrip(ClientPosition))
    {
        return ChipLayerId;
    }

    TArray<FDropZone> Zones;
    GatherDropZones(ClientPosition, Zones);

    const FCornerRadii ChipRadii(DROP_ZONE_CHIP_CORNER_RADIUS);

    const float ChipsOpacity = ChipsFade.EvaluateEaseOut();
    const float HoverBlend   = ChipHoverFade.EvaluateEaseOut();

    for (int32 Index = 0; Index < Zones.Size(); ++Index)
    {
        if (Zones[Index].bIsTabStrip)
        {
            continue;
        }

        float Hover = 0.0f;
        if (Index == HoveredChipIndex)
        {
            Hover = HoverBlend;
        }
        else if (Index == PreviousHoveredChipIndex)
        {
            Hover = 1.0f - HoverBlend;
        }

        const float FillOpacity    = Math::Lerp(DROP_ZONE_CHIP_OPACITY, DROP_ZONE_HOVER_OPACITY, Hover) * ChipsOpacity;
        const float OutlineOpacity = Math::Lerp(DROP_ZONE_CHIP_OUTLINE_OPACITY, DROP_ZONE_HOVER_OUTLINE_OPACITY, Hover) * ChipsOpacity;

        OutCommandList.AddBox(ChipLayerId, Zones[Index].Bounds, SelectionTint(FillOpacity), ChipRadii);
        OutCommandList.AddBoxOutline(ChipLayerId, Zones[Index].Bounds, SelectionTint(OutlineOpacity), Style.Metrics.BorderThickness, ChipRadii);
    }

    return ChipLayerId;
}

void FDockingArea::DockAgainstNode(FDockNode& TargetNode, const String& PanelId, EDockDirection Direction, int32 TabIndex)
{
    if (Direction == EDockDirection::Center && TargetNode.Kind == EDockNodeKind::Tabs)
    {
        const int32 InsertIndex = (TabIndex >= 0 && TabIndex <= TargetNode.TabIds.Size()) ? TabIndex : TargetNode.TabIds.Size();

        TargetNode.TabIds.Insert(InsertIndex, PanelId);
        TargetNode.ActiveTabIndex = InsertIndex;
        return;
    }

    const bool bIsHorizontal = Direction == EDockDirection::Left || Direction == EDockDirection::Right;
    const bool bIsLeading    = Direction == EDockDirection::Left || Direction == EDockDirection::Top;

    const FDockNode ExistingNode = TargetNode;
    const FDockNode NewNode      = FDockNode::CreateTabs({ PanelId });

    const EDockSplitOrientation Orientation = Direction == EDockDirection::Center
        ? TargetNode.Orientation
        : (bIsHorizontal ? EDockSplitOrientation::Horizontal : EDockSplitOrientation::Vertical);

    TargetNode = bIsLeading
        ? FDockNode::CreateSplit(Orientation, NewNode, ExistingNode)
        : FDockNode::CreateSplit(Orientation, ExistingNode, NewNode);
}

void FDockingArea::OnTabActivated(const String& PanelId)
{
    FDockNode* TabsNode = Root.FindTabsNode(PanelId);
    if (!TabsNode)
    {
        return;
    }

    for (int32 Index = 0; Index < TabsNode->TabIds.Size(); ++Index)
    {
        if (TabsNode->TabIds[Index] != PanelId || TabsNode->ActiveTabIndex == Index)
        {
            continue;
        }

        TabsNode->ActiveTabIndex = Index;
        RequestRebuild();
        return;
    }
}

void FDockingArea::OnTabClosed(const String& PanelId)
{
    UndockPanel(PanelId);
    OnPanelClosedDelegate.ExecuteIfBound(PanelId);
}

void FDockingArea::OnTabReordered(const TArray<int32>& Path, const String& PanelId, int32 NewIndex)
{
    FDockNode* TabsNode = Root.FindByPath(Path);
    if (!TabsNode || TabsNode->Kind != EDockNodeKind::Tabs)
    {
        return;
    }

    const int32 FromIndex = TabsNode->TabIds.Find(PanelId);
    if (!TabsNode->TabIds.IsValidIndex(FromIndex))
    {
        return;
    }

    const int32 ToIndex = Math::Clamp(NewIndex, 0, TabsNode->TabIds.Size() - 1);
    if (FromIndex == ToIndex)
    {
        return;
    }

    const String ActivePanelId = TabsNode->TabIds.IsValidIndex(TabsNode->ActiveTabIndex)
        ? TabsNode->TabIds[TabsNode->ActiveTabIndex]
        : String();

    TabsNode->TabIds.RemoveAt(FromIndex);
    TabsNode->TabIds.Insert(ToIndex, PanelId);

    const int32 ActiveIndex = TabsNode->TabIds.Find(ActivePanelId);
    if (TabsNode->TabIds.IsValidIndex(ActiveIndex))
    {
        TabsNode->ActiveTabIndex = ActiveIndex;
    }
}

void FDockingArea::OnTabDetached(const String& PanelId, const IntVector2& /* ClientPosition */, const IntVector2& ScreenPosition)
{
    FDockDragState::Get().BeginDrag(PanelId, this, ScreenPosition);
    OnPanelTornOutDelegate.ExecuteIfBound(PanelId, ScreenPosition);
}

void FDockingArea::OnTabDragMoved(const String& /* PanelId */, const IntVector2& /* ClientPosition */, const IntVector2& ScreenPosition)
{
    FDockDragState::Get().UpdateDrag(ScreenPosition);
}

void FDockingArea::OnTabDragFinished(const String& /* PanelId */, const IntVector2& /* ClientPosition */, const IntVector2& ScreenPosition)
{
    FDockDragState::Get().UpdateDrag(ScreenPosition);
    FDockDragState::Get().EndDrag();
}

void FDockingArea::OnSplitterFractionsChanged(const TArray<int32>& Path, const TArray<float>& Fractions)
{
    if (FDockNode* Node = Root.FindByPath(Path))
    {
        if (Node->Kind == EDockNodeKind::Split && Node->Children.Size() == Fractions.Size())
        {
            Node->ChildFractions = Fractions;
        }
    }
}
