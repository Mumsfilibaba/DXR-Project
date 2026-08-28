#include "GraphTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Graph/GraphCanvas.h>
#include <Application/Graph/GraphLayout.h>
#include <Application/Graph/GraphModel.h>
#include <Application/Input/Keys.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

/** @brief An eight by sixteen face, so every measurement in these tests is exact. */
static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

/** @brief Measures and arranges an element on its own, the way a window would. */
static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, FKey Key, const IntVector2& ClientPosition)
{
    return FCursorEvent(Type, Key, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

static FKeyEvent MakeKeyEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

/** @brief Presses, moves and releases, which is one drag end to end. */
static void DragCanvas(const TSharedPtr<FGraphCanvas>& Canvas, const IntVector2& From, const IntVector2& To, FKey Key = Keys::MouseButtonLeft)
{
    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Key, From));
    Canvas->OnMouseMove(MakeMoveEvent(To));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Key, To));
}

/** @brief A node with two inputs and one output, which is enough shape for every canvas test. */
static FGraphNode MakeNode(const String& Title, const Vector2& Position, const String& TypeTag)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FGraphNode Node;
    Node.Title     = Title;
    Node.Position  = Position;
    Node.TitleTint = Style.Colors.ControlNormal;

    Node.Pins.Add(FGraphPin(EGraphPinDirection::Input, "A", TypeTag, Style.Colors.Accent));
    Node.Pins.Add(FGraphPin(EGraphPinDirection::Input, "B", TypeTag, Style.Colors.Accent));
    Node.Pins.Add(FGraphPin(EGraphPinDirection::Output, "Out", TypeTag, Style.Colors.Accent));

    return Node;
}

/** @brief A graph position in the client space the canvas was arranged in. */
static IntVector2 GraphToClient(const TSharedPtr<FGraphCanvas>& Canvas, const Vector2& GraphPosition)
{
    const Vector2 ClientPosition = Canvas->GraphToScreen(GraphPosition);
    return IntVector2(static_cast<int32>(ClientPosition.X), static_cast<int32>(ClientPosition.Y));
}

/** @brief Where a pin sits in client space, which is what a link drag has to start and end on. */
static IntVector2 GetPinPosition(const TSharedPtr<FGraphCanvas>& Canvas, int32 NodeId, int32 PinIndex)
{
    TSharedPtr<FGraphNodeElement> Element = Canvas->FindNodeElement(NodeId);
    if (!Element)
    {
        return IntVector2(0, 0);
    }

    Vector2 Center(0.0f, 0.0f);
    Element->GetPinCenter(Element->GetNode().Pins[PinIndex].PinId, Center);

    return IntVector2(static_cast<int32>(Center.X), static_cast<int32>(Center.Y));
}

