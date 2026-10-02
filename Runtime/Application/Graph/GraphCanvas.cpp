#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Graph/GraphCanvas.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Geometry2D.h"
#include "Core/Math/Math.h"
#include "Core/Templates/NumericLimits.h"

// How many straight segments a link curve is flattened into when measuring the distance to it
constexpr int32 GRAPH_LINK_SEGMENTS = 24;

// The shortest horizontal reach of a link's control points, in graph space
constexpr float GRAPH_LINK_MIN_REACH = 40.0f;

// How thick a link is drawn, and how much thicker the hovered or selected one is
constexpr float GRAPH_LINK_THICKNESS           = 2.0f;
constexpr float GRAPH_LINK_HIGHLIGHT_THICKNESS = 3.5f;

// How far one wheel notch zooms, as a multiplier
constexpr float GRAPH_ZOOM_STEP = 1.1f;

// How much dimmer than the border the grid is drawn
constexpr float GRAPH_GRID_OPACITY = 0.35f;

// How many of the finest grid lines make up one heavier line
constexpr int32 GRAPH_GRID_MAJOR_EVERY = 4;

// The edge of one cell of the grids nodes and links are looked up through, in pixels
constexpr int32 GRAPH_SPATIAL_CELL_SIZE = 64;

// How far outside its rectangle a node is still listed, which covers the grab radius of the pins on its edges
constexpr int32 GRAPH_NODE_GRID_MARGIN = 16;

TSharedPtr<FGraphCanvas> FGraphCanvas::Create(const FDesc& Desc)
{
    TSharedPtr<FGraphCanvas> NewCanvas = MakeSharedPtr<FGraphCanvas>();
    NewCanvas->Initialize(Desc);
    return NewCanvas;
}

FGraphCanvas::FGraphCanvas()
    : FInteractiveElement()
    , Model(nullptr)
    , Font(nullptr)
    , NodeElements()
    , NodeElementIndexById()
    , LinkCurves()
    , LinkPoints()
    , ScratchItemBounds()
    , NodeGrid()
    , LinkGrid()
    , IndexedRevision(-1)
    , bIsSpatialIndexDirty(true)
    , SelectedNodeIds()
    , NodeStyle()
    , BackgroundColor(FUIStyle::GetDefault().Colors.WindowBackground)
    , LinkColor(0.0f, 0.0f, 0.0f, 0.0f)
    , GridColor(0.0f, 0.0f, 0.0f, 0.0f)
    , SurroundColor(0.0f, 0.0f, 0.0f, 0.0f)
    , CornerRadius(0.0f)
    , MarqueeBounds()
    , Pan(0.0f, 0.0f)
    , DraggingToPosition(0.0f, 0.0f)
    , DragAnchor(0, 0)
    , LastDragPosition(0, 0)
    , Zoom(1.0f)
    , FitMinZoom(MinZoom)
    , GridSpacingInGraphSpace(GridSpacing)
    , SelectedLinkId(-1)
    , DraggingFromPinId(-1)
    , HoveredPinId(-1)
    , HoveredLinkId(-1)
    , BuiltRevision(-1)
    , DragMode(EGraphDragMode::None)
    , bShowGrid(true)
    , bIsViewer(false)
    , bHasMovedSincePress(false)
    , OnGetContextMenuDelegate()
    , OnSelectionChangedDelegate()
{
}

FGraphCanvas::~FGraphCanvas() = default;

void FGraphCanvas::Initialize(const FDesc& Desc)
{
    SetDrawCachePolicy(EDrawCachePolicy::Never);

    Font                       = Desc.Font;
    NodeStyle                  = Desc.NodeStyle;
    BackgroundColor            = Desc.BackgroundColor;
    LinkColor                  = Desc.LinkColor;
    GridColor                  = Desc.GridColor;
    SurroundColor              = Desc.SurroundColor;
    CornerRadius               = Math::Max(Desc.CornerRadius, 0.0f);
    GridSpacingInGraphSpace    = Math::Max(Desc.GridSpacing, 1);
    FitMinZoom                 = Math::Clamp(Desc.FitMinZoom, MinZoom, MaxZoom);
    bShowGrid                  = Desc.bShowGrid;
    bIsViewer                  = Desc.bIsViewer;
    OnGetContextMenuDelegate   = Desc.OnGetContextMenu;
    OnSelectionChangedDelegate = Desc.OnSelectionChanged;

    SetModel(Desc.Model);
}

IntVector2 FGraphCanvas::PrepareDesiredSize()
{
    if (Model && Model->GetRevision() != BuiltRevision)
    {
        RebuildElements();
    }

    return FInteractiveElement::PrepareDesiredSize();
}

