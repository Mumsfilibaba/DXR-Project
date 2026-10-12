#include "GraphTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Algorithms/Algorithm.h>
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
    Element->Arrange(Bounds);
}

static FCursorEvent MakeMoveEvent(const FIntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, FIntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, FKey Key, const FIntVector2& ClientPosition)
{
    return FCursorEvent(Type, Key, ClientPosition, FIntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

static FKeyEvent MakeKeyEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

/** @brief Presses, moves and releases, which is one drag end to end. */
static void DragCanvas(const TSharedPtr<FGraphCanvas>& Canvas, const FIntVector2& From, const FIntVector2& To, FKey Key = Keys::MouseButtonLeft)
{
    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Key, From));
    Canvas->OnMouseMove(MakeMoveEvent(To));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Key, To));
}

/** @brief A node with two inputs and one output, which is enough shape for every canvas test. */
static FGraphNode MakeNode(const FString& Title, const FVector2& Position, const FString& TypeTag)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FGraphNode Node;
    Node.Title     = Title;
    Node.Position  = Position;
    Node.TitleTint = Style.Colors.ControlNormal;

    Node.Pins.Add(FGraphPin(EGraphPinDirection::Input, TEXT("A"), TypeTag, Style.Colors.Accent));
    Node.Pins.Add(FGraphPin(EGraphPinDirection::Input, TEXT("B"), TypeTag, Style.Colors.Accent));
    Node.Pins.Add(FGraphPin(EGraphPinDirection::Output, TEXT("Out"), TypeTag, Style.Colors.Accent));

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

static bool FindOutlineHasTint(const FDrawCommandList& CommandList, const FRectangle& Bounds, const FFloatColor& Tint)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::BoxOutline && Command.Bounds == Bounds && Command.HasTint(Tint))
        {
            return true;
        }
    }

    return false;
}

static int32 CountPolylinesTinted(const FDrawCommandList& CommandList, const FFloatColor& Tint)
{
    int32 Count = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        Count += (Command.Type == EDrawCommandType::Polyline && Command.HasTint(Tint)) ? 1 : 0;
    }

    return Count;
}

/** @brief A graph position in the client space the canvas was arranged in. */
static FIntVector2 GraphToClient(const TSharedPtr<FGraphCanvas>& Canvas, const FVector2& GraphPosition)
{
    const FVector2 ClientPosition = Canvas->GraphToScreen(GraphPosition);
    return FIntVector2(static_cast<int32>(ClientPosition.X), static_cast<int32>(ClientPosition.Y));
}

/** @brief Where a pin sits in client space, which is what a link drag has to start and end on. */
static FIntVector2 GetPinPosition(const TSharedPtr<FGraphCanvas>& Canvas, int32 NodeId, int32 PinIndex)
{
    TSharedPtr<FGraphNodeElement> Element = Canvas->FindNodeElement(NodeId);
    if (!Element)
    {
        return FIntVector2(0, 0);
    }

    FVector2 Center(0.0f, 0.0f);
    if (!Element->GetPinCenter(Element->GetNode().Pins[PinIndex].PinId, Center))
    {
        return FIntVector2(0, 0);
    }

    return FIntVector2(static_cast<int32>(Center.X), static_cast<int32>(Center.Y));
}