bool GraphModelEditing_Test()
{
    TEST_BEGIN();

    FGraphModel Model;

    TEST_SECTION("A node and its pins are handed ids of their own as they arrive");
    const int32 FirstId  = Model.AddNode(MakeNode("Source", Vector2(0.0f, 0.0f), "Texture"));
    const int32 SecondId = Model.AddNode(MakeNode("Sink", Vector2(300.0f, 0.0f), "Texture"));

    TEST_EXPECT(FirstId >= 0);
    TEST_EXPECT(SecondId != FirstId);
    TEST_EXPECT_EQ(Model.GetNodes().Size(), 2);

    const FGraphNode* Source = Model.FindNode(FirstId);
    const FGraphNode* Sink   = Model.FindNode(SecondId);
    TEST_EXPECT(Source != nullptr);
    TEST_EXPECT(Sink != nullptr);
    TEST_EXPECT(Source->Pins[0].PinId != Sink->Pins[0].PinId);

    const int32 SourceInput   = Source->Pins[0].PinId;
    const int32 SourceOutput  = Source->Pins[2].PinId;
    const int32 SinkInput     = Sink->Pins[0].PinId;
    const int32 SinkSecondary = Sink->Pins[1].PinId;
    const int32 SinkOutput    = Sink->Pins[2].PinId;

    TEST_SECTION("A pin resolves back to the node holding it");
    int32 OwningNodeId = -1;
    TEST_EXPECT(Model.FindPin(SourceOutput, OwningNodeId) != nullptr);
    TEST_EXPECT_EQ(OwningNodeId, FirstId);
    TEST_EXPECT(Model.FindPin(9999, OwningNodeId) == nullptr);
    TEST_EXPECT_EQ(OwningNodeId, -1);

    TEST_SECTION("A link runs from an output to an input, and refuses to run any other way");
    TEST_EXPECT(Model.CanConnect(SourceOutput, SinkInput));
    TEST_EXPECT(!Model.CanConnect(SinkInput, SourceOutput));
    TEST_EXPECT(!Model.CanConnect(SourceInput, SinkSecondary));
    TEST_EXPECT(!Model.CanConnect(SourceOutput, SourceInput));

    const int32 LinkId = Model.AddLink(SourceOutput, SinkInput);
    TEST_EXPECT(LinkId >= 0);
    TEST_EXPECT_EQ(Model.GetLinks().Size(), 1);
    TEST_EXPECT_EQ(Model.CountLinksOnPin(SourceOutput), 1);

    TEST_SECTION("The same pair cannot be connected twice");
    TEST_EXPECT(!Model.CanConnect(SourceOutput, SinkInput));
    TEST_EXPECT_EQ(Model.AddLink(SourceOutput, SinkInput), -1);

    TEST_SECTION("Type tags gate the connection, and an untagged pin takes anything");
    const int32 TypedId    = Model.AddNode(MakeNode("Buffer", Vector2(0.0f, 200.0f), "Buffer"));
    const int32 UntaggedId = Model.AddNode(MakeNode("Any", Vector2(0.0f, 400.0f), ""));

    const int32 TypedInput     = Model.FindNode(TypedId)->Pins[0].PinId;
    const int32 TypedOutput    = Model.FindNode(TypedId)->Pins[2].PinId;
    const int32 UntaggedOutput = Model.FindNode(UntaggedId)->Pins[2].PinId;

    TEST_EXPECT(!Model.CanConnect(TypedOutput, SinkSecondary));
    TEST_EXPECT(Model.CanConnect(UntaggedOutput, SinkSecondary));

    TEST_SECTION("A link that would close a cycle is refused");
    TEST_EXPECT(!Model.CanConnect(SinkOutput, SourceInput));

    TEST_SECTION("Removing a node takes every link touching it");
    Model.RemoveNode(SecondId);
    TEST_EXPECT_EQ(Model.GetNodes().Size(), 3);
    TEST_EXPECT(Model.GetLinks().IsEmpty());
    TEST_EXPECT(Model.FindLink(LinkId) == nullptr);

    TEST_SECTION("Moving a node is an edit like any other");
    Model.SetNodePosition(FirstId, Vector2(40.0f, 60.0f));
    TEST_EXPECT_EQ(Model.FindNode(FirstId)->Position.X, 40.0f);

    TEST_SECTION("Every edit changes the revision, so an observer can tell it is looking at a stale build");
    const int32 Revision = Model.GetRevision();
    Model.SetNodePosition(FirstId, Vector2(50.0f, 60.0f));
    TEST_EXPECT(Model.GetRevision() != Revision);

    TEST_SECTION("A read-only model refuses every edit and keeps its revision");
    Model.SetReadOnly(true);

    const int32 ReadOnlyRevision = Model.GetRevision();
    TEST_EXPECT_EQ(Model.AddNode(MakeNode("Blocked", Vector2(0.0f, 0.0f), "")), -1);
    TEST_EXPECT_EQ(Model.AddLink(UntaggedOutput, TypedInput), -1);

    Model.RemoveNode(FirstId);
    Model.SetNodePosition(FirstId, Vector2(0.0f, 0.0f));

    TEST_EXPECT_EQ(Model.GetNodes().Size(), 3);
    TEST_EXPECT_EQ(Model.GetRevision(), ReadOnlyRevision);

    TEST_END();
}