IntVector2 FGraphCanvas::ComputeDesiredSize() const
{
    return IntVector2(240, 160);
}

void FGraphCanvas::OnArrange(const FRectangle& AllottedBounds)
{
    MeasureNodeElements();

    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        const Vector2    TopLeft = GraphToScreen(Element->GetNode().Position);
        const IntVector2 Size    = Element->GetCachedDesiredSize();

        Element->SetZoom(Zoom);
        Element->Arrange(FRectangle(
            IntVector2(static_cast<int32>(TopLeft.X), static_cast<int32>(TopLeft.Y)),
            static_cast<int32>(static_cast<float>(Size.X) * Zoom),
            static_cast<int32>(static_cast<float>(Size.Y) * Zoom)));
    }

    MarkSpatialIndexDirty();

    UNREFERENCED_VARIABLE(AllottedBounds);
}

EChildVisit FGraphCanvas::VisitChildren(FChildVisitor& Visitor, EChildOrder Order) const
{
    return VisitChildArray(Visitor, Order, NodeElements, [](const TSharedPtr<FGraphNodeElement>& Element) -> const TSharedPtr<FGraphNodeElement>&
    {
        return Element;
    });
}

int32 FGraphCanvas::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (Model && Model->GetRevision() != BuiltRevision)
    {
        RequestContinuousArrange();
    }

    const FUIStyle&   Style  = FUIStyle::GetDefault();
    const FRectangle& Bounds = AllottedGeometry.Bounds;

    const FCornerRadii Radii(CornerRadius);

    OutCommandList.AddBox(LayerId, Bounds, BackgroundColor, Radii);

    if (bShowGrid)
    {
        DrawGrid(Bounds, OutCommandList, LayerId);
    }

    OutCommandList.PushClip(LayerId, Bounds);

    DrawLinks(OutCommandList, LayerId + 1);

    int32 MaxLayerId = LayerId + 2;
    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        const FRectangle& ElementBounds = Element->GetContentRectangle();
        if (ElementBounds.IsEmpty())
        {
            continue;
        }

        const FDrawGeometry ChildGeometry(ElementBounds, AllottedGeometry.Scale);
        MaxLayerId = Element->Draw(ChildGeometry, OutCommandList, MaxLayerId + 1);
    }

    if (DragMode == EGraphDragMode::Marquee && !MarqueeBounds.IsEmpty())
    {
        FFloatColor FillColor = Style.Colors.Accent;
        FillColor.A = 0.15f;

        OutCommandList.AddBox(MaxLayerId + 1, MarqueeBounds, FillColor);
        OutCommandList.AddBoxOutline(MaxLayerId + 1, MarqueeBounds, Style.Colors.Accent, 1.0f);
    }

    OutCommandList.PopClip(MaxLayerId + 1);

    if (CornerRadius > 0.0f && SurroundColor.A > 0.0f)
    {
        OutCommandList.AddPanelChrome(MaxLayerId + 1, Bounds, Radii, 0.0f, FFloatColor(0.0f, 0.0f, 0.0f, 0.0f), SurroundColor);
        return MaxLayerId + 2;
    }

    return MaxLayerId + 1;
}

FEventResponse FGraphCanvas::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    const IntVector2 ClientPosition = CursorEvent.GetClientPosition();
    const FKey       Key            = CursorEvent.GetKey();

    if (Key == Keys::MouseButtonRight)
    {
        OpenContextMenu(CursorEvent);
        return FEventResponse::Handled();
    }

    const bool bIsPanKey = Key == Keys::MouseButtonMiddle || (Key == Keys::MouseButtonLeft && CursorEvent.GetModifierKeys().IsAltDown());
    if (bIsPanKey)
    {
        DragMode = EGraphDragMode::Pan;
    }
    else if (Key == Keys::MouseButtonLeft)
    {
        const int32 PinId  = bIsViewer ? -1 : FindPinAt(ClientPosition);
        const int32 NodeId = PinId >= 0 ? -1 : FindNodeAt(ClientPosition);

        if (PinId >= 0)
        {
            DragMode           = EGraphDragMode::Link;
            DraggingFromPinId  = PinId;
            DraggingToPosition = Vector2(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));
        }
        else if (NodeId >= 0)
        {
            if (!IsNodeSelected(NodeId))
            {
                if (!CursorEvent.GetModifierKeys().IsShiftDown())
                {
                    SelectedNodeIds.Clear();
                }

                SelectedNodeIds.Add(NodeId);
                SelectedLinkId = -1;

                for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
                {
                    Element->SetSelected(IsNodeSelected(Element->GetNodeId()));
                }

                OnSelectionChangedDelegate.ExecuteIfBound();
            }

            DragMode = EGraphDragMode::MoveNodes;
        }
        else
        {
            const int32 LinkId = FindLinkAt(ClientPosition);
            if (LinkId >= 0)
            {
                SelectedNodeIds.Clear();
                SelectedLinkId = LinkId;

                for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
                {
                    Element->SetSelected(false);
                }

                OnSelectionChangedDelegate.ExecuteIfBound();
                return FEventResponse::Handled();
            }

            ClearSelection();

            DragMode      = EGraphDragMode::Marquee;
            MarqueeBounds = FRectangle(ClientPosition, 0, 0);
        }
    }
    else
    {
        return FEventResponse::Unhandled();
    }

    DragAnchor          = ClientPosition;
    LastDragPosition    = ClientPosition;
    bHasMovedSincePress = false;

    BeginPress();
    return FEventResponse::Handled();
}