bool GraphModelEditing_Test()
{
    TEST_BEGIN();

    FGraphModel Model;

    TEST_SECTION(TEXT("A node and its pins are handed ids of their own as they arrive"));
    const int32 FirstId  = Model.AddNode(MakeNode(TEXT("Source"), FVector2(0.0f, 0.0f), TEXT("Texture")));
    const int32 SecondId = Model.AddNode(MakeNode(TEXT("Sink"), FVector2(300.0f, 0.0f), TEXT("Texture")));

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

    TEST_SECTION(TEXT("A pin resolves back to the node holding it"));
    int32 OwningNodeId = -1;
    TEST_EXPECT(Model.FindPin(SourceOutput, OwningNodeId) != nullptr);
    TEST_EXPECT_EQ(OwningNodeId, FirstId);
    TEST_EXPECT(Model.FindPin(9999, OwningNodeId) == nullptr);
    TEST_EXPECT_EQ(OwningNodeId, -1);

    TEST_SECTION(TEXT("A link runs from an output to an input, and refuses to run any other way"));
    TEST_EXPECT(Model.CanConnect(SourceOutput, SinkInput));
    TEST_EXPECT(!Model.CanConnect(SinkInput, SourceOutput));
    TEST_EXPECT(!Model.CanConnect(SourceInput, SinkSecondary));
    TEST_EXPECT(!Model.CanConnect(SourceOutput, SourceInput));

    const int32 LinkId = Model.AddLink(SourceOutput, SinkInput);
    TEST_EXPECT(LinkId >= 0);
    TEST_EXPECT_EQ(Model.GetLinks().Size(), 1);
    TEST_EXPECT_EQ(Model.CountLinksOnPin(SourceOutput), 1);

    TEST_SECTION(TEXT("The same pair cannot be connected twice"));
    TEST_EXPECT(!Model.CanConnect(SourceOutput, SinkInput));
    TEST_EXPECT_EQ(Model.AddLink(SourceOutput, SinkInput), -1);

    TEST_SECTION(TEXT("Type tags gate the connection, and an untagged pin takes anything"));
    const int32 TypedId    = Model.AddNode(MakeNode(TEXT("Buffer"), FVector2(0.0f, 200.0f), TEXT("Buffer")));
    const int32 UntaggedId = Model.AddNode(MakeNode(TEXT("Any"), FVector2(0.0f, 400.0f), TEXT("")));

    const int32 TypedInput     = Model.FindNode(TypedId)->Pins[0].PinId;
    const int32 TypedOutput    = Model.FindNode(TypedId)->Pins[2].PinId;
    const int32 UntaggedOutput = Model.FindNode(UntaggedId)->Pins[2].PinId;

    TEST_EXPECT(!Model.CanConnect(TypedOutput, SinkSecondary));
    TEST_EXPECT(Model.CanConnect(UntaggedOutput, SinkSecondary));

    TEST_SECTION(TEXT("A link that would close a cycle is refused"));
    TEST_EXPECT(!Model.CanConnect(SinkOutput, SourceInput));

    TEST_SECTION(TEXT("Removing a node takes every link touching it"));
    Model.RemoveNode(SecondId);
    TEST_EXPECT_EQ(Model.GetNodes().Size(), 3);
    TEST_EXPECT(Model.GetLinks().IsEmpty());
    TEST_EXPECT(Model.FindLink(LinkId) == nullptr);

    TEST_SECTION(TEXT("Moving a node is an edit like any other"));
    Model.SetNodePosition(FirstId, FVector2(40.0f, 60.0f));
    TEST_EXPECT_EQ(Model.FindNode(FirstId)->Position.X, 40.0f);

    TEST_SECTION(TEXT("Every edit changes the revision, so an observer can tell it is looking at a stale build"));
    const int32 Revision = Model.GetRevision();
    Model.SetNodePosition(FirstId, FVector2(50.0f, 60.0f));
    TEST_EXPECT(Model.GetRevision() != Revision);

    TEST_SECTION(TEXT("A read-only model refuses every edit and keeps its revision"));
    Model.SetReadOnly(true);

    const int32 ReadOnlyRevision = Model.GetRevision();
    TEST_EXPECT_EQ(Model.AddNode(MakeNode(TEXT("Blocked"), FVector2(0.0f, 0.0f), TEXT(""))), -1);
    TEST_EXPECT_EQ(Model.AddLink(UntaggedOutput, TypedInput), -1);

    Model.RemoveNode(FirstId);
    Model.SetNodePosition(FirstId, FVector2(0.0f, 0.0f));

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

    TArray<FVector2>         NoSizes;
    TArray<FGraphLayoutEdge> NoEdges;

    TArray<FVector2> Positions;

    TEST_SECTION(TEXT("An empty graph lays out to nothing at all"));
    FLayeredGraphLayout::Layout(MakeArrayView(NoSizes), MakeArrayView(NoEdges), Settings, Positions);
    TEST_EXPECT(Positions.IsEmpty());

    TEST_SECTION(TEXT("A lone node sits at the origin"));
    TArray<FVector2> OneSize = { FVector2(100.0f, 50.0f) };
    FLayeredGraphLayout::Layout(MakeArrayView(OneSize), MakeArrayView(NoEdges), Settings, Positions);

    TEST_EXPECT_EQ(Positions.Size(), 1);
    TEST_EXPECT_EQ(Positions[0], FVector2(0.0f, 0.0f));

    // A diamond: one source feeding two middles that both feed one sink
    TArray<FVector2> Sizes =
    {
        FVector2(100.0f, 50.0f),
        FVector2(100.0f, 50.0f),
        FVector2(100.0f, 50.0f),
        FVector2(100.0f, 50.0f),
    };

    TArray<FGraphLayoutEdge> Edges =
    {
        FGraphLayoutEdge(0, 1),
        FGraphLayoutEdge(0, 2),
        FGraphLayoutEdge(1, 3),
        FGraphLayoutEdge(2, 3),
    };

    FLayeredGraphLayout::Layout(MakeArrayView(Sizes), MakeArrayView(Edges), Settings, Positions);

    TEST_SECTION(TEXT("Rank puts every node one column right of the furthest thing feeding it"));
    TEST_EXPECT_EQ(Positions.Size(), 4);
    TEST_EXPECT_EQ(Positions[0].X, 0.0f);
    TEST_EXPECT_EQ(Positions[1].X, 200.0f);
    TEST_EXPECT_EQ(Positions[2].X, 200.0f);
    TEST_EXPECT_EQ(Positions[3].X, 400.0f);

    TEST_SECTION(TEXT("Two nodes sharing a column are kept a row apart"));
    TEST_EXPECT(Math::Abs(Positions[1].Y - Positions[2].Y) >= 50.0f + Settings.RowSpacing);

    TEST_SECTION(TEXT("A sink between two feeders is pulled to the middle of them"));
    const float Middle = (Positions[1].Y + Positions[2].Y) * 0.5f;
    TEST_EXPECT(Math::Abs(Positions[3].Y - Middle) < 1.0f);

    TEST_SECTION(TEXT("The layout is a function of its input, so the same graph lays out the same way twice"));
    TArray<FVector2> SecondRun;
    FLayeredGraphLayout::Layout(MakeArrayView(Sizes), MakeArrayView(Edges), Settings, SecondRun);

    for (int32 Index = 0; Index < Positions.Size(); ++Index)
    {
        TEST_EXPECT_EQ(Positions[Index], SecondRun[Index]);
    }

    TEST_SECTION(TEXT("An edge skipping a column is routed through a dummy rather than dragging its end back"));
    TArray<FGraphLayoutEdge> SkippingEdges =
    {
        FGraphLayoutEdge(0, 1),
        FGraphLayoutEdge(1, 2),
        FGraphLayoutEdge(0, 2),
    };

    TArray<FVector2> ThreeSizes = { FVector2(100.0f, 50.0f), FVector2(100.0f, 50.0f), FVector2(100.0f, 50.0f) };
    FLayeredGraphLayout::Layout(MakeArrayView(ThreeSizes), MakeArrayView(SkippingEdges), Settings, Positions);

    TEST_EXPECT_EQ(Positions[0].X, 0.0f);
    TEST_EXPECT_EQ(Positions[1].X, 200.0f);
    TEST_EXPECT_EQ(Positions[2].X, 400.0f);

    TEST_SECTION(TEXT("A column is as wide as its widest node, so the next one clears it"));
    TArray<FVector2> WideSizes = { FVector2(300.0f, 50.0f), FVector2(100.0f, 50.0f) };
    TArray<FGraphLayoutEdge> OneEdge = { FGraphLayoutEdge(0, 1) };

    FLayeredGraphLayout::Layout(MakeArrayView(WideSizes), MakeArrayView(OneEdge), Settings, Positions);
    TEST_EXPECT_EQ(Positions[1].X, 380.0f);

    TEST_SECTION(TEXT("Counting crossings as inversions agrees with testing every pair, shared ends included"));
    {
        TArray<FLayerEdge> LayerEdges;
        uint32 State = 12345u;
        for (int32 Index = 0; Index < 60; ++Index)
        {
            State = (State * 1664525u) + 1013904223u;
            const int32 Upper = static_cast<int32>((State >> 8) % 9u);
            State = (State * 1664525u) + 1013904223u;
            const int32 Lower = static_cast<int32>((State >> 8) % 11u);
            LayerEdges.Add(FLayerEdge{ Upper, Lower });
        }

        Algorithm::Sort(LayerEdges, [](const FLayerEdge& A, const FLayerEdge& B)
        {
            return A.UpperPosition != B.UpperPosition ? A.UpperPosition < B.UpperPosition : A.LowerPosition < B.LowerPosition;
        });

        int64 PairwiseCrossings = 0;
        for (int32 First = 0; First < LayerEdges.Size(); ++First)
        {
            for (int32 Second = First + 1; Second < LayerEdges.Size(); ++Second)
            {
                const int32 UpperDelta = LayerEdges[First].UpperPosition - LayerEdges[Second].UpperPosition;
                const int32 LowerDelta = LayerEdges[First].LowerPosition - LayerEdges[Second].LowerPosition;
                PairwiseCrossings += (UpperDelta * LowerDelta < 0) ? 1 : 0;
            }
        }

        TEST_EXPECT_EQ(FLayeredGraphLayout::CountCrossings(MakeArrayView(LayerEdges), 11), PairwiseCrossings);
    }

    TEST_SECTION(TEXT("A cycle is cut where it closes, so its nodes still land in distinct, finite columns"));
    TArray<FGraphLayoutEdge> CycleEdges = { FGraphLayoutEdge(0, 1), FGraphLayoutEdge(1, 2), FGraphLayoutEdge(2, 0) };
    FLayeredGraphLayout::Layout(MakeArrayView(ThreeSizes), MakeArrayView(CycleEdges), Settings, Positions);

    TEST_EXPECT_EQ(Positions[0].X, 0.0f);
    TEST_EXPECT_EQ(Positions[1].X, 200.0f);
    TEST_EXPECT_EQ(Positions[2].X, 400.0f);

    TEST_SECTION(TEXT("Seed keys order a column where nothing else decides it"));
    TArray<int32> ReversedKeys = { 2, 1, 0 };
    FLayeredGraphLayout::Layout(MakeArrayView(ThreeSizes), MakeArrayView(NoEdges), Settings, Positions, MakeArrayView(ReversedKeys));

    TEST_EXPECT(Positions[2].Y < Positions[1].Y);
    TEST_EXPECT(Positions[1].Y < Positions[0].Y);

    TEST_SECTION(TEXT("A source can be told to keep up with the node before it"));
    FGraphLayoutSettings FollowSettings = Settings;
    FollowSettings.bSourcesFollowPreviousNode = true;

    TArray<FGraphLayoutEdge> ChainThenSource = { FGraphLayoutEdge(0, 1) };
    FLayeredGraphLayout::Layout(MakeArrayView(ThreeSizes), MakeArrayView(ChainThenSource), FollowSettings, Positions);

    TEST_EXPECT_EQ(Positions[1].X, 200.0f);
    TEST_EXPECT_EQ(Positions[2].X, 200.0f);

    TEST_END();
}