bool GraphLayoutLayered_Test()
{
    TEST_BEGIN();

    FGraphLayoutSettings Settings;
    Settings.ColumnSpacing = 80.0f;
    Settings.RowSpacing    = 24.0f;

    TArray<Vector2>          NoSizes;
    TArray<FGraphLayoutEdge> NoEdges;

    TArray<Vector2> Positions;

    TEST_SECTION("An empty graph lays out to nothing at all");
    FGraphLayout::LayoutLayered(MakeArrayView(NoSizes), MakeArrayView(NoEdges), Settings, Positions);
    TEST_EXPECT(Positions.IsEmpty());

    TEST_SECTION("A lone node sits at the origin");
    TArray<Vector2> OneSize = { Vector2(100.0f, 50.0f) };
    FGraphLayout::LayoutLayered(MakeArrayView(OneSize), MakeArrayView(NoEdges), Settings, Positions);

    TEST_EXPECT_EQ(Positions.Size(), 1);
    TEST_EXPECT_EQ(Positions[0], Vector2(0.0f, 0.0f));

    // A diamond: one source feeding two middles that both feed one sink
    TArray<Vector2> Sizes =
    {
        Vector2(100.0f, 50.0f),
        Vector2(100.0f, 50.0f),
        Vector2(100.0f, 50.0f),
        Vector2(100.0f, 50.0f),
    };

    TArray<FGraphLayoutEdge> Edges =
    {
        FGraphLayoutEdge(0, 1),
        FGraphLayoutEdge(0, 2),
        FGraphLayoutEdge(1, 3),
        FGraphLayoutEdge(2, 3),
    };

    FGraphLayout::LayoutLayered(MakeArrayView(Sizes), MakeArrayView(Edges), Settings, Positions);

    TEST_SECTION("Rank puts every node one column right of the furthest thing feeding it");
    TEST_EXPECT_EQ(Positions.Size(), 4);
    TEST_EXPECT_EQ(Positions[0].X, 0.0f);
    TEST_EXPECT_EQ(Positions[1].X, 200.0f);
    TEST_EXPECT_EQ(Positions[2].X, 200.0f);
    TEST_EXPECT_EQ(Positions[3].X, 400.0f);

    TEST_SECTION("Two nodes sharing a column are kept a row apart");
    TEST_EXPECT(Math::Abs(Positions[1].Y - Positions[2].Y) >= 50.0f + Settings.RowSpacing);

    TEST_SECTION("A sink between two feeders is pulled to the middle of them");
    const float Middle = (Positions[1].Y + Positions[2].Y) * 0.5f;
    TEST_EXPECT(Math::Abs(Positions[3].Y - Middle) < 1.0f);

    TEST_SECTION("The layout is a function of its input, so the same graph lays out the same way twice");
    TArray<Vector2> SecondRun;
    FGraphLayout::LayoutLayered(MakeArrayView(Sizes), MakeArrayView(Edges), Settings, SecondRun);

    for (int32 Index = 0; Index < Positions.Size(); ++Index)
    {
        TEST_EXPECT_EQ(Positions[Index], SecondRun[Index]);
    }

    TEST_SECTION("An edge skipping a column is routed through a dummy rather than dragging its end back");
    TArray<FGraphLayoutEdge> SkippingEdges =
    {
        FGraphLayoutEdge(0, 1),
        FGraphLayoutEdge(1, 2),
        FGraphLayoutEdge(0, 2),
    };

    TArray<Vector2> ThreeSizes = { Vector2(100.0f, 50.0f), Vector2(100.0f, 50.0f), Vector2(100.0f, 50.0f) };
    FGraphLayout::LayoutLayered(MakeArrayView(ThreeSizes), MakeArrayView(SkippingEdges), Settings, Positions);

    TEST_EXPECT_EQ(Positions[0].X, 0.0f);
    TEST_EXPECT_EQ(Positions[1].X, 200.0f);
    TEST_EXPECT_EQ(Positions[2].X, 400.0f);

    TEST_SECTION("A column is as wide as its widest node, so the next one clears it");
    TArray<Vector2> WideSizes = { Vector2(300.0f, 50.0f), Vector2(100.0f, 50.0f) };
    TArray<FGraphLayoutEdge> OneEdge = { FGraphLayoutEdge(0, 1) };

    FGraphLayout::LayoutLayered(MakeArrayView(WideSizes), MakeArrayView(OneEdge), Settings, Positions);
    TEST_EXPECT_EQ(Positions[1].X, 380.0f);

    TEST_END();
}

