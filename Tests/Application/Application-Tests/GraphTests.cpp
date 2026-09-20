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

static void DrawElement(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList)
{
    Element->OnDraw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), OutCommandList, 0);
}

static int32 CountCommands(const FDrawCommandList& CommandList, EDrawCommandType Type)
{
    int32 Count = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        Count += Command.Type == Type ? 1 : 0;
    }

    return Count;
}

static const FDrawCommand* FindBoxCommand(const FDrawCommandList& CommandList, const FRectangle& Bounds)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Bounds == Bounds)
        {
            return &Command;
        }
    }

    return nullptr;
}

static FFloatColor FindOutlineColor(const FDrawCommandList& CommandList, const FRectangle& Bounds)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::BoxOutline && Command.Bounds == Bounds)
        {
            return Command.Tint;
        }
    }

    return FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);
}

static int32 CountPolylinesTinted(const FDrawCommandList& CommandList, const FFloatColor& Tint)
{
    int32 Count = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        Count += (Command.Type == EDrawCommandType::Polyline && Command.Tint == Tint) ? 1 : 0;
    }

    return Count;
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
    if (!Element->GetPinCenter(Element->GetNode().Pins[PinIndex].PinId, Center))
    {
        return IntVector2(0, 0);
    }

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

    TEST_SECTION("A layout run after the arrange pass leaves the rebuilt elements ready to draw");
    Model->Clear();
    const int32 RebuiltId = Model->AddNode(MakeNode("Rebuilt", Vector2(0.0f, 0.0f), "Texture"));

    Canvas->AutoLayout();
    Canvas->FitToNodes();

    TSharedPtr<FGraphNodeElement> RebuiltElement = Canvas->FindNodeElement(RebuiltId);
    TEST_EXPECT(RebuiltElement != nullptr);
    TEST_EXPECT(!RebuiltElement->GetContentRectangle().IsEmpty());

    FDrawCommandList RebuiltCommands;
    DrawElement(Canvas, RebuiltCommands);
    TEST_EXPECT(FindBoxCommand(RebuiltCommands, RebuiltElement->GetContentRectangle()) != nullptr);

    TEST_SECTION("A canvas with no model still measures and arranges");
    TSharedPtr<FGraphCanvas> Empty = FGraphCanvas::Create(FGraphCanvas::FDesc());
    LayoutElement(Empty, FRectangle(IntVector2(0, 0), 200, 200));

    TEST_EXPECT_EQ(Empty->FindNodeAt(IntVector2(10, 10)), -1);
    TEST_EXPECT_EQ(Empty->FindLinkAt(IntVector2(10, 10)), -1);

    TEST_SECTION("A fit floor keeps a large graph from shrinking below a readable zoom");
    TSharedPtr<FGraphModel> WideModel = MakeSharedPtr<FGraphModel>();
    WideModel->AddNode(MakeNode("Near", Vector2(0.0f, 0.0f), "Texture"));
    WideModel->AddNode(MakeNode("Far", Vector2(4000.0f, 0.0f), "Texture"));

    FGraphCanvas::FDesc FlooredDesc;
    FlooredDesc.Font       = CreateFont();
    FlooredDesc.Model      = WideModel;
    FlooredDesc.FitMinZoom = 1.0f;

    TSharedPtr<FGraphCanvas> Floored = FGraphCanvas::Create(FlooredDesc);
    LayoutElement(Floored, FRectangle(IntVector2(0, 0), 600, 400));
    Floored->FitToNodes();

    TEST_EXPECT_EQ(Floored->GetZoom(), 1.0f);

    TEST_SECTION("Left square, the canvas fills its bounds to the corner and cuts nothing back out");
    FDrawCommandList SquareCommands;
    DrawElement(Floored, SquareCommands);

    TEST_EXPECT_EQ(SquareCommands.GetCommands()[0].Type, EDrawCommandType::Box);
    TEST_EXPECT_EQ(SquareCommands.GetCommands()[0].CornerRadius.TopLeft, 0.0f);

    TEST_SECTION("Given a radius and what it is laid on, it rounds its fill and cuts the corners back out over the grid");
    constexpr float CanvasRadius = 6.0f;

    FGraphCanvas::FDesc RoundedDesc;
    RoundedDesc.Font          = CreateFont();
    RoundedDesc.Model         = WideModel;
    RoundedDesc.CornerRadius  = CanvasRadius;
    RoundedDesc.SurroundColor = FFloatColor::Red;

    TSharedPtr<FGraphCanvas> Rounded = FGraphCanvas::Create(RoundedDesc);
    LayoutElement(Rounded, FRectangle(IntVector2(0, 0), 600, 400));

    FDrawCommandList RoundedCommands;
    DrawElement(Rounded, RoundedCommands);

    const FDrawCommand& Fill = RoundedCommands.GetCommands()[0];
    TEST_EXPECT_EQ(Fill.Type, EDrawCommandType::Box);
    TEST_EXPECT_EQ(Fill.CornerRadius.TopLeft, CanvasRadius);
    TEST_EXPECT_EQ(Fill.CornerRadius.BottomRight, CanvasRadius);

    TEST_SECTION("Those wedges are the last thing it draws, so the grid and the links cannot square the card off again");
    int32 WedgeCount   = 0;
    int32 TopWedgeLayer = -1;

    for (const FDrawCommand& Command : RoundedCommands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::ConvexPolygon && Command.Tint == FFloatColor::Red)
        {
            ++WedgeCount;
            TopWedgeLayer = Math::Max(TopWedgeLayer, Command.LayerId);
        }
    }

    TEST_EXPECT_EQ(WedgeCount, 4);

    for (const FDrawCommand& Command : RoundedCommands.GetCommands())
    {
        if (Command.Type != EDrawCommandType::ConvexPolygon)
        {
            TEST_EXPECT(Command.LayerId <= TopWedgeLayer);
        }
    }

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

