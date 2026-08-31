#include "Application/Application.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Docking/Splitter.h"
#include "Application/Docking/TabStrip.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Box.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Misc/IniFile.h"
#include "Core/Templates/CString.h"

// The section every saved layout starts with, and the prefix each node section is named by
constexpr const CHAR* LAYOUT_SECTION      = "Layout";
constexpr const CHAR* LAYOUT_NODE_PREFIX  = "Node";
constexpr int32       LAYOUT_FILE_VERSION = 1;

// How far the drop indicator is inset from the leaf it covers, so the leaf beneath stays readable
constexpr int32 DROP_INDICATOR_INSET = 2;

static void SplitList(const String& Value, TArray<String>& OutTokens)
{
    String Token;
    for (int32 Index = 0; Index < Value.Length(); ++Index)
    {
        const CHAR Character = Value[Index];
        if (Character == ',')
        {
            if (!Token.IsEmpty())
            {
                OutTokens.Add(Token);
            }

            Token.Clear();
            continue;
        }

        Token.Append(Character);
    }

    if (!Token.IsEmpty())
    {
        OutTokens.Add(Token);
    }
}

static String JoinList(const TArray<String>& Tokens)
{
    String Result;
    for (int32 Index = 0; Index < Tokens.Size(); ++Index)
    {
        if (Index > 0)
        {
            Result.Append(',');
        }

        Result.Append(Tokens[Index]);
    }

    return Result;
}

static int32 WriteNode(FIniFile& File, const FDockNode& Node, int32& InOutNextIndex)
{
    const int32 NodeIndex = InOutNextIndex++;

    TArray<String> ChildIndices;
    if (Node.Kind == EDockNodeKind::Split)
    {
        for (const FDockNode& Child : Node.Children)
        {
            ChildIndices.Add(String::Printf("%d", WriteNode(File, Child, InOutNextIndex)));
        }
    }

    const String SectionName = String::Printf("%s%d", LAYOUT_NODE_PREFIX, NodeIndex);
    if (Node.Kind == EDockNodeKind::Split)
    {
        TArray<String> Fractions;
        for (float Fraction : Node.ChildFractions)
        {
            Fractions.Add(String::Printf("%.6f", Fraction));
        }

        File.SetOrAddString(SectionName.Data(), "Kind", String("Split"));
        File.SetOrAddString(SectionName.Data(), "Orientation", String(Node.Orientation == EDockSplitOrientation::Horizontal ? "Horizontal" : "Vertical"));
        File.SetOrAddString(SectionName.Data(), "Children", JoinList(ChildIndices));
        File.SetOrAddString(SectionName.Data(), "Fractions", JoinList(Fractions));
    }
    else
    {
        File.SetOrAddString(SectionName.Data(), "Kind", String("Tabs"));
        File.SetOrAddString(SectionName.Data(), "Tabs", JoinList(Node.TabIds));
        File.SetOrAddInt(SectionName.Data(), "ActiveTab", Node.ActiveTabIndex);
    }

    return NodeIndex;
}

static bool ReadNode(FIniFile& File, int32 NodeIndex, int32 NumNodes, int32 Depth, FDockNode& OutNode)
{
    if (NodeIndex < 0 || NodeIndex >= NumNodes || Depth > NumNodes)
    {
        return false;
    }

    const String SectionName = String::Printf("%s%d", LAYOUT_NODE_PREFIX, NodeIndex);

    String Kind;
    if (!File.GetString(SectionName.Data(), "Kind", Kind))
    {
        return false;
    }

    if (Kind == "Tabs")
    {
        OutNode.Kind = EDockNodeKind::Tabs;

        String Tabs;
        File.GetString(SectionName.Data(), "Tabs", Tabs);
        SplitList(Tabs, OutNode.TabIds);

        int32 ActiveTab = 0;
        File.GetInt(SectionName.Data(), "ActiveTab", ActiveTab);

        OutNode.ActiveTabIndex = ActiveTab;
        return true;
    }

    OutNode.Kind = EDockNodeKind::Split;

    String Orientation;
    File.GetString(SectionName.Data(), "Orientation", Orientation);

    OutNode.Orientation = Orientation == "Vertical" ? EDockSplitOrientation::Vertical : EDockSplitOrientation::Horizontal;

    String Children;
    File.GetString(SectionName.Data(), "Children", Children);

    TArray<String> ChildIndices;
    SplitList(Children, ChildIndices);

    for (const String& ChildIndex : ChildIndices)
    {
        FDockNode ChildNode;
        if (!ReadNode(File, CString::Atoi(ChildIndex.Data()), NumNodes, Depth + 1, ChildNode))
        {
            return false;
        }

        OutNode.Children.Add(ChildNode);
    }

    String Fractions;
    File.GetString(SectionName.Data(), "Fractions", Fractions);

    TArray<String> FractionTokens;
    SplitList(Fractions, FractionTokens);

    if (FractionTokens.Size() == OutNode.Children.Size())
    {
        for (const String& Token : FractionTokens)
        {
            OutNode.ChildFractions.Add(CString::Atof(Token.Data()));
        }
    }
    else
    {
        OutNode.NormalizeFractions();
    }

    return true;
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
    , bAllowTearOut(true)
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
    bAllowTearOut          = Desc.bAllowTearOut;
    OnPanelTornOutDelegate = Desc.OnPanelTornOut;
    OnPanelClosedDelegate  = Desc.OnPanelClosed;

    FDockDragState::Get().RegisterArea(this);
}

