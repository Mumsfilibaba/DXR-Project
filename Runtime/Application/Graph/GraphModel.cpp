#include "Application/Graph/GraphModel.h"

FGraphModel::FGraphModel()
    : Nodes()
    , Links()
    , NextNodeId(0)
    , NextPinId(0)
    , NextLinkId(0)
    , Revision(0)
    , bIsReadOnly(false)
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
    Revision++;

    return NewNode.NodeId;
}

void FGraphModel::RemoveNode(int32 NodeId)
{
    if (bIsReadOnly)
    {
        return;
    }

    for (int32 Index = 0; Index < Nodes.Size(); ++Index)
    {
        if (Nodes[Index].NodeId != NodeId)
        {
            continue;
        }

        for (int32 LinkIndex = Links.Size() - 1; LinkIndex >= 0; --LinkIndex)
        {
            int32 FromNodeId = -1;
            int32 ToNodeId   = -1;

            FindPin(Links[LinkIndex].FromPinId, FromNodeId);
            FindPin(Links[LinkIndex].ToPinId, ToNodeId);

            if (FromNodeId == NodeId || ToNodeId == NodeId)
            {
                Links.RemoveAt(LinkIndex);
            }
        }

        Nodes.RemoveAt(Index);
        Revision++;
        return;
    }
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

    for (FGraphNode& Node : Nodes)
    {
        if (Node.NodeId == NodeId)
        {
            Node.Position = Position;
            Revision++;
            return;
        }
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
    Revision++;
}

void FGraphModel::SetReadOnly(bool bInIsReadOnly)
{
    bIsReadOnly = bInIsReadOnly;
}

const FGraphNode* FGraphModel::FindNode(int32 NodeId) const
{
    for (const FGraphNode& Node : Nodes)
    {
        if (Node.NodeId == NodeId)
        {
            return &Node;
        }
    }

    return nullptr;
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

    for (const FGraphNode& Node : Nodes)
    {
        for (const FGraphPin& Pin : Node.Pins)
        {
            if (Pin.PinId == PinId)
            {
                OutNodeId = Node.NodeId;
                return &Pin;
            }
        }
    }

    return nullptr;
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

bool FGraphModel::CanReachNode(int32 FromNodeId, int32 TargetNodeId) const
{
    if (FromNodeId == TargetNodeId)
    {
        return true;
    }

    TArray<int32> Pending;
    TArray<int32> Visited;

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

        for (const FGraphLink& Link : Links)
        {
            int32 LinkFromNodeId = -1;
            int32 LinkToNodeId   = -1;

            FindPin(Link.FromPinId, LinkFromNodeId);
            FindPin(Link.ToPinId, LinkToNodeId);

            if (LinkFromNodeId != NodeId)
            {
                continue;
            }

            if (LinkToNodeId == TargetNodeId)
            {
                return true;
            }

            Pending.Add(LinkToNodeId);
        }
    }

    return false;
}