bool GraphCanvasView_Test()
{
    TEST_BEGIN();

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();
    const int32 FirstId  = Model->AddNode(MakeNode("Source", Vector2(0.0f, 0.0f), "Texture"));
    const int32 SecondId = Model->AddNode(MakeNode("Sink", Vector2(400.0f, 200.0f), "Texture"));

    FGraphCanvas::FDesc Desc;
    Desc.Font  = CreateFont();
    Desc.Model = Model;

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_SECTION("The canvas builds one element per node, sized in graph space");
    TSharedPtr<FGraphNodeElement> SourceElement = Canvas->FindNodeElement(FirstId);
    TEST_EXPECT(SourceElement != nullptr);
    TEST_EXPECT_EQ(SourceElement->GetCachedDesiredSize(), IntVector2(FGraphNodeElement::MinimumWidth, 72));
    TEST_EXPECT(Canvas->FindNodeElement(999) == nullptr);

    TEST_SECTION("At rest, graph space and client space are the same thing");
    TEST_EXPECT_EQ(Canvas->GetZoom(), 1.0f);
    TEST_EXPECT_EQ(Canvas->GraphToScreen(Vector2(400.0f, 200.0f)), Vector2(400.0f, 200.0f));
    TEST_EXPECT_EQ(Canvas->FindNodeElement(SecondId)->GetContentRectangle().Position, IntVector2(400, 200));

    TEST_SECTION("Panning slides the graph and the two spaces stay each other's inverse");
    Canvas->SetPan(Vector2(30.0f, -20.0f));
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_EXPECT_EQ(Canvas->GraphToScreen(Vector2(0.0f, 0.0f)), Vector2(30.0f, -20.0f));
    TEST_EXPECT_EQ(Canvas->ScreenToGraph(Canvas->GraphToScreen(Vector2(120.0f, 45.0f))), Vector2(120.0f, 45.0f));

    TEST_SECTION("Zoom scales the rectangle a node is arranged into rather than transforming its drawing");
    Canvas->SetPan(Vector2(0.0f, 0.0f));
    Canvas->SetZoom(2.0f);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    const FRectangle ZoomedBounds = Canvas->FindNodeElement(FirstId)->GetContentRectangle();
    TEST_EXPECT_EQ(ZoomedBounds.Width, FGraphNodeElement::MinimumWidth * 2);
    TEST_EXPECT_EQ(ZoomedBounds.Height, 144);

    TEST_SECTION("Zoom stops at the two limits");
    Canvas->SetZoom(100.0f);
    TEST_EXPECT_EQ(Canvas->GetZoom(), FGraphCanvas::MaxZoom);

    Canvas->SetZoom(0.001f);
    TEST_EXPECT_EQ(Canvas->GetZoom(), FGraphCanvas::MinZoom);

    TEST_SECTION("Zooming about a point leaves the graph position under it where it was");
    Canvas->SetZoom(1.0f);
    Canvas->SetPan(Vector2(0.0f, 0.0f));

    const IntVector2 Anchor(250, 150);
    const Vector2    GraphUnderAnchor = Canvas->ScreenToGraph(Vector2(250.0f, 150.0f));

    Canvas->ZoomAt(Anchor, 2.0f);
    TEST_EXPECT_EQ(Canvas->GetZoom(), 2.0f);

    const Vector2 AnchorAfter = Canvas->GraphToScreen(GraphUnderAnchor);
    TEST_EXPECT(Math::Abs(AnchorAfter.X - 250.0f) < 0.01f);
    TEST_EXPECT(Math::Abs(AnchorAfter.Y - 150.0f) < 0.01f);

    TEST_SECTION("A wheel notch zooms about the cursor, and the far limit stops it");
    Canvas->SetZoom(FGraphCanvas::MaxZoom);
    Canvas->OnMouseScroll(FCursorEvent(EInputEventType::MouseScrolled, FModifierKeyState(), 1.0f, EScrollAxis::Vertical));
    TEST_EXPECT_EQ(Canvas->GetZoom(), FGraphCanvas::MaxZoom);

    TEST_SECTION("Fitting brings every node inside the canvas");
    Canvas->SetZoom(FGraphCanvas::MaxZoom);
    Canvas->FitToNodes();
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    const FRectangle CanvasBounds = Canvas->GetContentRectangle();
    for (const FGraphNode& Node : Model->GetNodes())
    {
        const FRectangle NodeBounds = Canvas->FindNodeElement(Node.NodeId)->GetContentRectangle();

        TEST_EXPECT(NodeBounds.Position.X >= CanvasBounds.Position.X);
        TEST_EXPECT(NodeBounds.Position.Y >= CanvasBounds.Position.Y);
        TEST_EXPECT(NodeBounds.GetRight() <= CanvasBounds.GetRight());
        TEST_EXPECT(NodeBounds.GetBottom() <= CanvasBounds.GetBottom());
    }

    TEST_SECTION("Laying out again writes the positions back into the model");
    Canvas->SetZoom(1.0f);
    Canvas->AutoLayout();
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, Vector2(0.0f, 0.0f));

    TEST_SECTION("A canvas with no model still measures and arranges");
    TSharedPtr<FGraphCanvas> Empty = FGraphCanvas::Create(FGraphCanvas::FDesc());
    LayoutElement(Empty, FRectangle(IntVector2(0, 0), 200, 200));

    TEST_EXPECT_EQ(Empty->FindNodeAt(IntVector2(10, 10)), -1);
    TEST_EXPECT_EQ(Empty->FindLinkAt(IntVector2(10, 10)), -1);

    TEST_END();
}

