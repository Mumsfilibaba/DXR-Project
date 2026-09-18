#include "Application/Application.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockLayoutFile.h"
#include "Application/Docking/DockWindowManager.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Docking/Splitter.h"
#include "Application/Docking/TabStrip.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

constexpr int32 DROP_INDICATOR_INSET   = 2;
constexpr int32 DROP_ZONE_CHIP_SIZE    = 96;
constexpr int32 DROP_ZONE_CHIP_GAP     = 8;
constexpr int32 DROP_ZONE_CHIP_MINIMUM = 12;

constexpr float DROP_ZONE_CHIP_GRAY       = 0.10f;
constexpr float DROP_ZONE_CHIP_OPACITY    = 0.55f;
constexpr float DROP_ZONE_HOVER_GRAY      = 0.22f;
constexpr float DROP_ZONE_HOVER_OPACITY   = 0.85f;
constexpr float DROP_ZONE_GHOST_OPACITY   = 0.75f;
constexpr float DROP_ZONE_LANDING_OPACITY = 0.20f;

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
    , bAllowTearOut(true)
    , bIsDropTarget(true)
    , bNeedsRebuild(false)
    , OnPanelTornOutDelegate()
    , OnPanelClosedDelegate()
{
}

FDockingArea::~FDockingArea()
{
    if (FDockDragState::IsInitialized())
    {
        FDockDragState::Get().UnregisterArea(this);
    }
}

void FDockingArea::Initialize(const FDesc& Desc)
{
    Font                   = Desc.Font;
    TabStyle               = Desc.TabStyle;
    TabCloseIcon           = Desc.TabCloseIcon;
    bAllowTearOut          = Desc.bAllowTearOut;
    bIsDropTarget          = Desc.bIsDropTarget;
    OnPanelTornOutDelegate = Desc.OnPanelTornOut;
    OnPanelClosedDelegate  = Desc.OnPanelClosed;

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

    const int32 NextLayerId = FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);

    const FVisualElement* FocusedFrame = FindFocusedFrame();
    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Leaf.Frame)
        {
            const bool bIsFocused = (FocusedFrame == Leaf.Frame.Get());

            OutCommandList.AddPanelChrome(
                NextLayerId,
                Leaf.Frame->GetContentRectangle(),
                FCornerRadii(Style.Panel.CornerRadius),
                Style.Panel.BorderThickness,
                bIsFocused ? Style.Panel.BorderFocused : Style.Panel.Border,
                Style.Colors.WindowBackground);
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

void FDockingArea::DockPanel(const String& PanelId, const String& TargetPanelId, EDockDirection Direction)
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

    DockAgainstNode(*TargetNode, PanelId, Direction);

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

    const String& TargetPanelId = Node->TabIds[Math::Clamp(Node->ActiveTabIndex, 0, Node->TabIds.Size() - 1)];
    if (Leaf.Strip)
    {
        FDropZone StripZone;
        StripZone.Bounds        = Leaf.Strip->GetContentRectangle();
        StripZone.TargetPanelId = TargetPanelId;

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
}

bool FDockingArea::HitTestDropTarget(const IntVector2& ScreenPosition, String& OutTargetPanelId, EDockDirection& OutDirection) const
{
    OutTargetPanelId.Clear();
    OutDirection = EDockDirection::Center;

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
    if (!GetContentRectangle().EncapsulatesPoint(ClientPosition))
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

    FBorder::FDesc OutsetDesc;
    OutsetDesc.Padding = FMargin(FUIStyle::GetDefault().Panel.Gap);
    OutsetDesc.Content = BuildNode(Root, TArray<int32>());

    SetContent(FBorder::Create(OutsetDesc));
}

TSharedPtr<FVisualElement> FDockingArea::BuildNode(FDockNode& Node, const TArray<int32>& Path)
{
    if (Node.Kind == EDockNodeKind::Tabs)
    {
        FTabStrip::FDesc StripDesc;
        StripDesc.Font           = Font;
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
        Column->AddSlot(Strip);
        Column->AddSlot(Body).SetFillCoefficient(1.0f);

        const FUIStyle& Style = FUIStyle::GetDefault();

        FBorder::FDesc FrameDesc;
        FrameDesc.BackgroundColor = Style.Panel.Fill;
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

FRectangle FDockingArea::ComputeDropBounds(const String& TargetPanelId, EDockDirection Direction) const
{
    const FDockNode* TargetNode = TargetPanelId.IsEmpty() ? &Root : Root.FindTabsNode(TargetPanelId);

    FRectangle LeafBounds = GetContentRectangle();
    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Root.FindByPath(Leaf.Path) == TargetNode && Leaf.Frame)
        {
            LeafBounds = Leaf.Frame->GetContentRectangle();
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
    if (!bIsDropTarget || !DragState.IsDragging() || !FApplication::IsInitialized())
    {
        return LayerId;
    }

    TSharedPtr<FWindow> Window = FApplication::Get().FindWindow(const_cast<FDockingArea*>(this)->AsSharedPtr());
    if (!Window)
    {
        return LayerId;
    }

    const IntVector2 ClientPosition = DragState.GetCursorPosition() - Window->GetPosition();
    if (!GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        return LayerId;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();
    if (DragState.GetTargetArea() == this)
    {
        const FRectangle LandingBounds = ComputeDropBounds(DragState.GetTargetPanelId(), DragState.GetTargetDirection());
        if (!LandingBounds.IsEmpty())
        {
            FRHITexture* const Ghost = FDockWindowManager::IsInitialized() ? FDockWindowManager::Get().GetDropPreviewTexture() : nullptr;
            if (Ghost)
            {
                OutCommandList.AddImage(LayerId, LandingBounds, FUIBrush(Ghost), FFloatColor(1.0f, 1.0f, 1.0f, DROP_ZONE_GHOST_OPACITY));
            }
            else
            {
                OutCommandList.AddBox(LayerId, LandingBounds, FFloatColor(DROP_ZONE_HOVER_GRAY, DROP_ZONE_HOVER_GRAY, DROP_ZONE_HOVER_GRAY, DROP_ZONE_LANDING_OPACITY));
            }

            OutCommandList.AddBoxOutline(LayerId, LandingBounds, Style.Colors.Accent, Style.Metrics.BorderThickness);
        }
    }

    TArray<FDropZone> Zones;
    GatherDropZones(ClientPosition, Zones);

    const int32 ChipLayerId = LayerId + 1;
    for (const FDropZone& Zone : Zones)
    {
        const bool  bIsHovered = Zone.Bounds.EncapsulatesPoint(ClientPosition);
        const float Gray       = bIsHovered ? DROP_ZONE_HOVER_GRAY : DROP_ZONE_CHIP_GRAY;
        const float Opacity    = bIsHovered ? DROP_ZONE_HOVER_OPACITY : DROP_ZONE_CHIP_OPACITY;

        OutCommandList.AddBox(ChipLayerId, Zone.Bounds, FFloatColor(Gray, Gray, Gray, Opacity));
        OutCommandList.AddBoxOutline(ChipLayerId, Zone.Bounds, Style.Colors.Border, Style.Metrics.BorderThickness);
    }

    return ChipLayerId;
}

void FDockingArea::DockAgainstNode(FDockNode& TargetNode, const String& PanelId, EDockDirection Direction)
{
    if (Direction == EDockDirection::Center && TargetNode.Kind == EDockNodeKind::Tabs)
    {
        TargetNode.TabIds.Add(PanelId);
        TargetNode.ActiveTabIndex = TargetNode.TabIds.Size() - 1;
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