IntVector2 FDockingArea::PrepareDesiredSize()
{
    FlushPendingRebuild();
    return FCompoundElement::PrepareDesiredSize();
}

int32 FDockingArea::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();
    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.WindowBackground);

    const int32 NextLayerId = FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);
    DrawDropIndicator(OutCommandList, NextLayerId);
    return NextLayerId + 2;
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
    FIniFile File;
    File.Filename = Filename;

    int32 NextIndex = 0;
    WriteNode(File, Root, NextIndex);

    File.SetOrAddInt(LAYOUT_SECTION, "Version", LAYOUT_FILE_VERSION);
    File.SetOrAddInt(LAYOUT_SECTION, "NumNodes", NextIndex);
    File.SetOrAddInt(LAYOUT_SECTION, "Root", 0);

    return File.WriteToFile();
}

bool FDockingArea::RestoreLayoutFromFile(const String& Filename)
{
    FIniFile File;
    if (!File.LoadFromFile(Filename))
    {
        return false;
    }

    int32 Version = 0;
    if (!File.GetInt(LAYOUT_SECTION, "Version", Version) || Version != LAYOUT_FILE_VERSION)
    {
        return false;
    }

    int32 NumNodes  = 0;
    int32 RootIndex = 0;

    File.GetInt(LAYOUT_SECTION, "NumNodes", NumNodes);
    File.GetInt(LAYOUT_SECTION, "Root", RootIndex);

    FDockNode RestoredRoot;
    if (!ReadNode(File, RootIndex, NumNodes, 0, RestoredRoot))
    {
        return false;
    }

    RestoreLayout(RestoredRoot);
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

    const int32 LeafIndex = FindLeafAt(ClientPosition);
    if (LeafIndex < 0)
    {
        return Root.IsEmpty();
    }

    const FLeafGeometry& Leaf = Leaves[LeafIndex];

    const FDockNode* Node = Root.FindByPath(Leaf.Path);
    if (!Node || Node->TabIds.IsEmpty())
    {
        return false;
    }

    OutTargetPanelId = Node->TabIds[Math::Clamp(Node->ActiveTabIndex, 0, Node->TabIds.Size() - 1)];

    const bool bIsOverStrip = Leaf.Strip && Leaf.Strip->GetContentRectangle().EncapsulatesPoint(ClientPosition);
    OutDirection            = bIsOverStrip ? EDockDirection::Center : ResolveDirection(Leaf.Column->GetContentRectangle(), ClientPosition);

    return true;
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

    SetContent(BuildNode(Root, TArray<int32>()));
}

TSharedPtr<FVisualElement> FDockingArea::BuildNode(FDockNode& Node, const TArray<int32>& Path)
{
    if (Node.Kind == EDockNodeKind::Tabs)
    {
        FTabStrip::FDesc StripDesc;
        StripDesc.Font           = Font;
        StripDesc.bAllowTearOut  = bAllowTearOut;
        StripDesc.OnTabActivated = FOnTabActivated::CreateRaw(this, &FDockingArea::OnTabActivated);
        StripDesc.OnTabClosed    = FOnTabClosed::CreateRaw(this, &FDockingArea::OnTabClosed);

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

        FLeafGeometry Leaf;
        Leaf.Strip  = Strip;
        Leaf.Column = Column;
        Leaf.Path   = Path;

        Leaves.Add(Leaf);
        return Column;
    }

    FSplitter::FDesc SplitterDesc;
    SplitterDesc.Orientation     = Node.Orientation;
    SplitterDesc.HandleThickness = FDockMetrics::SplitterThickness;

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
        if (Leaves[Index].Column && Leaves[Index].Column->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            return Index;
        }
    }

    return -1;
}