FEventResponse FGraphCanvas::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    EndDrag(CursorEvent.GetClientPosition());
    return FInteractiveElement::OnMouseButtonUp(CursorEvent);
}

FEventResponse FGraphCanvas::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const FEventResponse Response       = FInteractiveElement::OnMouseMove(CursorEvent);
    const IntVector2     ClientPosition = CursorEvent.GetClientPosition();

    if (DragMode == EGraphDragMode::None)
    {
        HoveredPinId  = FindPinAt(ClientPosition);
        HoveredLinkId = HoveredPinId >= 0 ? -1 : FindLinkAt(ClientPosition);

        for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
        {
            Element->SetHoveredPin(HoveredPinId);
        }
    }

    return Response;
}

FEventResponse FGraphCanvas::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    const float Delta = CursorEvent.GetScrollDelta();
    if (Delta == 0.0f)
    {
        return FEventResponse::Unhandled();
    }

    ZoomAt(CursorEvent.GetClientPosition(), Delta > 0.0f ? GRAPH_ZOOM_STEP : (1.0f / GRAPH_ZOOM_STEP));
    return FEventResponse::Handled();
}

FEventResponse FGraphCanvas::OnKeyDown(const FKeyEvent& KeyEvent)
{
    const FKey Key = KeyEvent.GetKey();
    if (Key == Keys::Delete || Key == Keys::Backspace)
    {
        DeleteSelection();
        return FEventResponse::Handled();
    }

    if (Key == Keys::F)
    {
        FitToNodes();
        return FEventResponse::Handled();
    }

    return FInteractiveElement::OnKeyDown(KeyEvent);
}

bool FGraphCanvas::SupportsKeyboardFocus() const
{
    return true;
}

void FGraphCanvas::SetModel(const TSharedPtr<FGraphModel>& InModel)
{
    Model = InModel;

    ClearSelection();
    RebuildElements();
}

void FGraphCanvas::FitToNodes()
{
    MeasureNodeElements();

    if (NodeElements.IsEmpty())
    {
        Pan  = Vector2(0.0f, 0.0f);
        Zoom = 1.0f;
        return;
    }

    const FRectangle& Bounds = GetContentRectangle();
    if (Bounds.IsEmpty())
    {
        return;
    }

    Vector2 Minimum = Vector2(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
    Vector2 Maximum = Vector2(TNumericLimits<float>::Lowest(), TNumericLimits<float>::Lowest());

    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        const Vector2&   Position = Element->GetNode().Position;
        const IntVector2 Size     = Element->GetCachedDesiredSize();

        Minimum.X = Math::Min(Minimum.X, Position.X);
        Minimum.Y = Math::Min(Minimum.Y, Position.Y);
        Maximum.X = Math::Max(Maximum.X, Position.X + static_cast<float>(Size.X));
        Maximum.Y = Math::Max(Maximum.Y, Position.Y + static_cast<float>(Size.Y));
    }

    const Vector2 GraphSize  = Maximum - Minimum;
    const float   WidthZoom  = GraphSize.X > 0.0f ? (static_cast<float>(Bounds.Width) / GraphSize.X) : MaxZoom;
    const float   HeightZoom = GraphSize.Y > 0.0f ? (static_cast<float>(Bounds.Height) / GraphSize.Y) : MaxZoom;

    Zoom = Math::Clamp(Math::Min(WidthZoom, HeightZoom), FitMinZoom, MaxZoom);

    const Vector2 ScaledSize(GraphSize.X * Zoom, GraphSize.Y * Zoom);
    Pan = Vector2(((static_cast<float>(Bounds.Width) - ScaledSize.X) * 0.5f) - (Minimum.X * Zoom),
        ((static_cast<float>(Bounds.Height) - ScaledSize.Y) * 0.5f) - (Minimum.Y * Zoom));

    ArrangeNodeElements();
}