bool GraphNodeStyle_Test()
{
    TEST_BEGIN();

    const FFloatColor Body(0.10f, 0.11f, 0.12f, 1.0f);
    const FFloatColor Border(0.30f, 0.31f, 0.32f, 1.0f);
    const FFloatColor MutedBody(0.01f, 0.02f, 0.03f, 1.0f);
    const FFloatColor MutedBorder(0.04f, 0.05f, 0.06f, 1.0f);
    const FFloatColor PinOutline(0.90f, 0.10f, 0.10f, 1.0f);

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();
    const int32 NormalId = Model->AddNode(MakeNode("Normal", Vector2(0.0f, 0.0f), "Texture"));

    FGraphNode Muted = MakeNode("Muted", Vector2(0.0f, 200.0f), "Texture");
    Muted.bIsMuted   = true;

    const int32 MutedId = Model->AddNode(Muted);

    FGraphCanvas::FDesc Desc;
    Desc.Font                    = CreateFont();
    Desc.Model                   = Model;
    Desc.NodeStyle.Body          = Body;
    Desc.NodeStyle.Border        = Border;
    Desc.NodeStyle.MutedBody     = MutedBody;
    Desc.NodeStyle.MutedBorder   = MutedBorder;
    Desc.NodeStyle.PinOutline    = PinOutline;
    Desc.NodeStyle.CornerRadius  = 6.0f;

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_SECTION("The desc's style reaches every element the canvas builds");
    TEST_EXPECT_EQ(Canvas->GetNodeStyle().CornerRadius, 6.0f);
    TEST_EXPECT_EQ(Canvas->FindNodeElement(NormalId)->GetStyle().Body, Body);

    FDrawCommandList CommandList;
    DrawElement(Canvas->FindNodeElement(NormalId), CommandList);

    const FRectangle NodeBounds = Canvas->FindNodeElement(NormalId)->GetContentRectangle();

    TEST_SECTION("The body takes the style's fill, rounded on all four corners at the unzoomed radius");
    const FDrawCommand* BodyCommand = FindBoxCommand(CommandList, NodeBounds);
    TEST_EXPECT(BodyCommand != nullptr);
    TEST_EXPECT_EQ(BodyCommand->Tint, Body);
    TEST_EXPECT_EQ(BodyCommand->CornerRadius.TopLeft, 6.0f);
    TEST_EXPECT_EQ(BodyCommand->CornerRadius.BottomLeft, 6.0f);

    TEST_SECTION("The title bar meets the body square, so only its top corners are rounded");
    const FRectangle    TitleBounds(NodeBounds.Position, NodeBounds.Width, FGraphNodeElement::TitleHeight);
    const FDrawCommand* TitleCommand = FindBoxCommand(CommandList, TitleBounds);

    TEST_EXPECT(TitleCommand != nullptr);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.TopLeft, 6.0f);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.TopRight, 6.0f);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.BottomLeft, 0.0f);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.BottomRight, 0.0f);

    TEST_SECTION("The outline takes the style's border, and every pin is stroked with the outline color");
    TEST_EXPECT_EQ(FindOutlineColor(CommandList, NodeBounds), Border);
    TEST_EXPECT_EQ(CountPolylinesTinted(CommandList, PinOutline), 3);

    TEST_SECTION("A muted node swaps to the second color set rather than fading the first");
    FDrawCommandList MutedCommands;
    DrawElement(Canvas->FindNodeElement(MutedId), MutedCommands);

    const FRectangle    MutedBounds = Canvas->FindNodeElement(MutedId)->GetContentRectangle();
    const FDrawCommand* MutedBox    = FindBoxCommand(MutedCommands, MutedBounds);

    TEST_EXPECT(MutedBox != nullptr);
    TEST_EXPECT_EQ(MutedBox->Tint, MutedBody);
    TEST_EXPECT_EQ(FindOutlineColor(MutedCommands, MutedBounds), MutedBorder);

    TEST_SECTION("The radius tracks the zoom, so a node twice the size is rounded twice as far");
    Canvas->SetZoom(2.0f);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    FDrawCommandList ZoomedCommands;
    DrawElement(Canvas->FindNodeElement(NormalId), ZoomedCommands);

    const FDrawCommand* ZoomedBody = FindBoxCommand(ZoomedCommands, Canvas->FindNodeElement(NormalId)->GetContentRectangle());
    TEST_EXPECT(ZoomedBody != nullptr);
    TEST_EXPECT_EQ(ZoomedBody->CornerRadius.TopLeft, 12.0f);

    TEST_END();
}