EDockDirection FDockingArea::ResolveDirection(const FRectangle& Bounds, const IntVector2& ClientPosition)
{
    if (Bounds.IsEmpty())
    {
        return EDockDirection::Center;
    }

    const int32 EdgeWidth  = Math::Max(1, Math::RoundToInt(static_cast<float>(Bounds.Width) * EdgeZoneFraction));
    const int32 EdgeHeight = Math::Max(1, Math::RoundToInt(static_cast<float>(Bounds.Height) * EdgeZoneFraction));
    const int32 FromLeft   = ClientPosition.X - Bounds.Position.X;
    const int32 FromRight  = Bounds.GetRight() - ClientPosition.X;
    const int32 FromTop    = ClientPosition.Y - Bounds.Position.Y;
    const int32 FromBottom = Bounds.GetBottom() - ClientPosition.Y;

    const bool bIsNearLeft   = FromLeft < EdgeWidth;
    const bool bIsNearRight  = FromRight < EdgeWidth;
    const bool bIsNearTop    = FromTop < EdgeHeight;
    const bool bIsNearBottom = FromBottom < EdgeHeight;

    if (!bIsNearLeft && !bIsNearRight && !bIsNearTop && !bIsNearBottom)
    {
        return EDockDirection::Center;
    }

    const int32 HorizontalDistance = Math::Min(FromLeft, FromRight);
    const int32 VerticalDistance   = Math::Min(FromTop, FromBottom);

    if ((bIsNearLeft || bIsNearRight) && (!bIsNearTop && !bIsNearBottom || HorizontalDistance <= VerticalDistance))
    {
        return FromLeft <= FromRight ? EDockDirection::Left : EDockDirection::Right;
    }

    return FromTop <= FromBottom ? EDockDirection::Top : EDockDirection::Bottom;
}

void FDockingArea::DrawDropIndicator(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FDockDragState& DragState = FDockDragState::Get();
    if (!DragState.IsDragging() || DragState.GetTargetArea() != this)
    {
        return;
    }

    const FDockNode* TargetNode = DragState.GetTargetPanelId().IsEmpty() ? &Root : Root.FindTabsNode(DragState.GetTargetPanelId());

    FRectangle LeafBounds = GetContentRectangle();
    for (const FLeafGeometry& Leaf : Leaves)
    {
        if (Root.FindByPath(Leaf.Path) == TargetNode && Leaf.Column)
        {
            LeafBounds = Leaf.Column->GetContentRectangle();
            break;
        }
    }

    LeafBounds = LeafBounds.Deflate(FMargin(DROP_INDICATOR_INSET, DROP_INDICATOR_INSET));
    if (LeafBounds.IsEmpty())
    {
        return;
    }

    FRectangle IndicatorBounds = LeafBounds;
    switch (DragState.GetTargetDirection())
    {
        case EDockDirection::Left:
            IndicatorBounds.Width = LeafBounds.Width / 2;
            break;

        case EDockDirection::Right:
            IndicatorBounds.Width = LeafBounds.Width / 2;
            IndicatorBounds.Position.X += LeafBounds.Width - IndicatorBounds.Width;
            break;

        case EDockDirection::Top:
            IndicatorBounds.Height = LeafBounds.Height / 2;
            break;

        case EDockDirection::Bottom:
            IndicatorBounds.Height = LeafBounds.Height / 2;
            IndicatorBounds.Position.Y += LeafBounds.Height - IndicatorBounds.Height;
            break;

        case EDockDirection::Center:
            break;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    FFloatColor FillColor = Style.Colors.Accent;
    FillColor.A           = 0.25f;

    OutCommandList.AddBox(LayerId, IndicatorBounds, FillColor);
    OutCommandList.AddBoxOutline(LayerId + 1, IndicatorBounds, Style.Colors.Accent, Style.Metrics.BorderThickness);
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

IntVector2 FDockingArea::ToScreenPosition(const IntVector2& ClientPosition) const
{
    if (!FApplication::IsInitialized())
    {
        return ClientPosition;
    }

    TSharedPtr<FWindow> Window = FApplication::Get().FindWindow(const_cast<FDockingArea*>(this)->AsSharedPtr());
    return Window ? ClientPosition + Window->GetPosition() : ClientPosition;
}

void FDockingArea::OnTabDetached(const String& PanelId, const IntVector2& ClientPosition)
{
    const IntVector2 ScreenPosition = ToScreenPosition(ClientPosition);

    FDockDragState::Get().BeginDrag(PanelId, this, ScreenPosition);
    OnPanelTornOutDelegate.ExecuteIfBound(PanelId, ScreenPosition);
}

void FDockingArea::OnTabDragMoved(const String& PanelId, const IntVector2& ClientPosition)
{
    UNREFERENCED_VARIABLE(PanelId);

    FDockDragState::Get().UpdateDrag(ToScreenPosition(ClientPosition));
}

void FDockingArea::OnTabDragFinished(const String& PanelId, const IntVector2& ClientPosition)
{
    UNREFERENCED_VARIABLE(PanelId);

    FDockDragState::Get().UpdateDrag(ToScreenPosition(ClientPosition));
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
