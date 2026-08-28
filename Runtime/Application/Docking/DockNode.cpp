#include "Application/Docking/DockNode.h"
#include "Core/Math/Math.h"

FDockNode::FDockNode()
    : Kind(EDockNodeKind::Tabs)
    , Orientation(EDockSplitOrientation::Horizontal)
    , ChildFractions()
    , Children()
    , TabIds()
    , ActiveTabIndex(0)
{
}

FDockNode::FDockNode(const FDockNode& Other) = default;

FDockNode::FDockNode(FDockNode&& Other) = default;

FDockNode::~FDockNode() = default;

FDockNode& FDockNode::operator=(const FDockNode& Other) = default;

FDockNode& FDockNode::operator=(FDockNode&& Other) = default;

FDockNode FDockNode::MakeTabs(const TArray<String>& InTabIds)
{
    FDockNode Node;
    Node.Kind   = EDockNodeKind::Tabs;
    Node.TabIds = InTabIds;
    return Node;
}

FDockNode FDockNode::MakeSplit(EDockSplitOrientation InOrientation, const FDockNode& First, const FDockNode& Second)
{
    FDockNode Node;
    Node.Kind        = EDockNodeKind::Split;
    Node.Orientation = InOrientation;

    Node.Children.Add(First);
    Node.Children.Add(Second);
    Node.NormalizeFractions();
    return Node;
}

IntVector2 FDockNode::ComputeMinimumSize() const
{
    if (Kind == EDockNodeKind::Tabs)
    {
        return IntVector2(FDockMetrics::MinimumPanelWidth, FDockMetrics::MinimumPanelHeight + FDockMetrics::TabStripHeight);
    }

    IntVector2 Minimum(0, 0);
    for (const FDockNode& Child : Children)
    {
        const IntVector2 ChildMinimum = Child.ComputeMinimumSize();
        if (Orientation == EDockSplitOrientation::Horizontal)
        {
            Minimum.X += ChildMinimum.X;
            Minimum.Y = Math::Max(Minimum.Y, ChildMinimum.Y);
        }
        else
        {
            Minimum.X = Math::Max(Minimum.X, ChildMinimum.X);
            Minimum.Y += ChildMinimum.Y;
        }
    }

    const int32 NumHandles  = Math::Max(0, Children.Size() - 1);
    const int32 HandleSpace = NumHandles * FDockMetrics::SplitterThickness;

    if (Orientation == EDockSplitOrientation::Horizontal)
    {
        Minimum.X += HandleSpace;
    }
    else
    {
        Minimum.Y += HandleSpace;
    }

    return Minimum;
}

bool FDockNode::IsEmpty() const
{
    return Kind == EDockNodeKind::Tabs ? TabIds.IsEmpty() : Children.IsEmpty();
}

void FDockNode::GatherPanelIds(TArray<String>& OutPanelIds) const
{
    if (Kind == EDockNodeKind::Tabs)
    {
        for (const String& TabId : TabIds)
        {
            OutPanelIds.Add(TabId);
        }

        return;
    }

    for (const FDockNode& Child : Children)
    {
        Child.GatherPanelIds(OutPanelIds);
    }
}

FDockNode* FDockNode::FindTabsNode(const String& PanelId)
{
    return const_cast<FDockNode*>(static_cast<const FDockNode*>(this)->FindTabsNode(PanelId));
}

const FDockNode* FDockNode::FindTabsNode(const String& PanelId) const
{
    if (Kind == EDockNodeKind::Tabs)
    {
        return TabIds.Contains(PanelId) ? this : nullptr;
    }

    for (const FDockNode& Child : Children)
    {
        if (const FDockNode* Found = Child.FindTabsNode(PanelId))
        {
            return Found;
        }
    }

    return nullptr;
}

FDockNode* FDockNode::FindByPath(const TArray<int32>& Path)
{
    return const_cast<FDockNode*>(static_cast<const FDockNode*>(this)->FindByPath(Path));
}

const FDockNode* FDockNode::FindByPath(const TArray<int32>& Path) const
{
    const FDockNode* Node = this;
    for (int32 Index : Path)
    {
        if (Node->Kind != EDockNodeKind::Split || Index < 0 || Index >= Node->Children.Size())
        {
            return nullptr;
        }

        Node = &Node->Children[Index];
    }

    return Node;
}

void FDockNode::CollapseDegenerateNodes()
{
    if (Kind != EDockNodeKind::Split)
    {
        ActiveTabIndex = TabIds.IsEmpty() ? 0 : Math::Clamp(ActiveTabIndex, 0, TabIds.Size() - 1);
        return;
    }

    for (FDockNode& Child : Children)
    {
        Child.CollapseDegenerateNodes();
    }

    for (int32 Index = Children.Size() - 1; Index >= 0; --Index)
    {
        if (Children[Index].IsEmpty())
        {
            Children.RemoveAt(Index);

            if (Index < ChildFractions.Size())
            {
                ChildFractions.RemoveAt(Index);
            }
        }
    }

    for (int32 Index = Children.Size() - 1; Index >= 0; --Index)
    {
        FDockNode& Child = Children[Index];
        if (Child.Kind != EDockNodeKind::Split || Child.Orientation != Orientation)
        {
            continue;
        }

        const float     ChildShare    = Index < ChildFractions.Size() ? ChildFractions[Index] : 0.0f;
        const FDockNode AbsorbedChild = Child;

        Children.RemoveAt(Index);
        if (Index < ChildFractions.Size())
        {
            ChildFractions.RemoveAt(Index);
        }

        for (int32 GrandChildIndex = 0; GrandChildIndex < AbsorbedChild.Children.Size(); ++GrandChildIndex)
        {
            const float GrandChildShare = GrandChildIndex < AbsorbedChild.ChildFractions.Size() ? AbsorbedChild.ChildFractions[GrandChildIndex] : 0.0f;

            Children.Insert(Index + GrandChildIndex, AbsorbedChild.Children[GrandChildIndex]);
            ChildFractions.Insert(Index + GrandChildIndex, ChildShare * GrandChildShare);
        }
    }

    if (Children.Size() == 1)
    {
        FDockNode OnlyChild = Children[0];
        *this = OnlyChild;
        return;
    }

    if (Children.IsEmpty())
    {
        Kind = EDockNodeKind::Tabs;
        ChildFractions.Clear();
        TabIds.Clear();
        ActiveTabIndex = 0;
        return;
    }

    if (ChildFractions.Size() != Children.Size())
    {
        NormalizeFractions();
        return;
    }

    float Total = 0.0f;
    for (float Fraction : ChildFractions)
    {
        Total += Fraction;
    }

    if (Total <= 0.0f)
    {
        NormalizeFractions();
        return;
    }

    for (float& Fraction : ChildFractions)
    {
        Fraction /= Total;
    }
}

void FDockNode::NormalizeFractions()
{
    ChildFractions.Clear();

    if (Children.IsEmpty())
    {
        return;
    }

    const float EvenShare = 1.0f / static_cast<float>(Children.Size());
    for (int32 Index = 0; Index < Children.Size(); ++Index)
    {
        ChildFractions.Add(EvenShare);
    }
}