void FGraphCanvas::AutoLayout()
{
    if (!Model || Model->IsReadOnly())
    {
        return;
    }

    MeasureNodeElements();

    const TArray<FGraphNode>& Nodes = Model->GetNodes();

    TArray<Vector2>    NodeSizes;
    TMap<int32, int32> NodeIndexById;
    NodeSizes.Reserve(Nodes.Size());

    for (const FGraphNode& Node : Nodes)
    {
        NodeIndexById.Add(Node.NodeId, NodeSizes.Size());

        TSharedPtr<FGraphNodeElement> Element = FindNodeElement(Node.NodeId);

        const IntVector2 Size = Element 
            ? Element->GetCachedDesiredSize() 
            : IntVector2(FGraphNodeElement::MinimumWidth, FGraphNodeElement::TitleHeight);

        NodeSizes.Add(Vector2(static_cast<float>(Size.X), static_cast<float>(Size.Y)));
    }

    TArray<FGraphLayoutEdge> Edges;
    for (const FGraphLink& Link : Model->GetLinks())
    {
        int32 FromNodeId = -1;
        int32 ToNodeId   = -1;

        if (!Model->FindPin(Link.FromPinId, FromNodeId) || !Model->FindPin(Link.ToPinId, ToNodeId))
        {
            continue;
        }

        int32 FromIndex = -1;
        int32 ToIndex   = -1;

        for (int32 Index = 0; Index < Nodes.Size(); ++Index)
        {
            FromIndex = Nodes[Index].NodeId == FromNodeId ? Index : FromIndex;
            ToIndex   = Nodes[Index].NodeId == ToNodeId ? Index : ToIndex;
        }

        if (FromIndex >= 0 && ToIndex >= 0)
        {
            Edges.Add(FGraphLayoutEdge(FromIndex, ToIndex));
        }
    }

    TArray<Vector2> Positions;
    FLayeredGraphLayout::Layout(MakeArrayView(NodeSizes), MakeArrayView(Edges), FGraphLayoutSettings(), Positions);

    for (int32 Index = 0; Index < Nodes.Size() && Index < Positions.Size(); ++Index)
    {
        Model->SetNodePosition(Nodes[Index].NodeId, Positions[Index]);
    }

    ArrangeNodeElements();
}

void FGraphCanvas::ZoomAt(const IntVector2& ClientPosition, float ZoomDelta)
{
    const Vector2 Anchor      = Vector2(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));
    const Vector2 GraphAnchor = ScreenToGraph(Anchor);
    const float   NewZoom     = Math::Clamp(Zoom * ZoomDelta, MinZoom, MaxZoom);

    Zoom = NewZoom;

    const FRectangle& Bounds = GetContentRectangle();
    Pan = Vector2(
        Anchor.X - static_cast<float>(Bounds.Position.X) - (GraphAnchor.X * NewZoom),
        Anchor.Y - static_cast<float>(Bounds.Position.Y) - (GraphAnchor.Y * NewZoom));

    MarkViewChanged();
}

void FGraphCanvas::SetZoom(float InZoom)
{
    Zoom = Math::Clamp(InZoom, MinZoom, MaxZoom);
    MarkViewChanged();
}

void FGraphCanvas::SetPan(const Vector2& InPan)
{
    Pan = InPan;
    MarkViewChanged();
}

Vector2 FGraphCanvas::GraphToScreen(const Vector2& GraphPosition) const
{
    const FRectangle& Bounds = GetContentRectangle();
    return Vector2(
        static_cast<float>(Bounds.Position.X) + Pan.X + (GraphPosition.X * Zoom),
        static_cast<float>(Bounds.Position.Y) + Pan.Y + (GraphPosition.Y * Zoom));
}

Vector2 FGraphCanvas::ScreenToGraph(const Vector2& ClientPosition) const
{
    const FRectangle& Bounds = GetContentRectangle();
    return Vector2(
        (ClientPosition.X - static_cast<float>(Bounds.Position.X) - Pan.X) / Zoom,
        (ClientPosition.Y - static_cast<float>(Bounds.Position.Y) - Pan.Y) / Zoom);
}

int32 FGraphCanvas::FindNodeAt(const IntVector2& ClientPosition) const
{
    RefreshSpatialIndex();

    int32 TopIndex = -1;
    NodeGrid.ForEachInCell(ClientPosition, [this, &ClientPosition, &TopIndex](int32 Index)
    {
        if (Index > TopIndex && NodeElements[Index]->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            TopIndex = Index;
        }
    });

    return TopIndex >= 0 ? NodeElements[TopIndex]->GetNodeId() : -1;
}