bool GraphStackedPins_Test()
{
    TEST_BEGIN();

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();
    const int32 NodeId = Model->AddNode(MakeNode("Pass", Vector2(0.0f, 0.0f), "Texture"));

    FGraphCanvas::FDesc PairedDesc;
    PairedDesc.Font  = CreateFont();
    PairedDesc.Model = Model;

    TSharedPtr<FGraphCanvas> Paired = FGraphCanvas::Create(PairedDesc);
    LayoutElement(Paired, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_SECTION("Paired, an input and an output share a row, so two inputs and one output make two");
    const int32 PairedHeight = Paired->FindNodeElement(NodeId)->GetCachedDesiredSize().Y;
    TEST_EXPECT_EQ(PairedHeight, 72);

    FGraphCanvas::FDesc StackedDesc;
    StackedDesc.Font                    = CreateFont();
    StackedDesc.Model                   = Model;
    StackedDesc.NodeStyle.bStackPinRows = true;

    TSharedPtr<FGraphCanvas> Stacked = FGraphCanvas::Create(StackedDesc);
    LayoutElement(Stacked, FRectangle(IntVector2(0, 0), 600, 400));

    TSharedPtr<FGraphNodeElement> Element = Stacked->FindNodeElement(NodeId);

    TEST_SECTION("Stacked, every pin takes a row of its own, so the same node is a row taller");
    TEST_EXPECT_EQ(Element->GetCachedDesiredSize().Y, PairedHeight + FGraphNodeElement::PinRowHeight);

    TEST_SECTION("The inputs take the first rows and the output follows below both of them");
    const FGraphNode* Node = Model->FindNode(NodeId);

    Vector2 FirstInput(0.0f, 0.0f);
    Vector2 SecondInput(0.0f, 0.0f);
    Vector2 Output(0.0f, 0.0f);

    TEST_EXPECT(Element->GetPinCenter(Node->Pins[0].PinId, FirstInput));
    TEST_EXPECT(Element->GetPinCenter(Node->Pins[1].PinId, SecondInput));
    TEST_EXPECT(Element->GetPinCenter(Node->Pins[2].PinId, Output));

    TEST_EXPECT_EQ(SecondInput.Y - FirstInput.Y, static_cast<float>(FGraphNodeElement::PinRowHeight));
    TEST_EXPECT_EQ(Output.Y - SecondInput.Y, static_cast<float>(FGraphNodeElement::PinRowHeight));
    TEST_EXPECT(Output.X > SecondInput.X);

    TEST_SECTION("A rule separates the two groups, drawn where the outputs begin");
    FDrawCommandList CommandList;
    DrawElement(Element, CommandList);

    const FRectangle Bounds     = Element->GetContentRectangle();
    bool             bFoundRule = false;

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Line && Command.Bounds.Position.Y == Bounds.Position.Y + FGraphNodeElement::TitleHeight + (FGraphNodeElement::PinRowHeight * 2))
        {
            bFoundRule = true;
        }
    }

    TEST_EXPECT(bFoundRule);

    TEST_SECTION("A node with only inputs has nothing to separate");
    FGraphNode InputsOnly;
    InputsOnly.Title    = "Sink";
    InputsOnly.Position = Vector2(0.0f, 300.0f);
    InputsOnly.Pins.Add(FGraphPin(EGraphPinDirection::Input, "A", "Texture", FFloatColor::White));

    const int32 SinkId = Model->AddNode(InputsOnly);
    LayoutElement(Stacked, FRectangle(IntVector2(0, 0), 600, 400));

    FDrawCommandList SinkCommands;
    DrawElement(Stacked->FindNodeElement(SinkId), SinkCommands);

    TEST_EXPECT_EQ(CountCommands(SinkCommands, EDrawCommandType::Line), 0);

    TEST_END();
}