bool GraphCanvasInteraction_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();
    const int32 FirstId  = Model->AddNode(MakeNode("Source", Vector2(20.0f, 20.0f), "Texture"));
    const int32 SecondId = Model->AddNode(MakeNode("Sink", Vector2(300.0f, 40.0f), "Texture"));

    int32 SelectionChanges = 0;

    FGraphCanvas::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.Model              = Model;
    Desc.OnSelectionChanged = FOnGraphSelectionChanged::CreateLambda([&SelectionChanges]() { SelectionChanges++; });

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    // The title bar is the part of a node that is neither a pin nor a body control
    const IntVector2 SourceTitle(20 + 70, 20 + 12);
    const IntVector2 SinkTitle(300 + 70, 40 + 12);
    const IntVector2 EmptyCanvas(500, 350);

    TEST_SECTION("Clicking a node selects it and clicking empty canvas drops the selection");
    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, SourceTitle));
    TEST_EXPECT(Canvas->IsNodeSelected(FirstId));
    TEST_EXPECT_EQ(Canvas->GetSelectedNodes().Size(), 1);
    TEST_EXPECT_EQ(SelectionChanges, 1);
    TEST_EXPECT(Canvas->FindNodeElement(FirstId)->IsSelected());

    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, SourceTitle));

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, EmptyCanvas));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, EmptyCanvas));
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION("Dragging a node moves it in graph space, whatever the zoom is");
    Canvas->SetZoom(2.0f);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    const IntVector2 ZoomedTitle(40 + 140, 40 + 24);
    DragCanvas(Canvas, ZoomedTitle, ZoomedTitle + IntVector2(80, 40));

    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, Vector2(60.0f, 40.0f));

    Model->SetNodePosition(FirstId, Vector2(20.0f, 20.0f));
    Canvas->SetZoom(1.0f);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_SECTION("A marquee selects everything it touches and nothing it misses");
    Canvas->ClearSelection();
    DragCanvas(Canvas, IntVector2(10, 10), IntVector2(200, 120));

    TEST_EXPECT_EQ(Canvas->GetSelectedNodes().Size(), 1);
    TEST_EXPECT(Canvas->IsNodeSelected(FirstId));
    TEST_EXPECT(!Canvas->IsNodeSelected(SecondId));
    TEST_EXPECT(Canvas->GetMarqueeBounds().IsEmpty());

    DragCanvas(Canvas, IntVector2(10, 10), IntVector2(560, 260));
    TEST_EXPECT_EQ(Canvas->GetSelectedNodes().Size(), 2);

    TEST_SECTION("Dragging from an output pin to an input pin makes the link");
    Canvas->ClearSelection();

    const IntVector2 SourceOutput = GetPinPosition(Canvas, FirstId, 2);
    const IntVector2 SinkInput    = GetPinPosition(Canvas, SecondId, 0);

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, SourceOutput));
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::Link);
    TEST_EXPECT(Canvas->GetDraggingFromPin() >= 0);

    Canvas->OnMouseMove(MakeMoveEvent(SinkInput));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, SinkInput));

    TEST_EXPECT_EQ(Model->GetLinks().Size(), 1);
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::None);
    TEST_EXPECT_EQ(Canvas->GetDraggingFromPin(), -1);

    TEST_SECTION("A drag that ends on nothing, or on a pin that cannot take it, leaves the graph alone");
    DragCanvas(Canvas, SourceOutput, EmptyCanvas);
    TEST_EXPECT_EQ(Model->GetLinks().Size(), 1);

    DragCanvas(Canvas, SourceOutput, GetPinPosition(Canvas, FirstId, 0));
    TEST_EXPECT_EQ(Model->GetLinks().Size(), 1);

    TEST_SECTION("The link can be picked off the curve it is drawn as");
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    const IntVector2 CurveMiddle((SourceOutput.X + SinkInput.X) / 2, (SourceOutput.Y + SinkInput.Y) / 2);
    TEST_EXPECT(Canvas->FindLinkAt(CurveMiddle) >= 0);
    TEST_EXPECT_EQ(Canvas->FindLinkAt(EmptyCanvas), -1);

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, CurveMiddle));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, CurveMiddle));

    TEST_EXPECT(Canvas->GetSelectedLink() >= 0);
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION("Delete removes what is selected, links first and then nodes");
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));
    TEST_EXPECT(Model->GetLinks().IsEmpty());
    TEST_EXPECT_EQ(Model->GetNodes().Size(), 2);

    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));
    Canvas->SetSelectedNodes({ SecondId });
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));

    TEST_EXPECT_EQ(Model->GetNodes().Size(), 1);
    TEST_EXPECT(Model->FindNode(SecondId) == nullptr);

    TEST_SECTION("The middle button pans instead of selecting");
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));
    Canvas->SetPan(Vector2(0.0f, 0.0f));

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonMiddle, EmptyCanvas));
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::Pan);

    Canvas->OnMouseMove(MakeMoveEvent(EmptyCanvas + IntVector2(-40, 25)));
    TEST_EXPECT_EQ(Canvas->GetPan(), Vector2(-40.0f, 25.0f));

    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonMiddle, EmptyCanvas + IntVector2(-40, 25)));
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::None);
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION("A read-only model is a viewer: it selects, and it does not move or delete anything");
    Model->SetReadOnly(true);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    const Vector2    PositionBefore = Model->FindNode(FirstId)->Position;
    const IntVector2 TitleGrab      = GraphToClient(Canvas, PositionBefore) + IntVector2(70, 12);

    Canvas->SetSelectedNodes({ FirstId });
    DragCanvas(Canvas, TitleGrab, TitleGrab + IntVector2(30, 30));
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));

    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, PositionBefore);
    TEST_EXPECT_EQ(Model->GetNodes().Size(), 1);

    TEST_END();
}
