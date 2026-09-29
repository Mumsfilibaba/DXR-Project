#include "Application/Graph/GraphModel.h"
#include "Core/Containers/Set.h"

FGraphModel::FGraphModel()
    : Nodes()
    , Links()
    , NodeIndexById()
    , PinLocationById()
    , NextNodeId(0)
    , NextPinId(0)
    , NextLinkId(0)
    , Revision(0)
    , bIsReadOnly(false)
    , bIsIndexDirty(false)
{
}

FGraphModel::~FGraphModel() = default;

int32 FGraphModel::AddNode(const FGraphNode& Node)
{
    if (bIsReadOnly)
    {
        return -1;
    }

    FGraphNode NewNode = Node;
    NewNode.NodeId     = NextNodeId++;

    for (FGraphPin& Pin : NewNode.Pins)
    {
        Pin.PinId = NextPinId++;
    }

    Nodes.Add(NewNode);
    MarkIndexDirty();
    Revision++;

    return NewNode.NodeId;
}

void FGraphModel::RemoveNode(int32 NodeId)
{
    if (bIsReadOnly)
    {
        return;
    }

    const int32 Index = FindNodeIndex(NodeId);
    if (Index < 0)
    {
        return;
    }

    for (int32 LinkIndex = Links.Size() - 1; LinkIndex >= 0; --LinkIndex)
    {
        int32 FromNodeId = -1;
        int32 ToNodeId   = -1;

        if ((FindPin(Links[LinkIndex].FromPinId, FromNodeId) && FromNodeId == NodeId)
            || (FindPin(Links[LinkIndex].ToPinId, ToNodeId) && ToNodeId == NodeId))
        {
            Links.RemoveAt(LinkIndex);
        }
    }

    Nodes.RemoveAt(Index);
    MarkIndexDirty();
    Revision++;
}

int32 FGraphModel::AddLink(int32 FromPinId, int32 ToPinId)
{
    if (bIsReadOnly || !CanConnect(FromPinId, ToPinId))
    {
        return -1;
    }

    const int32 NewLinkId = NextLinkId++;
    Links.Add(FGraphLink(NewLinkId, FromPinId, ToPinId));
    Revision++;

    return NewLinkId;
}

void FGraphModel::RemoveLink(int32 LinkId)
{
    if (bIsReadOnly)
    {
        return;
    }

    for (int32 Index = 0; Index < Links.Size(); ++Index)
    {
        if (Links[Index].LinkId == LinkId)
        {
            Links.RemoveAt(Index);
            Revision++;
            return;
        }
    }
}

bool FGraphModel::CanConnect(int32 FromPinId, int32 ToPinId) const
{
    int32 FromNodeId = -1;
    int32 ToNodeId   = -1;

    const FGraphPin* FromPin = FindPin(FromPinId, FromNodeId);
    const FGraphPin* ToPin   = FindPin(ToPinId, ToNodeId);

    if (!FromPin || !ToPin || FromNodeId == ToNodeId)
    {
        return false;
    }

    if (FromPin->Direction != EGraphPinDirection::Output || ToPin->Direction != EGraphPinDirection::Input)
    {
        return false;
    }

    if (!FromPin->TypeTag.IsEmpty() && !ToPin->TypeTag.IsEmpty() && FromPin->TypeTag != ToPin->TypeTag)
    {
        return false;
    }

    for (const FGraphLink& Link : Links)
    {
        if (Link.FromPinId == FromPinId && Link.ToPinId == ToPinId)
        {
            return false;
        }
    }

    return !CanReachNode(ToNodeId, FromNodeId);
}

void FGraphModel::SetNodePosition(int32 NodeId, const Vector2& Position)
{
    if (bIsReadOnly)
    {
        return;
    }

    const int32 Index = FindNodeIndex(NodeId);
    if (Index >= 0)
    {
        Nodes[Index].Position = Position;
        Revision++;
    }
}

void FGraphModel::Clear()
{
    if (bIsReadOnly)
    {
        return;
    }

    Nodes.Clear();
    Links.Clear();
    MarkIndexDirty();
    Revision++;
}

void FGraphModel::SetReadOnly(bool bInIsReadOnly)
{
    bIsReadOnly = bInIsReadOnly;
}

const FGraphNode* FGraphModel::FindNode(int32 NodeId) const
{
    const int32 Index = FindNodeIndex(NodeId);
    return Index >= 0 ? &Nodes[Index] : nullptr;
}

const FGraphLink* FGraphModel::FindLink(int32 LinkId) const
{
    for (const FGraphLink& Link : Links)
    {
        if (Link.LinkId == LinkId)
        {
            return &Link;
        }
    }

    return nullptr;
}

const FGraphPin* FGraphModel::FindPin(int32 PinId, int32& OutNodeId) const
{
    OutNodeId = -1;

    EnsureIndex();

    const FPinLocation* Location = PinLocationById.Find(PinId);
    if (!Location)
    {
        return nullptr;
    }

    const FGraphNode& Node = Nodes[Location->NodeIndex];
    OutNodeId = Node.NodeId;
    return &Node.Pins[Location->PinIndex];
}

int32 FGraphModel::CountLinksOnPin(int32 PinId) const
{
    int32 Count = 0;
    for (const FGraphLink& Link : Links)
    {
        Count += (Link.FromPinId == PinId || Link.ToPinId == PinId) ? 1 : 0;
    }

    return Count;
}

int32 FGraphModel::FindNodeIndex(int32 NodeId) const
{
    EnsureIndex();

    const int32* Index = NodeIndexById.Find(NodeId);
    return Index ? *Index : -1;
}

void FGraphModel::EnsureIndex() const
{
    if (!bIsIndexDirty)
    {
        return;
    }

    NodeIndexById.Clear();
    PinLocationById.Clear();

    for (int32 NodeIndex = 0; NodeIndex < Nodes.Size(); ++NodeIndex)
    {
        const FGraphNode& Node = Nodes[NodeIndex];
        NodeIndexById.Add(Node.NodeId, NodeIndex);

        for (int32 PinIndex = 0; PinIndex < Node.Pins.Size(); ++PinIndex)
        {
            PinLocationById.Add(Node.Pins[PinIndex].PinId, FPinLocation{ NodeIndex, PinIndex });
        }
    }

    bIsIndexDirty = false;
}

void FGraphModel::MarkIndexDirty()
{
    bIsIndexDirty = true;
}

bool FGraphModel::CanReachNode(int32 FromNodeId, int32 TargetNodeId) const
{
    if (FromNodeId == TargetNodeId)
    {
        return true;
    }

    TMap<int32, TArray<int32>> Successors;
    for (const FGraphLink& Link : Links)
    {
        int32 LinkFromNodeId = -1;
        int32 LinkToNodeId   = -1;

        if (FindPin(Link.FromPinId, LinkFromNodeId) && FindPin(Link.ToPinId, LinkToNodeId))
        {
            Successors.FindOrAdd(LinkFromNodeId).Add(LinkToNodeId);
        }
    }

    TArray<int32> Pending;
    TSet<int32>   Visited;

    Pending.Add(FromNodeId);

    while (!Pending.IsEmpty())
    {
        const int32 NodeId = Pending.Last();
        Pending.Pop();

        if (Visited.Contains(NodeId))
        {
            continue;
        }

        Visited.Add(NodeId);

        const TArray<int32>* NextNodes = Successors.Find(NodeId);
        if (!NextNodes)
        {
            continue;
        }

        for (const int32 SuccessorId : *NextNodes)
        {
            if (SuccessorId == TargetNodeId)
            {
                return true;
            }

            Pending.Add(SuccessorId);
        }
    }

    return false;
}