int32 FGraphCanvas::FindPinAt(const IntVector2& ClientPosition) const
{
    RefreshSpatialIndex();

    int32 TopIndex = -1;
    int32 TopPinId = -1;
    NodeGrid.ForEachInCell(ClientPosition, [this, &ClientPosition, &TopIndex, &TopPinId](int32 Index)
    {
        if (Index <= TopIndex)
        {
            return;
        }

        const int32 PinId = NodeElements[Index]->FindPinAt(ClientPosition);
        if (PinId >= 0)
        {
            TopIndex = Index;
            TopPinId = PinId;
        }
    });

    return TopPinId;
}

int32 FGraphCanvas::FindLinkAt(const IntVector2& ClientPosition) const
{
    if (!Model)
    {
        return -1;
    }

    RefreshSpatialIndex();

    const Vector2 Position(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));

    int32 ClosestLinkId   = -1;
    float ClosestDistance = LinkGrabDistance;

    LinkGrid.ForEachInCell(ClientPosition, [this, &Position, &ClosestLinkId, &ClosestDistance](int32 CurveIndex)
    {
        const FLinkCurve& Curve    = LinkCurves[CurveIndex];
        const float       Distance = Geometry2D::DistanceToPolyline(GetLinkPoints(Curve), Position);
        if (Distance <= ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestLinkId   = Curve.LinkId;
        }
    });

    return ClosestLinkId;
}

void FGraphCanvas::SetSelectedNodes(const TArray<int32>& NodeIds)
{
    SelectedNodeIds = NodeIds;
    SelectedLinkId  = -1;

    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        Element->SetSelected(IsNodeSelected(Element->GetNodeId()));
    }

    OnSelectionChangedDelegate.ExecuteIfBound();
}

void FGraphCanvas::ClearSelection()
{
    if (SelectedNodeIds.IsEmpty() && SelectedLinkId < 0)
    {
        return;
    }

    SelectedNodeIds.Clear();
    SelectedLinkId = -1;

    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        Element->SetSelected(false);
    }

    OnSelectionChangedDelegate.ExecuteIfBound();
}

bool FGraphCanvas::IsNodeSelected(int32 NodeId) const
{
    return SelectedNodeIds.Contains(NodeId);
}

void FGraphCanvas::DeleteSelection()
{
    if (!Model || Model->IsReadOnly() || bIsViewer)
    {
        return;
    }

    if (SelectedLinkId >= 0)
    {
        Model->RemoveLink(SelectedLinkId);
    }

    for (const int32 NodeId : SelectedNodeIds)
    {
        Model->RemoveNode(NodeId);
    }

    SelectedNodeIds.Clear();
    SelectedLinkId = -1;

    OnSelectionChangedDelegate.ExecuteIfBound();
}

TSharedPtr<FGraphNodeElement> FGraphCanvas::FindNodeElement(int32 NodeId) const
{
    const int32* Index = NodeElementIndexById.Find(NodeId);
    return Index ? NodeElements[*Index] : nullptr;
}

void FGraphCanvas::OnDragged(const FCursorEvent& CursorEvent)
{
    const IntVector2 ClientPosition = CursorEvent.GetClientPosition();
    const IntVector2 Delta          = ClientPosition - LastDragPosition;

    LastDragPosition = ClientPosition;

    if (Math::Abs(ClientPosition.X - DragAnchor.X) > DragThreshold || Math::Abs(ClientPosition.Y - DragAnchor.Y) > DragThreshold)
    {
        bHasMovedSincePress = true;
    }

    switch (DragMode)
    {
        case EGraphDragMode::Pan:
        {
            Pan = Pan + Vector2(static_cast<float>(Delta.X), static_cast<float>(Delta.Y));
            MarkViewChanged();
            break;
        }

        case EGraphDragMode::MoveNodes:
        {
            if (!Model || Model->IsReadOnly())
            {
                break;
            }

            const Vector2 GraphDelta(static_cast<float>(Delta.X) / Zoom, static_cast<float>(Delta.Y) / Zoom);
            for (const int32 NodeId : SelectedNodeIds)
            {
                const FGraphNode* Node = Model->FindNode(NodeId);
                if (Node)
                {
                    Model->SetNodePosition(NodeId, Node->Position + GraphDelta);
                }
            }

            MarkViewChanged();
            break;
        }

        case EGraphDragMode::Marquee:
        {
            const IntVector2 Minimum = IntVector2(Math::Min(DragAnchor.X, ClientPosition.X), Math::Min(DragAnchor.Y, ClientPosition.Y));
            const IntVector2 Maximum = IntVector2(Math::Max(DragAnchor.X, ClientPosition.X), Math::Max(DragAnchor.Y, ClientPosition.Y));

            MarqueeBounds = FRectangle(Minimum, Maximum.X - Minimum.X, Maximum.Y - Minimum.Y);
            break;
        }

        case EGraphDragMode::Link:
        {
            DraggingToPosition = Vector2(static_cast<float>(ClientPosition.X), static_cast<float>(ClientPosition.Y));
            HoveredPinId       = FindPinAt(ClientPosition);

            for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
            {
                Element->SetHoveredPin(HoveredPinId);
            }

            break;
        }

        default:
        {
            break;
        }
    }
}