bool GraphViewerMode_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();
    const int32 FirstId  = Model->AddNode(MakeNode("Source", Vector2(20.0f, 20.0f), "Texture"));
    const int32 SecondId = Model->AddNode(MakeNode("Sink", Vector2(300.0f, 40.0f), "Texture"));

    FGraphCanvas::FDesc Desc;
    Desc.Font      = CreateFont();
    Desc.Model     = Model;
    Desc.bIsViewer = true;

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    TEST_EXPECT(Canvas->IsViewer());

    TEST_SECTION("A viewer still selects and still moves what it selected");
    const IntVector2 SourceTitle(20 + 70, 20 + 12);
    DragCanvas(Canvas, SourceTitle, SourceTitle + IntVector2(40, 25));

    TEST_EXPECT(Canvas->IsNodeSelected(FirstId));
    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, Vector2(60.0f, 45.0f));

    TEST_SECTION("A press on a pin drags the node it belongs to rather than pulling a link off it");
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    const IntVector2 SourceOutput = GetPinPosition(Canvas, FirstId, 2);
    const IntVector2 SinkInput    = GetPinPosition(Canvas, SecondId, 0);

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, SourceOutput));
    TEST_EXPECT(Canvas->GetDragMode() != EGraphDragMode::Link);
    TEST_EXPECT_EQ(Canvas->GetDraggingFromPin(), -1);

    Canvas->OnMouseMove(MakeMoveEvent(SinkInput));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, SinkInput));

    TEST_EXPECT(Model->GetLinks().IsEmpty());

    TEST_SECTION("Delete leaves the graph alone, however much of it is selected");
    LayoutElement(Canvas, FRectangle(IntVector2(0, 0), 600, 400));

    Canvas->SetSelectedNodes({ FirstId, SecondId });
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));

    TEST_EXPECT_EQ(Model->GetNodes().Size(), 2);

    TEST_END();
}