bool GraphCanvasView_Test()
{
    TEST_BEGIN();

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();
    const int32 FirstId  = Model->AddNode(MakeNode(TEXT("Source"), FVector2(0.0f, 0.0f), TEXT("Texture")));
    const int32 SecondId = Model->AddNode(MakeNode(TEXT("Sink"), FVector2(400.0f, 200.0f), TEXT("Texture")));

    FGraphCanvas::FDesc Desc;
    Desc.Font  = CreateFont();
    Desc.Model = Model;

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_SECTION(TEXT("The canvas builds one element per node, sized in graph space"));
    TSharedPtr<FGraphNodeElement> SourceElement = Canvas->FindNodeElement(FirstId);
    TEST_EXPECT(SourceElement != nullptr);
    TEST_EXPECT_EQ(SourceElement->GetCachedDesiredSize(), FIntVector2(FGraphNodeElement::MinimumWidth, 72));
    TEST_EXPECT(Canvas->FindNodeElement(999) == nullptr);

    TEST_SECTION(TEXT("At rest, graph space and client space are the same thing"));
    TEST_EXPECT_EQ(Canvas->GetZoom(), 1.0f);
    TEST_EXPECT_EQ(Canvas->GraphToScreen(FVector2(400.0f, 200.0f)), FVector2(400.0f, 200.0f));
    TEST_EXPECT_EQ(Canvas->FindNodeElement(SecondId)->GetContentRectangle().Position, FIntVector2(400, 200));

    TEST_SECTION(TEXT("Panning slides the graph and the two spaces stay each other's inverse"));
    Canvas->SetPan(FVector2(30.0f, -20.0f));
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_EXPECT_EQ(Canvas->GraphToScreen(FVector2(0.0f, 0.0f)), FVector2(30.0f, -20.0f));
    TEST_EXPECT_EQ(Canvas->ScreenToGraph(Canvas->GraphToScreen(FVector2(120.0f, 45.0f))), FVector2(120.0f, 45.0f));

    TEST_SECTION(TEXT("Zoom scales the rectangle a node is arranged into rather than transforming its drawing"));
    Canvas->SetPan(FVector2(0.0f, 0.0f));
    Canvas->SetZoom(2.0f);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    const FRectangle ZoomedBounds = Canvas->FindNodeElement(FirstId)->GetContentRectangle();
    TEST_EXPECT_EQ(ZoomedBounds.Width, FGraphNodeElement::MinimumWidth * 2);
    TEST_EXPECT_EQ(ZoomedBounds.Height, 144);

    TEST_SECTION(TEXT("Zoom stops at the two limits"));
    Canvas->SetZoom(100.0f);
    TEST_EXPECT_EQ(Canvas->GetZoom(), FGraphCanvas::MaxZoom);

    Canvas->SetZoom(0.001f);
    TEST_EXPECT_EQ(Canvas->GetZoom(), FGraphCanvas::MinZoom);

    TEST_SECTION(TEXT("Zooming about a point leaves the graph position under it where it was"));
    Canvas->SetZoom(1.0f);
    Canvas->SetPan(FVector2(0.0f, 0.0f));

    const FIntVector2 Anchor(250, 150);
    const FVector2    GraphUnderAnchor = Canvas->ScreenToGraph(FVector2(250.0f, 150.0f));

    Canvas->ZoomAt(Anchor, 2.0f);
    TEST_EXPECT_EQ(Canvas->GetZoom(), 2.0f);

    const FVector2 AnchorAfter = Canvas->GraphToScreen(GraphUnderAnchor);
    TEST_EXPECT(Math::Abs(AnchorAfter.X - 250.0f) < 0.01f);
    TEST_EXPECT(Math::Abs(AnchorAfter.Y - 150.0f) < 0.01f);

    TEST_SECTION(TEXT("A wheel notch zooms about the cursor, and the far limit stops it"));
    Canvas->SetZoom(FGraphCanvas::MaxZoom);
    Canvas->OnMouseScroll(FCursorEvent(EInputEventType::MouseScrolled, FModifierKeyState(), 1.0f, EScrollAxis::Vertical));
    TEST_EXPECT_EQ(Canvas->GetZoom(), FGraphCanvas::MaxZoom);

    TEST_SECTION(TEXT("Fitting brings every node inside the canvas"));
    Canvas->SetZoom(FGraphCanvas::MaxZoom);
    Canvas->FitToNodes();
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    const FRectangle CanvasBounds = Canvas->GetContentRectangle();
    for (const FGraphNode& Node : Model->GetNodes())
    {
        const FRectangle NodeBounds = Canvas->FindNodeElement(Node.NodeId)->GetContentRectangle();

        TEST_EXPECT(NodeBounds.Position.X >= CanvasBounds.Position.X);
        TEST_EXPECT(NodeBounds.Position.Y >= CanvasBounds.Position.Y);
        TEST_EXPECT(NodeBounds.GetRight() <= CanvasBounds.GetRight());
        TEST_EXPECT(NodeBounds.GetBottom() <= CanvasBounds.GetBottom());
    }

    TEST_SECTION(TEXT("Laying out again writes the positions back into the model"));
    Canvas->SetZoom(1.0f);
    Canvas->AutoLayout();
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, FVector2(0.0f, 0.0f));

    TEST_SECTION(TEXT("A layout run after the arrange pass leaves the rebuilt elements ready to draw"));
    Model->Clear();
    const int32 RebuiltId = Model->AddNode(MakeNode(TEXT("Rebuilt"), FVector2(0.0f, 0.0f), TEXT("Texture")));

    Canvas->AutoLayout();
    Canvas->FitToNodes();

    TSharedPtr<FGraphNodeElement> RebuiltElement = Canvas->FindNodeElement(RebuiltId);
    TEST_EXPECT(RebuiltElement != nullptr);
    TEST_EXPECT(!RebuiltElement->GetContentRectangle().IsEmpty());

    FDrawCommandList RebuiltCommands;
    DrawElement(Canvas, RebuiltCommands);
    TEST_EXPECT(FindBoxCommand(RebuiltCommands, RebuiltElement->GetContentRectangle()) != nullptr);

    TEST_SECTION(TEXT("A canvas with no model still measures and arranges"));
    TSharedPtr<FGraphCanvas> Empty = FGraphCanvas::Create(FGraphCanvas::FDesc());
    LayoutElement(Empty, FRectangle(FIntVector2(0, 0), 200, 200));

    TEST_EXPECT_EQ(Empty->FindNodeAt(FIntVector2(10, 10)), -1);
    TEST_EXPECT_EQ(Empty->FindLinkAt(FIntVector2(10, 10)), -1);

    TEST_SECTION(TEXT("A fit floor keeps a large graph from shrinking below a readable zoom"));
    TSharedPtr<FGraphModel> WideModel = MakeSharedPtr<FGraphModel>();
    WideModel->AddNode(MakeNode(TEXT("Near"), FVector2(0.0f, 0.0f), TEXT("Texture")));
    WideModel->AddNode(MakeNode(TEXT("Far"), FVector2(4000.0f, 0.0f), TEXT("Texture")));

    FGraphCanvas::FDesc FlooredDesc;
    FlooredDesc.Font       = CreateFont();
    FlooredDesc.Model      = WideModel;
    FlooredDesc.FitMinZoom = 1.0f;

    TSharedPtr<FGraphCanvas> Floored = FGraphCanvas::Create(FlooredDesc);
    LayoutElement(Floored, FRectangle(FIntVector2(0, 0), 600, 400));
    Floored->FitToNodes();

    TEST_EXPECT_EQ(Floored->GetZoom(), 1.0f);

    TEST_SECTION(TEXT("Left square, the canvas fills its bounds to the corner and cuts nothing back out"));
    FDrawCommandList SquareCommands;
    DrawElement(Floored, SquareCommands);

    TEST_EXPECT_EQ(SquareCommands.GetCommands()[0].Type, EDrawCommandType::Box);
    TEST_EXPECT_EQ(SquareCommands.GetCommands()[0].CornerRadius.TopLeft, 0.0f);

    TEST_SECTION(TEXT("Given a radius and what it is laid on, it rounds its fill and cuts the corners back out over the grid"));
    constexpr float CanvasRadius = 6.0f;

    FGraphCanvas::FDesc RoundedDesc;
    RoundedDesc.Font          = CreateFont();
    RoundedDesc.Model         = WideModel;
    RoundedDesc.CornerRadius  = CanvasRadius;
    RoundedDesc.SurroundColor = FFloatColor::Red;

    TSharedPtr<FGraphCanvas> Rounded = FGraphCanvas::Create(RoundedDesc);
    LayoutElement(Rounded, FRectangle(FIntVector2(0, 0), 600, 400));

    FDrawCommandList RoundedCommands;
    DrawElement(Rounded, RoundedCommands);

    const FDrawCommand& Fill = RoundedCommands.GetCommands()[0];
    TEST_EXPECT_EQ(Fill.Type, EDrawCommandType::Box);
    TEST_EXPECT_EQ(Fill.CornerRadius.TopLeft, CanvasRadius);
    TEST_EXPECT_EQ(Fill.CornerRadius.BottomRight, CanvasRadius);

    TEST_SECTION(TEXT("Those wedges are the last thing it draws, so the grid and the links cannot square the card off again"));
    int32 WedgeCount   = 0;
    int32 TopWedgeLayer = -1;

    for (const FDrawCommand& Command : RoundedCommands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::CornerWedges && Command.HasTint(FFloatColor::Red))
        {
            ++WedgeCount;
            TopWedgeLayer = Math::Max(TopWedgeLayer, Command.LayerId);

            TEST_EXPECT(Command.CornerRadius == FCornerRadii(CanvasRadius));
        }
    }

    TEST_EXPECT_EQ(WedgeCount, 1);

    for (const FDrawCommand& Command : RoundedCommands.GetCommands())
    {
        if (Command.Type != EDrawCommandType::CornerWedges)
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
    const int32 FirstId  = Model->AddNode(MakeNode(TEXT("Source"), FVector2(20.0f, 20.0f), TEXT("Texture")));
    const int32 SecondId = Model->AddNode(MakeNode(TEXT("Sink"), FVector2(300.0f, 40.0f), TEXT("Texture")));

    int32 SelectionChanges = 0;
    int32 ContextNodeId    = -2;

    FGraphCanvas::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.Model              = Model;
    Desc.OnSelectionChanged = FOnGraphSelectionChanged::CreateLambda([&SelectionChanges]() { SelectionChanges++; });
    Desc.OnGetContextMenu   = FOnGetGraphContextMenu::CreateLambda(
        [&ContextNodeId](const FVector2&, int32 NodeId) -> TSharedPtr<FVisualElement>
    {
        ContextNodeId = NodeId;
        return nullptr;
    });

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    // The title bar is the part of a node that is neither a pin nor a body control
    const FIntVector2 SourceTitle(20 + 70, 20 + 12);
    const FIntVector2 SinkTitle(300 + 70, 40 + 12);
    const FIntVector2 EmptyCanvas(500, 350);

    TEST_SECTION(TEXT("A node context request identifies the node without activating its normal selection action"));
    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonRight, SourceTitle));
    TEST_EXPECT_EQ(ContextNodeId, FirstId);
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION(TEXT("Clicking a node selects it and clicking empty canvas drops the selection"));
    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, SourceTitle));
    TEST_EXPECT(Canvas->IsNodeSelected(FirstId));
    TEST_EXPECT_EQ(Canvas->GetSelectedNodes().Size(), 1);
    TEST_EXPECT_EQ(SelectionChanges, 1);
    TEST_EXPECT(Canvas->FindNodeElement(FirstId)->IsSelected());

    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, SourceTitle));

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, EmptyCanvas));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, EmptyCanvas));
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION(TEXT("Dragging a node moves it in graph space, whatever the zoom is"));
    Canvas->SetZoom(2.0f);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    const FIntVector2 ZoomedTitle(40 + 140, 40 + 24);
    DragCanvas(Canvas, ZoomedTitle, ZoomedTitle + FIntVector2(80, 40));

    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, FVector2(60.0f, 40.0f));

    Model->SetNodePosition(FirstId, FVector2(20.0f, 20.0f));
    Canvas->SetZoom(1.0f);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_SECTION(TEXT("A marquee selects everything it touches and nothing it misses"));
    Canvas->ClearSelection();
    DragCanvas(Canvas, FIntVector2(10, 10), FIntVector2(200, 120));

    TEST_EXPECT_EQ(Canvas->GetSelectedNodes().Size(), 1);
    TEST_EXPECT(Canvas->IsNodeSelected(FirstId));
    TEST_EXPECT(!Canvas->IsNodeSelected(SecondId));
    TEST_EXPECT(Canvas->GetMarqueeBounds().IsEmpty());

    DragCanvas(Canvas, FIntVector2(10, 10), FIntVector2(560, 260));
    TEST_EXPECT_EQ(Canvas->GetSelectedNodes().Size(), 2);

    TEST_SECTION(TEXT("Dragging from an output pin to an input pin makes the link"));
    Canvas->ClearSelection();

    const FIntVector2 SourceOutput = GetPinPosition(Canvas, FirstId, 2);
    const FIntVector2 SinkInput    = GetPinPosition(Canvas, SecondId, 0);

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, SourceOutput));
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::Link);
    TEST_EXPECT(Canvas->GetDraggingFromPin() >= 0);

    Canvas->OnMouseMove(MakeMoveEvent(SinkInput));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, SinkInput));

    TEST_EXPECT_EQ(Model->GetLinks().Size(), 1);
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::None);
    TEST_EXPECT_EQ(Canvas->GetDraggingFromPin(), -1);

    TEST_SECTION(TEXT("A drag that ends on nothing, or on a pin that cannot take it, leaves the graph alone"));
    DragCanvas(Canvas, SourceOutput, EmptyCanvas);
    TEST_EXPECT_EQ(Model->GetLinks().Size(), 1);

    DragCanvas(Canvas, SourceOutput, GetPinPosition(Canvas, FirstId, 0));
    TEST_EXPECT_EQ(Model->GetLinks().Size(), 1);

    TEST_SECTION(TEXT("The link can be picked off the curve it is drawn as"));
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    const FIntVector2 CurveMiddle((SourceOutput.X + SinkInput.X) / 2, (SourceOutput.Y + SinkInput.Y) / 2);
    TEST_EXPECT(Canvas->FindLinkAt(CurveMiddle) >= 0);
    TEST_EXPECT_EQ(Canvas->FindLinkAt(EmptyCanvas), -1);

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, CurveMiddle));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, CurveMiddle));

    TEST_EXPECT(Canvas->GetSelectedLink() >= 0);
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION(TEXT("Delete removes what is selected, links first and then nodes"));
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));
    TEST_EXPECT(Model->GetLinks().IsEmpty());
    TEST_EXPECT_EQ(Model->GetNodes().Size(), 2);

    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));
    Canvas->SetSelectedNodes({ SecondId });
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));

    TEST_EXPECT_EQ(Model->GetNodes().Size(), 1);
    TEST_EXPECT(Model->FindNode(SecondId) == nullptr);

    TEST_SECTION(TEXT("The middle button pans instead of selecting"));
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));
    Canvas->SetPan(FVector2(0.0f, 0.0f));

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonMiddle, EmptyCanvas));
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::Pan);

    Canvas->OnMouseMove(MakeMoveEvent(EmptyCanvas + FIntVector2(-40, 25)));
    TEST_EXPECT_EQ(Canvas->GetPan(), FVector2(-40.0f, 25.0f));

    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonMiddle, EmptyCanvas + FIntVector2(-40, 25)));
    TEST_EXPECT(Canvas->GetDragMode() == EGraphDragMode::None);
    TEST_EXPECT(Canvas->GetSelectedNodes().IsEmpty());

    TEST_SECTION(TEXT("A read-only model is a viewer: it selects, and it does not move or delete anything"));
    Model->SetReadOnly(true);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    const FVector2    PositionBefore = Model->FindNode(FirstId)->Position;
    const FIntVector2 TitleGrab      = GraphToClient(Canvas, PositionBefore) + FIntVector2(70, 12);

    Canvas->SetSelectedNodes({ FirstId });
    DragCanvas(Canvas, TitleGrab, TitleGrab + FIntVector2(30, 30));
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
    const int32 NormalId = Model->AddNode(MakeNode(TEXT("Normal"), FVector2(0.0f, 0.0f), TEXT("Texture")));

    FGraphNode Muted = MakeNode(TEXT("Muted"), FVector2(0.0f, 200.0f), TEXT("Texture"));
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
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_SECTION(TEXT("The desc's style reaches every element the canvas builds"));
    TEST_EXPECT_EQ(Canvas->GetNodeStyle().CornerRadius, 6.0f);
    TEST_EXPECT_EQ(Canvas->FindNodeElement(NormalId)->GetStyle().Body, Body);

    FDrawCommandList CommandList;
    DrawElement(Canvas->FindNodeElement(NormalId), CommandList);

    const FRectangle NodeBounds = Canvas->FindNodeElement(NormalId)->GetContentRectangle();

    TEST_SECTION(TEXT("The body takes the style's fill, rounded on all four corners at the unzoomed radius"));
    const FDrawCommand* BodyCommand = FindBoxCommand(CommandList, NodeBounds);
    TEST_EXPECT(BodyCommand != nullptr);
    TEST_EXPECT(BodyCommand->HasTint(Body));
    TEST_EXPECT_EQ(BodyCommand->CornerRadius.TopLeft, 6.0f);
    TEST_EXPECT_EQ(BodyCommand->CornerRadius.BottomLeft, 6.0f);

    TEST_SECTION(TEXT("The title bar meets the body square, so only its top corners are rounded"));
    const FRectangle    TitleBounds(NodeBounds.Position, NodeBounds.Width, FGraphNodeElement::TitleHeight);
    const FDrawCommand* TitleCommand = FindBoxCommand(CommandList, TitleBounds);

    TEST_EXPECT(TitleCommand != nullptr);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.TopLeft, 6.0f);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.TopRight, 6.0f);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.BottomLeft, 0.0f);
    TEST_EXPECT_EQ(TitleCommand->CornerRadius.BottomRight, 0.0f);

    TEST_SECTION(TEXT("The outline takes the style's border, and every pin is stroked with the outline color"));
    TEST_EXPECT(FindOutlineHasTint(CommandList, NodeBounds, Border));
    TEST_EXPECT_EQ(CountPolylinesTinted(CommandList, PinOutline), 3);

    TEST_SECTION(TEXT("A muted node swaps to the second color set rather than fading the first"));
    FDrawCommandList MutedCommands;
    DrawElement(Canvas->FindNodeElement(MutedId), MutedCommands);

    const FRectangle    MutedBounds = Canvas->FindNodeElement(MutedId)->GetContentRectangle();
    const FDrawCommand* MutedBox    = FindBoxCommand(MutedCommands, MutedBounds);

    TEST_EXPECT(MutedBox != nullptr);
    TEST_EXPECT(MutedBox->HasTint(MutedBody));
    TEST_EXPECT(FindOutlineHasTint(MutedCommands, MutedBounds, MutedBorder));

    TEST_SECTION(TEXT("The radius tracks the zoom, so a node twice the size is rounded twice as far"));
    Canvas->SetZoom(2.0f);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

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
    const int32 NodeId = Model->AddNode(MakeNode(TEXT("Pass"), FVector2(0.0f, 0.0f), TEXT("Texture")));

    FGraphCanvas::FDesc PairedDesc;
    PairedDesc.Font  = CreateFont();
    PairedDesc.Model = Model;

    TSharedPtr<FGraphCanvas> Paired = FGraphCanvas::Create(PairedDesc);
    LayoutElement(Paired, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_SECTION(TEXT("Paired, an input and an output share a row, so two inputs and one output make two"));
    const int32 PairedHeight = Paired->FindNodeElement(NodeId)->GetCachedDesiredSize().Y;
    TEST_EXPECT_EQ(PairedHeight, 72);

    FGraphCanvas::FDesc StackedDesc;
    StackedDesc.Font                    = CreateFont();
    StackedDesc.Model                   = Model;
    StackedDesc.NodeStyle.bStackPinRows = true;

    TSharedPtr<FGraphCanvas> Stacked = FGraphCanvas::Create(StackedDesc);
    LayoutElement(Stacked, FRectangle(FIntVector2(0, 0), 600, 400));

    TSharedPtr<FGraphNodeElement> Element = Stacked->FindNodeElement(NodeId);

    TEST_SECTION(TEXT("Stacked, every pin takes a row of its own, so the same node is a row taller"));
    TEST_EXPECT_EQ(Element->GetCachedDesiredSize().Y, PairedHeight + FGraphNodeElement::PinRowHeight);

    TEST_SECTION(TEXT("The inputs take the first rows and the output follows below both of them"));
    const FGraphNode* Node = Model->FindNode(NodeId);

    FVector2 FirstInput(0.0f, 0.0f);
    FVector2 SecondInput(0.0f, 0.0f);
    FVector2 Output(0.0f, 0.0f);

    TEST_EXPECT(Element->GetPinCenter(Node->Pins[0].PinId, FirstInput));
    TEST_EXPECT(Element->GetPinCenter(Node->Pins[1].PinId, SecondInput));
    TEST_EXPECT(Element->GetPinCenter(Node->Pins[2].PinId, Output));

    TEST_EXPECT_EQ(SecondInput.Y - FirstInput.Y, static_cast<float>(FGraphNodeElement::PinRowHeight));
    TEST_EXPECT_EQ(Output.Y - SecondInput.Y, static_cast<float>(FGraphNodeElement::PinRowHeight));
    TEST_EXPECT(Output.X > SecondInput.X);

    TEST_SECTION(TEXT("A rule separates the two groups, drawn where the outputs begin"));
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

    TEST_SECTION(TEXT("A node with only inputs has nothing to separate"));
    FGraphNode InputsOnly;
    InputsOnly.Title    = TEXT("Sink");
    InputsOnly.Position = FVector2(0.0f, 300.0f);
    InputsOnly.Pins.Add(FGraphPin(EGraphPinDirection::Input, TEXT("A"), TEXT("Texture"), FFloatColor::White));

    const int32 SinkId = Model->AddNode(InputsOnly);
    LayoutElement(Stacked, FRectangle(FIntVector2(0, 0), 600, 400));

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
    const int32 FirstId  = Model->AddNode(MakeNode(TEXT("Source"), FVector2(20.0f, 20.0f), TEXT("Texture")));
    const int32 SecondId = Model->AddNode(MakeNode(TEXT("Sink"), FVector2(300.0f, 40.0f), TEXT("Texture")));

    FGraphCanvas::FDesc Desc;
    Desc.Font      = CreateFont();
    Desc.Model     = Model;
    Desc.bIsViewer = true;

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    TEST_EXPECT(Canvas->IsViewer());

    TEST_SECTION(TEXT("A viewer still selects and still moves what it selected"));
    const FIntVector2 SourceTitle(20 + 70, 20 + 12);
    DragCanvas(Canvas, SourceTitle, SourceTitle + FIntVector2(40, 25));

    TEST_EXPECT(Canvas->IsNodeSelected(FirstId));
    TEST_EXPECT_EQ(Model->FindNode(FirstId)->Position, FVector2(60.0f, 45.0f));

    TEST_SECTION(TEXT("A press on a pin drags the node it belongs to rather than pulling a link off it"));
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    const FIntVector2 SourceOutput = GetPinPosition(Canvas, FirstId, 2);
    const FIntVector2 SinkInput    = GetPinPosition(Canvas, SecondId, 0);

    Canvas->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, SourceOutput));
    TEST_EXPECT(Canvas->GetDragMode() != EGraphDragMode::Link);
    TEST_EXPECT_EQ(Canvas->GetDraggingFromPin(), -1);

    Canvas->OnMouseMove(MakeMoveEvent(SinkInput));
    Canvas->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Keys::MouseButtonLeft, SinkInput));

    TEST_EXPECT(Model->GetLinks().IsEmpty());

    TEST_SECTION(TEXT("Delete leaves the graph alone, however much of it is selected"));
    LayoutElement(Canvas, FRectangle(FIntVector2(0, 0), 600, 400));

    Canvas->SetSelectedNodes({ FirstId, SecondId });
    Canvas->OnKeyDown(MakeKeyEvent(Keys::Delete));

    TEST_EXPECT_EQ(Model->GetNodes().Size(), 2);

    TEST_END();
}