bool FGraphCanvas::AcceptsPressFromKey(FKey Key) const
{
    return Key == Keys::MouseButtonLeft || Key == Keys::MouseButtonMiddle;
}

void FGraphCanvas::RebuildElements()
{
    NodeElements.Clear();
    NodeElementIndexById.Clear();
    MarkSpatialIndexDirty();

    if (!Model)
    {
        BuiltRevision = -1;
        return;
    }

    for (const FGraphNode& Node : Model->GetNodes())
    {
        TSharedPtr<FGraphNodeElement> Element = FGraphNodeElement::Create(Node, Font);
        Element->SetParentElement(AsSharedPtr());
        Element->SetStyle(NodeStyle);
        Element->SetSelected(IsNodeSelected(Node.NodeId));
        Element->SetZoom(Zoom);

        NodeElementIndexById.Add(Node.NodeId, NodeElements.Size());
        NodeElements.Add(Element);
    }

    BuiltRevision = Model->GetRevision();
}

void FGraphCanvas::MeasureNodeElements()
{
    if (Model && Model->GetRevision() != BuiltRevision)
    {
        RebuildElements();
    }

    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        Element->PrepareDesiredSize();
    }
}

void FGraphCanvas::ArrangeNodeElements()
{
    MeasureNodeElements();

    const FRectangle& Bounds = GetContentRectangle();
    if (!Bounds.IsEmpty())
    {
        OnArrange(Bounds);
    }
}

void FGraphCanvas::EndDrag(const IntVector2& ClientPosition)
{
    if (DragMode == EGraphDragMode::Link && Model)
    {
        const int32 TargetPinId = FindPinAt(ClientPosition);
        if (TargetPinId >= 0 && TargetPinId != DraggingFromPinId)
        {
            int32 FromNodeId = -1;
            
            const FGraphPin* FromPin = Model->FindPin(DraggingFromPinId, FromNodeId);
            if (FromPin && FromPin->Direction == EGraphPinDirection::Output)
            {
                Model->AddLink(DraggingFromPinId, TargetPinId);
            }
            else
            {
                Model->AddLink(TargetPinId, DraggingFromPinId);
            }
        }
    }
    else if (DragMode == EGraphDragMode::Marquee && bHasMovedSincePress)
    {
        TArray<int32> Touched;
        for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
        {
            if (!Element->GetContentRectangle().Intersect(MarqueeBounds).IsEmpty())
            {
                Touched.Add(Element->GetNodeId());
            }
        }

        SetSelectedNodes(Touched);
    }

    DragMode          = EGraphDragMode::None;
    DraggingFromPinId = -1;
    MarqueeBounds     = FRectangle();
}

bool FGraphCanvas::GetLinkCurve(const FGraphLink& Link, Vector2& OutStart, Vector2& OutStartControl, Vector2& OutEndControl, Vector2& OutEnd) const
{
    if (!Model)
    {
        return false;
    }

    int32 FromNodeId = -1;
    int32 ToNodeId   = -1;

    if (!Model->FindPin(Link.FromPinId, FromNodeId) || !Model->FindPin(Link.ToPinId, ToNodeId))
    {
        return false;
    }

    TSharedPtr<FGraphNodeElement> FromElement = FindNodeElement(FromNodeId);
    TSharedPtr<FGraphNodeElement> ToElement   = FindNodeElement(ToNodeId);

    if (!FromElement || !ToElement)
    {
        return false;
    }

    if (!FromElement->GetPinCenter(Link.FromPinId, OutStart) || !ToElement->GetPinCenter(Link.ToPinId, OutEnd))
    {
        return false;
    }

    const float Reach = Math::Max(GRAPH_LINK_MIN_REACH * Zoom, Math::Abs(OutEnd.X - OutStart.X) * 0.5f);
    OutStartControl = Vector2(OutStart.X + Reach, OutStart.Y);
    OutEndControl   = Vector2(OutEnd.X - Reach, OutEnd.Y);
    return true;
}

void FGraphCanvas::MarkSpatialIndexDirty()
{
    bIsSpatialIndexDirty = true;
}

void FGraphCanvas::MarkViewChanged()
{
    MarkSpatialIndexDirty();
    InvalidateArrange();
    InvalidatePaint();
}

TArrayView<const Vector2> FGraphCanvas::GetLinkPoints(const FLinkCurve& Curve) const
{
    return TArrayView<const Vector2>(LinkPoints.Data() + Curve.FirstPoint, GRAPH_LINK_SEGMENTS + 1);
}

void FGraphCanvas::RefreshSpatialIndex() const
{
    const int32 ModelRevision = Model ? Model->GetRevision() : -1;
    if (!bIsSpatialIndexDirty && IndexedRevision == ModelRevision)
    {
        return;
    }

    bIsSpatialIndexDirty = false;
    IndexedRevision      = ModelRevision;

    const FRectangle& Bounds = GetContentRectangle();

    ScratchItemBounds.Reset();
    ScratchItemBounds.Reserve(NodeElements.Size());
    for (const TSharedPtr<FGraphNodeElement>& Element : NodeElements)
    {
        ScratchItemBounds.Add(Element->GetContentRectangle().Inflate(FMargin(GRAPH_NODE_GRID_MARGIN)));
    }

    NodeGrid.Build(Bounds, GRAPH_SPATIAL_CELL_SIZE, ScratchItemBounds);

    LinkCurves.Reset();
    LinkPoints.Reset();
    ScratchItemBounds.Reset();

    if (Model)
    {
        const int32 GrabMargin = static_cast<int32>(Math::Ceil(LinkGrabDistance)) + 1;

        for (const FGraphLink& Link : Model->GetLinks())
        {
            FLinkCurve Curve;
            Curve.LinkId     = Link.LinkId;
            Curve.FromPinId  = Link.FromPinId;
            Curve.FirstPoint = LinkPoints.Size();

            if (!GetLinkCurve(Link, Curve.Start, Curve.StartControl, Curve.EndControl, Curve.End))
            {
                continue;
            }

            Geometry2D::FlattenCubicBezier(Curve.Start, Curve.StartControl, Curve.EndControl, Curve.End, GRAPH_LINK_SEGMENTS, LinkPoints);

            Vector2 Minimum = Curve.Start;
            Vector2 Maximum = Curve.Start;
            for (const Vector2& Point : GetLinkPoints(Curve))
            {
                Minimum = Vector2(Math::Min(Minimum.X, Point.X), Math::Min(Minimum.Y, Point.Y));
                Maximum = Vector2(Math::Max(Maximum.X, Point.X), Math::Max(Maximum.Y, Point.Y));
            }

            const IntVector2 MinimumCorner(static_cast<int32>(Math::Floor(Minimum.X)), static_cast<int32>(Math::Floor(Minimum.Y)));
            const IntVector2 MaximumCorner(static_cast<int32>(Math::Ceil(Maximum.X)), static_cast<int32>(Math::Ceil(Maximum.Y)));

            const FRectangle CurveBounds(MinimumCorner, (MaximumCorner.X - MinimumCorner.X) + 1, (MaximumCorner.Y - MinimumCorner.Y) + 1);
            ScratchItemBounds.Add(CurveBounds.Inflate(FMargin(GrabMargin)));
            LinkCurves.Add(Curve);
        }
    }

    LinkGrid.Build(Bounds, GRAPH_SPATIAL_CELL_SIZE, ScratchItemBounds);
}

void FGraphCanvas::DrawGrid(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const float Spacing = static_cast<float>(GridSpacingInGraphSpace) * Zoom;
    if (Spacing < 4.0f)
    {
        return;
    }

    FFloatColor MinorColor = GridColor.A > 0.0f ? GridColor : FUIStyle::GetDefault().Colors.Border;
    MinorColor.A *= GRAPH_GRID_OPACITY;

    FFloatColor MajorColor = GridColor.A > 0.0f ? GridColor : FUIStyle::GetDefault().Colors.Border;
    MajorColor.A *= GRAPH_GRID_OPACITY * 2.0f;

    const float MajorSpacing = Spacing * static_cast<float>(GRAPH_GRID_MAJOR_EVERY);
    const float FirstX       = static_cast<float>(Bounds.Position.X) + Math::FMod(Pan.X, MajorSpacing) - MajorSpacing;
    const float FirstY       = static_cast<float>(Bounds.Position.Y) + Math::FMod(Pan.Y, MajorSpacing) - MajorSpacing;

    int32 Index = 0;
    for (float X = FirstX; X <= static_cast<float>(Bounds.GetRight()); X += Spacing, ++Index)
    {
        if (X < static_cast<float>(Bounds.Position.X))
        {
            continue;
        }

        const bool bIsMajor = (Index % GRAPH_GRID_MAJOR_EVERY) == 0;
        const int32 LineX = Math::RoundToInt(X);
        OutCommandList.AddLine(LayerId, FRectangle(IntVector2(LineX, Bounds.Position.Y), 1, Bounds.Height), bIsMajor ? MajorColor : MinorColor);
    }

    Index = 0;
    for (float Y = FirstY; Y <= static_cast<float>(Bounds.GetBottom()); Y += Spacing, ++Index)
    {
        if (Y < static_cast<float>(Bounds.Position.Y))
        {
            continue;
        }

        const bool bIsMajor = (Index % GRAPH_GRID_MAJOR_EVERY) == 0;
        const int32 LineY = Math::RoundToInt(Y);
        OutCommandList.AddLine(LayerId, FRectangle(IntVector2(Bounds.Position.X, LineY), Bounds.Width, 1), bIsMajor ? MajorColor : MinorColor);
    }
}

void FGraphCanvas::DrawLinks(FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (!Model)
    {
        return;
    }

    RefreshSpatialIndex();

    for (const FLinkCurve& Curve : LinkCurves)
    {
        int32            FromNodeId = -1;
        const FGraphPin* FromPin    = Model->FindPin(Curve.FromPinId, FromNodeId);

        const bool  bIsHighlighted = Curve.LinkId == SelectedLinkId || Curve.LinkId == HoveredLinkId;
        const float Thickness      = bIsHighlighted ? GRAPH_LINK_HIGHLIGHT_THICKNESS : GRAPH_LINK_THICKNESS;

        FFloatColor CurveColor = ResolveLinkColor(FromPin);
        if (Curve.LinkId == SelectedLinkId)
        {
            CurveColor = FUIStyle::GetDefault().Colors.Accent;
        }

        OutCommandList.AddBezier(LayerId, Curve.Start, Curve.StartControl, Curve.EndControl, Curve.End, CurveColor, Thickness);
    }

    if (DragMode != EGraphDragMode::Link || DraggingFromPinId < 0)
    {
        return;
    }

    int32 FromNodeId = -1;
    
    const FGraphPin*              FromPin     = Model->FindPin(DraggingFromPinId, FromNodeId);
    TSharedPtr<FGraphNodeElement> FromElement = FindNodeElement(FromNodeId);

    Vector2 Start;
    if (!FromElement || !FromElement->GetPinCenter(DraggingFromPinId, Start))
    {
        return;
    }

    const bool  bIsFromOutput = FromPin && FromPin->Direction == EGraphPinDirection::Output;
    const float Reach         = Math::Max(GRAPH_LINK_MIN_REACH * Zoom, Math::Abs(DraggingToPosition.X - Start.X) * 0.5f);

    const Vector2 StartControl = Vector2(Start.X + (bIsFromOutput ? Reach : -Reach), Start.Y);
    const Vector2 EndControl   = Vector2(DraggingToPosition.X + (bIsFromOutput ? -Reach : Reach), DraggingToPosition.Y);

    OutCommandList.AddBezier(LayerId, Start, StartControl, EndControl, DraggingToPosition, ResolveLinkColor(FromPin), GRAPH_LINK_THICKNESS);
}

FFloatColor FGraphCanvas::ResolveLinkColor(const FGraphPin* FromPin) const
{
    if (LinkColor.A > 0.0f)
    {
        return LinkColor;
    }

    return FromPin ? FromPin->Tint : FUIStyle::GetDefault().Colors.Text;
}

void FGraphCanvas::OpenContextMenu(const FCursorEvent& CursorEvent)
{
    if (!OnGetContextMenuDelegate.IsBound() || !FApplication::IsInitialized())
    {
        return;
    }

    const Vector2 ClientPosition(static_cast<float>(CursorEvent.GetClientPosition().X), static_cast<float>(CursorEvent.GetClientPosition().Y));

    TSharedPtr<FVisualElement> Menu = OnGetContextMenuDelegate.Execute(
        ScreenToGraph(ClientPosition), FindNodeAt(CursorEvent.GetClientPosition()));
    if (!Menu)
    {
        return;
    }

    FMenuStack::Get().PushMenu(AsSharedPtr(), FRectangle(CursorEvent.GetScreenPosition(), 0, 0), EMenuPlacement::AtCursor, Menu);
}
