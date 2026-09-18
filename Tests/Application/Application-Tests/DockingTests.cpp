#include "DockingTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/IniFile.h>
#include <Core/Misc/Paths.h>
#include <Application/Docking/DockDragState.h>
#include <Application/Docking/DockLayoutFile.h>
#include <Application/Docking/DockNode.h>
#include <Application/Docking/DockWindowManager.h>
#include <Application/Docking/DockingArea.h>
#include <Application/Docking/Splitter.h>
#include <Application/Docking/TabStrip.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/ElementPath.h>
#include <Application/Elements/ScrollBar.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TrueTypeFontFace.h>

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
}

static TSharedPtr<FVisualElement> MakePanel(const String& Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = Font;

    return FTextBlock::Create(Desc);
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition, bool bIsDown)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(), bIsDown);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeScrollEvent(float Delta, EScrollAxis Axis = EScrollAxis::Vertical)
{
    return FCursorEvent(EInputEventType::MouseScrolled, FModifierKeyState(), Delta, Axis);
}

static void DragSplitterHandle(const TSharedPtr<FSplitter>& Splitter, int32 HandleIndex, const IntVector2& Delta)
{
    const IntVector2 Start = Splitter->GetHandleRectangle(HandleIndex).GetCenter();
    const IntVector2 End   = Start + Delta;

    Splitter->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Start, true));
    Splitter->OnMouseMove(MakeMoveEvent(End));
    Splitter->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, End, false));
}

static TSharedPtr<FTab> FindTab(const TSharedPtr<FTabStrip>& Strip, const String& PanelId)
{
    for (const TSharedPtr<FTab>& Tab : Strip->GetTabs())
    {
        if (Tab->GetPanelId() == PanelId)
        {
            return Tab;
        }
    }

    return nullptr;
}

static TSharedPtr<FTabStrip> FindStrip(const TSharedPtr<FDockingArea>& Area)
{
    return Area->FindPanelTabStrip(String());
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
        if (Command.Type == Type)
        {
            Count++;
        }
    }

    return Count;
}

static const FDrawCommand* FindCommand(const FDrawCommandList& CommandList, EDrawCommandType Type)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == Type)
        {
            return &Command;
        }
    }

    return nullptr;
}

static TArray<String> GetTabOrder(const TSharedPtr<FTabStrip>& Strip)
{
    TArray<String> Order;
    for (const TSharedPtr<FTab>& Tab : Strip->GetTabs())
    {
        Order.Add(Tab->GetPanelId());
    }

    return Order;
}

static bool AreNodesEqual(const FDockNode& Lhs, const FDockNode& Rhs)
{
    if (Lhs.Kind != Rhs.Kind || Lhs.TabIds != Rhs.TabIds || Lhs.Children.Size() != Rhs.Children.Size())
    {
        return false;
    }

    if (Lhs.Kind == EDockNodeKind::Tabs)
    {
        return Lhs.ActiveTabIndex == Rhs.ActiveTabIndex;
    }

    if (Lhs.Orientation != Rhs.Orientation || Lhs.ChildFractions.Size() != Rhs.ChildFractions.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Lhs.ChildFractions.Size(); ++Index)
    {
        if (Math::Abs(Lhs.ChildFractions[Index] - Rhs.ChildFractions[Index]) > 0.0001f)
        {
            return false;
        }
    }

    for (int32 Index = 0; Index < Lhs.Children.Size(); ++Index)
    {
        if (!AreNodesEqual(Lhs.Children[Index], Rhs.Children[Index]))
        {
            return false;
        }
    }

    return true;
}

static FStubPlatformWindow* GetStubWindow(const TSharedPtr<FWindow>& Window)
{
    return static_cast<FStubPlatformWindow*>(Window->GetPlatformWindow().Get());
}

struct FDockingFixture
{
    FDockingFixture(FScopedStubApplication& Application, const IntVector2& Size, const IntVector2& Position = IntVector2(0, 0))
        : Font(CreateFont())
        , Window(Application.CreateWindow(Size, Position))
        , Area(nullptr)
    {
        FDockingArea::FDesc Desc;
        Desc.Font = Font;

        Area = FDockingArea::Create(Desc);

        Area->RegisterPanel("Outliner", "Outliner", MakePanel("Outliner", Font));
        Area->RegisterPanel("Details", "Details", MakePanel("Details", Font));
        Area->RegisterPanel("Content", "Content Browser", MakePanel("Content", Font));

        Window->SetContent(Area);
    }

    void Layout()
    {
        FApplication::LayoutWindow(Window);
    }

    NODISCARD IntVector2 GetDropZoneCenter(const IntVector2& ClientHint, EDockDirection Direction) const
    {
        TArray<FDropZone> Zones;
        Area->GatherDropZones(ClientHint, Zones);

        for (const FDropZone& Zone : Zones)
        {
            if (Zone.Direction == Direction)
            {
                return Zone.Bounds.GetCenter() + Window->GetPosition();
            }
        }

        return IntVector2(-1, -1);
    }

    TSharedPtr<IFontFace>    Font;
    TSharedPtr<FWindow>      Window;
    TSharedPtr<FDockingArea> Area;
};

bool DockNodeMinimumSize_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A leaf is a panel with a strip over it");
    const FDockNode Leaf = FDockNode::CreateTabs({ "Outliner" });

    TEST_EXPECT_EQ(Leaf.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth);
    TEST_EXPECT_EQ(Leaf.ComputeMinimumSize().Y, FDockMetrics::MinimumPanelHeight + FDockMetrics::TabStripHeight);

    TEST_SECTION("A row adds its children's widths and the handle between them, and takes the taller one");
    const FDockNode Row = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Leaf, Leaf);

    TEST_EXPECT_EQ(Row.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth * 2 + FDockMetrics::SplitterThickness);
    TEST_EXPECT_EQ(Row.ComputeMinimumSize().Y, Leaf.ComputeMinimumSize().Y);

    TEST_SECTION("A column does the same the other way round");
    const FDockNode Column = FDockNode::CreateSplit(EDockSplitOrientation::Vertical, Leaf, Leaf);

    TEST_EXPECT_EQ(Column.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth);
    TEST_EXPECT_EQ(Column.ComputeMinimumSize().Y, Leaf.ComputeMinimumSize().Y * 2 + FDockMetrics::SplitterThickness);

    TEST_SECTION("Nesting accumulates, so an outer drag cannot squeeze an inner panel out of existence");
    const FDockNode Nested = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Leaf, Column);

    TEST_EXPECT_EQ(Nested.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth * 2 + FDockMetrics::SplitterThickness);
    TEST_EXPECT_EQ(Nested.ComputeMinimumSize().Y, Column.ComputeMinimumSize().Y);

    TEST_SECTION("A fresh split shares its space evenly");
    TEST_EXPECT_EQ(Row.ChildFractions.Size(), 2);
    TEST_EXPECT(Math::Abs(Row.ChildFractions[0] - 0.5f) < 0.0001f);

    TEST_END();
}

bool DockNodeCollapse_Test()
{
    TEST_BEGIN();

    const FDockNode Outliner = FDockNode::CreateTabs({ "Outliner" });
    const FDockNode Details  = FDockNode::CreateTabs({ "Details" });

    TEST_SECTION("An emptied leaf is dropped and the split of one that leaves becomes its other child");
    FDockNode Row = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Outliner, Details);
    Row.Children[0].TabIds.Clear();
    Row.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Row.Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Row.TabIds.Size(), 1);
    TEST_EXPECT_EQ(Row.TabIds[0], String("Details"));

    TEST_SECTION("A split whose child splits the same way absorbs it rather than nesting");
    FDockNode Inner = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Outliner, Details);
    FDockNode Outer = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Inner, FDockNode::CreateTabs({ "Content" }));

    Outer.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Outer.Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(Outer.Children.Size(), 3);
    TEST_EXPECT_EQ(Outer.Children[0].TabIds[0], String("Outliner"));
    TEST_EXPECT_EQ(Outer.Children[1].TabIds[0], String("Details"));
    TEST_EXPECT_EQ(Outer.Children[2].TabIds[0], String("Content"));

    TEST_SECTION("The absorbed children divide the share the child they came from had");
    TEST_EXPECT(Math::Abs(Outer.ChildFractions[0] - 0.25f) < 0.0001f);
    TEST_EXPECT(Math::Abs(Outer.ChildFractions[1] - 0.25f) < 0.0001f);
    TEST_EXPECT(Math::Abs(Outer.ChildFractions[2] - 0.50f) < 0.0001f);

    TEST_SECTION("A child splitting the other way is left alone, because it is a real nesting");
    FDockNode Mixed = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, FDockNode::CreateSplit(EDockSplitOrientation::Vertical, Outliner, Details), FDockNode::CreateTabs({ "Content" }));
    Mixed.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Mixed.Children.Size(), 2);
    TEST_EXPECT_EQ(Mixed.Children[0].Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(Mixed.Children[0].Orientation, EDockSplitOrientation::Vertical);

    TEST_SECTION("Emptying everything leaves an empty leaf rather than a split with no children");
    FDockNode Everything = FDockNode::CreateSplit(EDockSplitOrientation::Vertical, Outliner, Details);
    Everything.Children[0].TabIds.Clear();
    Everything.Children[1].TabIds.Clear();
    Everything.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Everything.Kind, EDockNodeKind::Tabs);
    TEST_EXPECT(Everything.IsEmpty());

    TEST_SECTION("A collapse that empties two levels does not leave a rung behind");
    FDockNode Deep = FDockNode::CreateSplit(
        EDockSplitOrientation::Horizontal,
        FDockNode::CreateSplit(EDockSplitOrientation::Vertical, Outliner, Details),
        FDockNode::CreateTabs({ "Content" }));

    Deep.Children[0].Children[0].TabIds.Clear();
    Deep.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Deep.Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(Deep.Children.Size(), 2);
    TEST_EXPECT_EQ(Deep.Children[0].Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Deep.Children[0].TabIds[0], String("Details"));

    TEST_SECTION("The fractions still add to one after all of that");
    float Total = 0.0f;
    for (float Fraction : Deep.ChildFractions)
    {
        Total += Fraction;
    }

    TEST_EXPECT(Math::Abs(Total - 1.0f) < 0.0001f);

    TEST_SECTION("An active index left past the end of a shortened tab list is pulled back");
    FDockNode Stack = FDockNode::CreateTabs({ "Outliner", "Details" });
    Stack.ActiveTabIndex = 1;
    Stack.TabIds.RemoveAt(1);
    Stack.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Stack.ActiveTabIndex, 0);

    TEST_END();
}

bool SplitterLayout_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FSplitter::FDesc Desc;
    Desc.Orientation     = EDockSplitOrientation::Horizontal;
    Desc.HandleThickness = 4;

    TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
    Splitter->AddChild(MakePanel("Left", Font), IntVector2(120, 60));
    Splitter->AddChild(MakePanel("Right", Font), IntVector2(120, 60));

    LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 404, 300));

    TArray<TSharedPtr<FVisualElement>> Children;
    Splitter->GetChildren(Children);

    TEST_SECTION("The handle comes out of the space before the children divide what is left");
    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 200);
    TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 200);

    TEST_SECTION("The children fill the height of a horizontal splitter");
    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Height, 300);

    TEST_SECTION("The handle sits in the gap between them");
    const FRectangle Handle = Splitter->GetHandleRectangle(0);
    TEST_EXPECT_EQ(Handle.Position.X, 200);
    TEST_EXPECT_EQ(Handle.Width, 4);
    TEST_EXPECT_EQ(Handle.Height, 300);

    TEST_SECTION("Points on the handle resolve to it and points on a child do not");
    TEST_EXPECT_EQ(Splitter->GetHandleIndexAt(IntVector2(202, 150)), 0);
    TEST_EXPECT_EQ(Splitter->GetHandleIndexAt(IntVector2(100, 150)), -1);
    TEST_EXPECT_EQ(Splitter->GetHandleIndexAt(IntVector2(300, 150)), -1);

    TEST_SECTION("A third child re-divides the space and adds a second handle");
    Splitter->AddChild(MakePanel("Far", Font), IntVector2(120, 60));
    LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 308, 300));

    Children.Clear();
    Splitter->GetChildren(Children);

    TEST_EXPECT_EQ(Children.Size(), 3);
    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 100);
    TEST_EXPECT_EQ(Children[2]->GetContentRectangle().GetRight(), 308);
    TEST_EXPECT_EQ(Splitter->GetHandleRectangle(1).Position.X, 204);

    TEST_SECTION("A vertical splitter divides the other axis and its handles run across");
    FSplitter::FDesc ColumnDesc;
    ColumnDesc.Orientation     = EDockSplitOrientation::Vertical;
    ColumnDesc.HandleThickness = 4;

    TSharedPtr<FSplitter> Column = FSplitter::Create(ColumnDesc);
    Column->AddChild(MakePanel("Top", Font), IntVector2(120, 60));
    Column->AddChild(MakePanel("Bottom", Font), IntVector2(120, 60));

    LayoutElement(Column, FRectangle(IntVector2(0, 0), 400, 204));

    Children.Clear();
    Column->GetChildren(Children);

    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Height, 100);
    TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Position.Y, 104);
    TEST_EXPECT_EQ(Column->GetHandleRectangle(0).Height, 4);
    TEST_EXPECT_EQ(Column->GetHandleRectangle(0).Width, 400);

    TEST_SECTION("The cursor changes only over a handle, and along the axis it moves");
    ECursor Cursor = ECursor::Arrow;
    TEST_EXPECT(!Column->GetCursor(Cursor));

    Column->OnMouseMove(MakeMoveEvent(Column->GetHandleRectangle(0).GetCenter()));
    TEST_EXPECT(Column->GetCursor(Cursor));
    TEST_EXPECT_EQ(Cursor, ECursor::ResizeNS);

    TEST_END();
}

bool SplitterSeededDesc_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font = CreateFont();

    TEST_SECTION("Shares seeded through the description survive every child arriving");
    {
        FSplitter::FDesc Desc;
        Desc.Orientation     = EDockSplitOrientation::Horizontal;
        Desc.HandleThickness = 4;
        Desc.Fractions.Add(0.25f);
        Desc.Fractions.Add(0.75f);

        TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
        Splitter->AddChild(MakePanel("Left", Font), IntVector2(120, 60));
        Splitter->AddChild(MakePanel("Right", Font), IntVector2(120, 60));

        LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 404, 300));

        TArray<TSharedPtr<FVisualElement>> Children;
        Splitter->GetChildren(Children);

        TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 100);
        TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 300);
    }

    TEST_SECTION("A seeded set that does not sum to one is normalized rather than refused");
    {
        FSplitter::FDesc Desc;
        Desc.Orientation     = EDockSplitOrientation::Horizontal;
        Desc.HandleThickness = 4;
        Desc.Fractions.Add(1.0f);
        Desc.Fractions.Add(3.0f);

        TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
        Splitter->AddChild(MakePanel("Left", Font), IntVector2(120, 60));
        Splitter->AddChild(MakePanel("Right", Font), IntVector2(120, 60));

        LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 404, 300));

        TEST_EXPECT_EQ(Splitter->GetFractions().Size(), 2);
        TEST_EXPECT(Math::Abs(Splitter->GetFractions()[0] - 0.25f) < 0.001f);
    }

    TEST_SECTION("A child arriving past the seeded shares falls back to sharing evenly");
    {
        FSplitter::FDesc Desc;
        Desc.Orientation     = EDockSplitOrientation::Horizontal;
        Desc.HandleThickness = 4;
        Desc.Fractions.Add(0.25f);
        Desc.Fractions.Add(0.75f);

        TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
        Splitter->AddChild(MakePanel("Left", Font), IntVector2(120, 60));
        Splitter->AddChild(MakePanel("Right", Font), IntVector2(120, 60));
        Splitter->AddChild(MakePanel("Far", Font), IntVector2(120, 60));

        TEST_EXPECT_EQ(Splitter->GetFractions().Size(), 3);
        TEST_EXPECT(Math::Abs(Splitter->GetFractions()[0] - (1.0f / 3.0f)) < 0.001f);
    }

    TEST_SECTION("A minimum seeded through the description wins over the one AddChild is given");
    {
        FSplitter::FDesc Desc;
        Desc.Orientation     = EDockSplitOrientation::Horizontal;
        Desc.HandleThickness = 4;
        Desc.MinimumSizes.Add(IntVector2(120, 60));
        Desc.MinimumSizes.Add(IntVector2(250, 60));

        TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
        Splitter->AddChild(MakePanel("Left", Font), IntVector2(10, 10));
        Splitter->AddChild(MakePanel("Right", Font), IntVector2(10, 10));

        LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 404, 300));

        TArray<TSharedPtr<FVisualElement>> Children;
        Splitter->GetChildren(Children);

        DragSplitterHandle(Splitter, 0, IntVector2(400, 0));
        TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 250);
    }

    TEST_SECTION("An unseeded description still shares evenly and takes the minimums AddChild is given");
    {
        FSplitter::FDesc Desc;
        Desc.Orientation     = EDockSplitOrientation::Horizontal;
        Desc.HandleThickness = 4;

        TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
        Splitter->AddChild(MakePanel("Left", Font), IntVector2(120, 60));
        Splitter->AddChild(MakePanel("Right", Font), IntVector2(120, 60));

        LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 404, 300));

        TArray<TSharedPtr<FVisualElement>> Children;
        Splitter->GetChildren(Children);

        TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 200);

        DragSplitterHandle(Splitter, 0, IntVector2(400, 0));
        TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 120);
    }

    TEST_END();
}

bool SplitterDrag_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font = CreateFont();

    int32         NumChangeCallbacks = 0;
    TArray<float> ReportedFractions;

    FSplitter::FDesc Desc;
    Desc.Orientation        = EDockSplitOrientation::Horizontal;
    Desc.HandleThickness    = 4;
    Desc.OnFractionsChanged = FOnSplitterFractionsChanged::CreateLambda([&](const TArray<float>& Fractions)
    {
        NumChangeCallbacks++;
        ReportedFractions = Fractions;
    });

    TSharedPtr<FSplitter> Splitter = FSplitter::Create(Desc);
    Splitter->AddChild(MakePanel("Left", Font), IntVector2(120, 60));
    Splitter->AddChild(MakePanel("Right", Font), IntVector2(120, 60));

    LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 404, 300));

    TArray<TSharedPtr<FVisualElement>> Children;
    Splitter->GetChildren(Children);

    TEST_SECTION("Dragging the handle right moves that many pixels into the leading child");
    DragSplitterHandle(Splitter, 0, IntVector2(50, 0));

    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 250);
    TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 150);

    TEST_SECTION("The shares that came out of it are reported once, when the drag settles");
    TEST_EXPECT_EQ(NumChangeCallbacks, 1);
    TEST_EXPECT_EQ(ReportedFractions.Size(), 2);
    TEST_EXPECT(Math::Abs(ReportedFractions[0] - 0.625f) < 0.001f);

    TEST_SECTION("A drag past the trailing minimum stops there rather than squeezing it out");
    DragSplitterHandle(Splitter, 0, IntVector2(400, 0));

    TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 120);
    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 280);

    TEST_SECTION("The same holds the other way");
    DragSplitterHandle(Splitter, 0, IntVector2(-400, 0));

    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 120);
    TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 280);

    TEST_SECTION("A drag that hits a minimum and comes back follows the cursor rather than lagging behind it");
    const IntVector2 Start = Splitter->GetHandleRectangle(0).GetCenter();

    Splitter->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Start, true));
    Splitter->OnMouseMove(MakeMoveEvent(Start + IntVector2(-400, 0)));
    Splitter->OnMouseMove(MakeMoveEvent(Start + IntVector2(60, 0)));
    Splitter->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Start + IntVector2(60, 0), false));

    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 180);

    TEST_SECTION("A middle handle leaves the far child where it was");
    Splitter->AddChild(MakePanel("Far", Font), IntVector2(120, 60));
    LayoutElement(Splitter, FRectangle(IntVector2(0, 0), 608, 300));

    Children.Clear();
    Splitter->GetChildren(Children);

    const int32 FarWidthBefore = Children[2]->GetContentRectangle().Width;
    DragSplitterHandle(Splitter, 0, IntVector2(40, 0));

    TEST_EXPECT_EQ(Children[2]->GetContentRectangle().Width, FarWidthBefore);
    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 240);
    TEST_EXPECT_EQ(Children[1]->GetContentRectangle().Width, 160);

    TEST_END();
}

bool TabStripReorder_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font = CreateFont();

    String ActivatedPanelId;
    String ClosedPanelId;
    String ReorderedPanelId;
    int32  ReorderedIndex = -1;

    FTabStrip::FDesc Desc;
    Desc.Font           = Font;
    Desc.OnTabActivated = FOnTabActivated::CreateLambda([&](const String& PanelId) { ActivatedPanelId = PanelId; });
    Desc.OnTabClosed    = FOnTabClosed::CreateLambda([&](const String& PanelId) { ClosedPanelId = PanelId; });
    Desc.OnTabReordered = FOnTabReordered::CreateLambda([&](const String& PanelId, int32 NewIndex)
    {
        ReorderedPanelId = PanelId;
        ReorderedIndex   = NewIndex;
    });

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(Desc);
    Strip->AddTab("Outliner", "Outliner", true);
    Strip->AddTab("Details", "Details", true);
    Strip->AddTab("Content", "Content", true);

    LayoutElement(Strip, FRectangle(IntVector2(0, 0), 600, FDockMetrics::TabStripHeight));

    TEST_SECTION("The first tab added is the one showing");
    TEST_EXPECT_EQ(Strip->GetActivePanelId(), String("Outliner"));
    TEST_EXPECT(FindTab(Strip, "Outliner")->IsActive());
    TEST_EXPECT(!FindTab(Strip, "Details")->IsActive());

    TEST_SECTION("The tabs are laid out left to right at the style's spacing, each as wide as its label needs");
    const int32 TabSpacing = FUIStyle::GetDefault().Tab.Spacing;

    TEST_EXPECT_EQ(FindTab(Strip, "Outliner")->GetContentRectangle().Position.X, TabSpacing);
    TEST_EXPECT_EQ(FindTab(Strip, "Details")->GetContentRectangle().Position.X,
        FindTab(Strip, "Outliner")->GetContentRectangle().GetRight() + TabSpacing);

    TEST_SECTION("A tab floats inside the strip, held off the top and the bottom by the style's insets");
    const FUITabStyle& TabStyle = FUIStyle::GetDefault().Tab;

    TEST_EXPECT_EQ(FDockMetrics::TabStripHeight, TabStyle.StripHeight);
    TEST_EXPECT_EQ(FindTab(Strip, "Outliner")->GetContentRectangle().Position.Y, TabStyle.TopInset);
    TEST_EXPECT_EQ(FindTab(Strip, "Outliner")->GetContentRectangle().Height,
        FDockMetrics::TabStripHeight - TabStyle.TopInset - TabStyle.BottomInset);

    TEST_SECTION("Its close button is held off the trailing edge by the close inset, not by the label's padding");
    const FRectangle OutlinerBounds = FindTab(Strip, "Outliner")->GetContentRectangle();
    const FRectangle OutlinerClose  = FindTab(Strip, "Outliner")->GetCloseButtonRectangle();

    TEST_EXPECT_EQ(OutlinerBounds.GetRight() - OutlinerClose.GetRight(), TabStyle.CloseInset);
    TEST_EXPECT_EQ(OutlinerClose.Width, TabStyle.CloseSize);
    TEST_EXPECT(OutlinerClose.Position.X > OutlinerBounds.Position.X + TabStyle.HorizontalPadding);

    TEST_SECTION("Its label is centred in the pill and then lifted by the style's nudge, so it reads level with the cross");
    FDrawCommandList LabelCommands;
    DrawElement(FindTab(Strip, "Outliner"), LabelCommands);

    const FDrawCommand* Label = FindCommand(LabelCommands, EDrawCommandType::Text);
    TEST_EXPECT(Label != nullptr);

    if (Label)
    {
        TEST_EXPECT_EQ(Label->Bounds.Position.Y,
            OutlinerBounds.Position.Y + ((OutlinerBounds.Height - Font->GetLineHeight()) / 2) + TabStyle.LabelOffsetY);
        TEST_EXPECT(Label->Bounds.GetCenter().Y < OutlinerClose.GetCenter().Y);
    }

    TEST_SECTION("A hovered tab puts the hand under the cursor, over its close button as much as over its label");
    const TSharedPtr<FTab> HoveredTab = FindTab(Strip, "Outliner");

    ECursor Cursor = ECursor::Arrow;
    TEST_EXPECT(!HoveredTab->GetCursor(Cursor));

    HoveredTab->OnMouseEntered(MakeMoveEvent(OutlinerBounds.GetCenter()));
    TEST_EXPECT(HoveredTab->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::Hand);

    HoveredTab->OnMouseMove(MakeMoveEvent(OutlinerClose.GetCenter()));
    TEST_EXPECT(HoveredTab->GetCursor(Cursor));

    TEST_SECTION("The strip behind them has no opinion, so the gap between two tabs keeps the arrow");
    HoveredTab->OnMouseLeft(MakeMoveEvent(IntVector2(900, 900)));

    TEST_EXPECT(!HoveredTab->GetCursor(Cursor));
    TEST_EXPECT(!Strip->GetCursor(Cursor));

    TEST_SECTION("Pressing a tab shows it, so a drag starts from the panel it is about to move");
    const TSharedPtr<FTab> DetailsTab = FindTab(Strip, "Details");
    DetailsTab->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, DetailsTab->GetContentRectangle().GetCenter(), true));

    TEST_EXPECT_EQ(Strip->GetActivePanelId(), String("Details"));
    TEST_EXPECT_EQ(ActivatedPanelId, String("Details"));
    TEST_EXPECT_EQ(Strip->GetDraggedPanelId(), String("Details"));

    TEST_SECTION("Dragging it over the first tab moves it there and reports the new index");
    const IntVector2 OverFirst(FindTab(Strip, "Outliner")->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(DetailsTab.Get(), OverFirst, OverFirst);

    TEST_EXPECT_EQ(GetTabOrder(Strip)[0], String("Details"));
    TEST_EXPECT_EQ(GetTabOrder(Strip)[1], String("Outliner"));
    TEST_EXPECT_EQ(ReorderedPanelId, String("Details"));
    TEST_EXPECT_EQ(ReorderedIndex, 0);

    TEST_SECTION("The moved tab is arranged where it now sits rather than where it was");
    TEST_EXPECT_EQ(DetailsTab->GetContentRectangle().Position.X, TabSpacing);

    TEST_SECTION("Letting go ends the drag");
    DetailsTab->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, DetailsTab->GetContentRectangle().GetCenter(), false));
    TEST_EXPECT(Strip->GetDraggedPanelId().IsEmpty());

    TEST_SECTION("A drag past the far end takes the tab to the far end");
    const TSharedPtr<FTab> ContentTab = FindTab(Strip, "Content");
    Strip->OnTabPressed(ContentTab.Get(), ContentTab->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(ContentTab.Get(), IntVector2(-40, 10), IntVector2(-40, 10));

    TEST_EXPECT_EQ(GetTabOrder(Strip)[0], String("Content"));

    Strip->OnTabReleased(ContentTab.Get(), ContentTab->GetContentRectangle().GetCenter(), ContentTab->GetContentRectangle().GetCenter());

    TEST_SECTION("Clicking the cross closes the panel, and clicking the label does not");
    const TSharedPtr<FTab> OutlinerTab = FindTab(Strip, "Outliner");

    const IntVector2 LabelPoint = OutlinerTab->GetContentRectangle().Position + IntVector2(4, 4);
    const IntVector2 CrossPoint = OutlinerTab->GetCloseButtonRectangle().GetCenter();

    Strip->OnTabPressed(OutlinerTab.Get(), LabelPoint);
    Strip->OnTabReleased(OutlinerTab.Get(), LabelPoint, LabelPoint);
    TEST_EXPECT(ClosedPanelId.IsEmpty());

    TEST_SECTION("Pressing the cross and letting go elsewhere does not close it either");
    Strip->OnTabPressed(OutlinerTab.Get(), CrossPoint);
    Strip->OnTabReleased(OutlinerTab.Get(), LabelPoint, LabelPoint);
    TEST_EXPECT(ClosedPanelId.IsEmpty());

    Strip->OnTabPressed(OutlinerTab.Get(), CrossPoint);
    Strip->OnTabReleased(OutlinerTab.Get(), CrossPoint, CrossPoint);
    TEST_EXPECT_EQ(ClosedPanelId, String("Outliner"));

    TEST_SECTION("Removing the active tab hands the panel to its neighbour");
    Strip->SetActiveTab("Details");
    Strip->RemoveTab("Details");

    TEST_EXPECT_EQ(Strip->GetTabs().Size(), 2);
    TEST_EXPECT(!Strip->GetActivePanelId().IsEmpty());
    TEST_EXPECT(FindTab(Strip, Strip->GetActivePanelId())->IsActive());

    TEST_SECTION("A strip with reordering turned off keeps its order");
    FTabStrip::FDesc FixedDesc;
    FixedDesc.Font          = Font;
    FixedDesc.bAllowReorder = false;

    TSharedPtr<FTabStrip> Fixed = FTabStrip::Create(FixedDesc);
    Fixed->AddTab("First", "First", false);
    Fixed->AddTab("Second", "Second", false);

    LayoutElement(Fixed, FRectangle(IntVector2(0, 0), 600, FDockMetrics::TabStripHeight));

    const TSharedPtr<FTab> SecondTab = FindTab(Fixed, "Second");
    Fixed->OnTabPressed(SecondTab.Get(), SecondTab->GetContentRectangle().GetCenter());
    Fixed->OnTabDragged(SecondTab.Get(), IntVector2(2, 10), IntVector2(2, 10));

    TEST_EXPECT_EQ(GetTabOrder(Fixed)[0], String("First"));

    TEST_END();
}

bool TabStripScroll_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FUIStyle::ResetDefault();

    const TSharedPtr<IFontFace> Font     = CreateFont();
    const FUITabStyle&          TabStyle = FUIStyle::GetDefault().Tab;

    FTabStrip::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(Desc);
    for (int32 Index = 0; Index < 12; ++Index)
    {
        const String PanelId = String::Printf("Panel%d", Index);
        Strip->AddTab(PanelId, PanelId, true);
    }

    TEST_SECTION("A strip wide enough for every tab has nothing to scroll");
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), 4000, FDockMetrics::TabStripHeight));

    TEST_EXPECT_EQ(Strip->GetMaxScrollOffset(), 0);
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), 0);

    TEST_SECTION("The desired width budgets the gap before the first tab as well as the one after each");
    const int32 ContentWidth = Strip->ComputeDesiredSize().X;

    int32 ExpectedWidth = TabStyle.Spacing;
    for (const TSharedPtr<FTab>& Tab : Strip->GetTabs())
    {
        ExpectedWidth += Tab->GetCachedDesiredSize().X + TabStyle.Spacing;
    }

    TEST_EXPECT_EQ(ContentWidth, ExpectedWidth);
    TEST_EXPECT_EQ(Strip->GetTabs().Last()->GetContentRectangle().GetRight() + TabStyle.Spacing, ContentWidth);

    TEST_SECTION("Narrowing it past the content leaves exactly the overflow to scroll through");
    const int32 ViewWidth = 300;
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), ViewWidth, FDockMetrics::TabStripHeight));

    TEST_EXPECT_EQ(Strip->GetMaxScrollOffset(), ContentWidth - ViewWidth);

    TEST_SECTION("Scrolling shifts every tab by the offset, leading gap and all");
    const int32 FirstTabStart = Strip->GetTabs()[0]->GetContentRectangle().Position.X;

    Strip->SetScrollOffset(50);
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), ViewWidth, FDockMetrics::TabStripHeight));

    TEST_EXPECT_EQ(Strip->GetScrollOffset(), 50);
    TEST_EXPECT_EQ(Strip->GetTabs()[0]->GetContentRectangle().Position.X, FirstTabStart - 50);

    TEST_SECTION("An offset past either end is clamped rather than running the tabs off the strip");
    Strip->SetScrollOffset(-100);
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), 0);

    Strip->SetScrollOffset(100000);
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), Strip->GetMaxScrollOffset());

    TEST_SECTION("The wheel scrolls sideways on either axis, since the strip has only the one to give");
    Strip->SetScrollOffset(200);

    TEST_EXPECT(Strip->OnMouseScroll(MakeScrollEvent(1.0f)).IsEventHandled());
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), 200 - FTabStrip::DefaultScrollAmountPerWheelStep);

    TEST_EXPECT(Strip->OnMouseScroll(MakeScrollEvent(-1.0f, EScrollAxis::Horizontal)).IsEventHandled());
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), 200);

    TEST_SECTION("Activating a tab off the leading end pulls it flush against that edge");
    Strip->SetScrollOffset(Strip->GetMaxScrollOffset());
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), ViewWidth, FDockMetrics::TabStripHeight));

    Strip->SetActiveTab("Panel0");
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), ViewWidth, FDockMetrics::TabStripHeight));

    TEST_EXPECT_EQ(Strip->GetScrollOffset(), TabStyle.Spacing);
    TEST_EXPECT_EQ(Strip->GetTabs()[0]->GetContentRectangle().Position.X, 0);

    TEST_SECTION("One off the trailing end comes just far enough to show, rather than all the way to the stop");
    Strip->SetActiveTab("Panel11");
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), ViewWidth, FDockMetrics::TabStripHeight));

    TEST_EXPECT_EQ(Strip->GetTabs().Last()->GetContentRectangle().GetRight(), ViewWidth);
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), Strip->GetMaxScrollOffset() - TabStyle.Spacing);

    TEST_SECTION("A tab already in view is left where it is");
    const int32 SettledOffset = Strip->GetScrollOffset();

    Strip->SetActiveTab("Panel11");
    TEST_EXPECT_EQ(Strip->GetScrollOffset(), SettledOffset);

    TEST_SECTION("The strip clips itself, so a tab scrolled past the end stops painting outside it");
    FDrawCommandList ScrolledCommands;
    DrawElement(Strip, ScrolledCommands);

    TEST_EXPECT_EQ(CountCommands(ScrolledCommands, EDrawCommandType::ClipPush), 1);
    TEST_EXPECT_EQ(CountCommands(ScrolledCommands, EDrawCommandType::ClipPop), 1);
    TEST_EXPECT_EQ(FindCommand(ScrolledCommands, EDrawCommandType::ClipPush)->Bounds, Strip->GetContentRectangle());

    TEST_SECTION("It stops answering for a point out there too, so the tab cannot be clicked through the panel below");
    FElementPath OutsidePath;
    Strip->FindChildrenContainingPoint(IntVector2(ViewWidth + 40, TabStyle.TopInset + 4), OutsidePath);
    TEST_EXPECT(OutsidePath.GetElements().IsEmpty());

    FElementPath InsidePath;
    Strip->FindChildrenContainingPoint(Strip->GetTabs().Last()->GetContentRectangle().GetCenter(), InsidePath);
    TEST_EXPECT(!InsidePath.GetElements().IsEmpty());

    TEST_SECTION("The bar it scrolls with lies along the bottom of the strip, thin enough to leave the pills alone");
    const TSharedPtr<FScrollBar>& ScrollBar = Strip->GetScrollBar();
    TEST_EXPECT(ScrollBar != nullptr);
    TEST_EXPECT(ScrollBar->GetOrientation() == EOrientation::Horizontal);

    const FRectangle BarBounds = ScrollBar->GetContentRectangle();
    TEST_EXPECT_EQ(BarBounds.Height, TabStyle.ScrollBarThickness);
    TEST_EXPECT_EQ(BarBounds.GetBottom(), Strip->GetContentRectangle().GetBottom());
    TEST_EXPECT(ScrollBar->IsScrollable());

    TEST_SECTION("Its track is cleared away, so what fades in over the strip is the thumb on its own");
    TEST_EXPECT_EQ(ScrollBar->GetStyle().Track.A, 0.0f);
    TEST_EXPECT(ScrollBar->GetStyle().Grab.A > 0.0f);

    TEST_SECTION("It is invisible until the cursor arrives, and dragging it scrolls the tabs");
    TEST_EXPECT_EQ(ScrollBar->GetOpacity(), 0.0f);

    ScrollBar->SetOffset(0);
    ScrollBar->SetOpacity(1.0f);
    ScrollBar->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(BarBounds.GetRight() - 1, BarBounds.Position.Y), true));

    TEST_EXPECT_EQ(Strip->GetScrollOffset(), Strip->GetMaxScrollOffset());

    TEST_SECTION("An opacity outside zero to one is clamped rather than blowing the alpha out");
    ScrollBar->SetOpacity(4.0f);
    TEST_EXPECT_EQ(ScrollBar->GetOpacity(), 1.0f);

    ScrollBar->SetOpacity(-1.0f);
    TEST_EXPECT_EQ(ScrollBar->GetOpacity(), 0.0f);

    TEST_SECTION("A strip everything fits in hides the bar and hands the wheel back to whatever is behind it");
    LayoutElement(Strip, FRectangle(IntVector2(0, 0), 4000, FDockMetrics::TabStripHeight));

    TEST_EXPECT_EQ(Strip->GetScrollOffset(), 0);
    TEST_EXPECT(!Strip->OnMouseScroll(MakeScrollEvent(1.0f)).IsEventHandled());

    FDrawCommandList FittedCommands;
    DrawElement(Strip, FittedCommands);
    TEST_EXPECT_EQ(CountCommands(FittedCommands, EDrawCommandType::ClipPush), 1);

    TEST_END();
}

bool TabStripStyle_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FUIStyle::ResetDefault();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FUITabStyle TabStyle;
    TabStyle.Fill               = FFloatColor(0.90f, 0.10f, 0.10f, 1.0f);
    TabStyle.FillActive         = FFloatColor(0.10f, 0.90f, 0.10f, 1.0f);
    TabStyle.StripFill          = FFloatColor(0.10f, 0.10f, 0.90f, 1.0f);
    TabStyle.Separator          = FFloatColor(0.90f, 0.90f, 0.10f, 1.0f);
    TabStyle.Spacing            = 6;
    TabStyle.TopInset           = 5;
    TabStyle.BottomInset        = 3;
    TabStyle.CornerRadius       = 9.0f;
    TabStyle.StripHeight        = 44;
    TabStyle.HorizontalPadding  = 20;
    TabStyle.SeparatorThickness = 3;
    TabStyle.CloseInset         = 7;
    TabStyle.MinWidth           = 0;

    FTabStrip::FDesc Desc;
    Desc.Font  = Font;
    Desc.Style = TabStyle;

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(Desc);
    Strip->AddTab("Outliner", "Outliner", true);
    Strip->AddTab("Details", "Details", true);

    Strip->PrepareDesiredSize();

    TEST_SECTION("The strip asks for the height the description named rather than the shipped one");
    TEST_EXPECT_EQ(Strip->GetCachedDesiredSize().Y, TabStyle.StripHeight);
    TEST_EXPECT(TabStyle.StripHeight != FUIStyle::GetDefault().Tab.StripHeight);

    Strip->Tick(FRectangle(IntVector2(0, 0), 600, TabStyle.StripHeight));

    TEST_SECTION("Its tabs take that description's spacing, inset and padding too");
    const TSharedPtr<FTab> OutlinerTab = FindTab(Strip, "Outliner");
    const TSharedPtr<FTab> DetailsTab  = FindTab(Strip, "Details");

    TEST_EXPECT_EQ(OutlinerTab->GetContentRectangle().Position.X, TabStyle.Spacing);
    TEST_EXPECT_EQ(OutlinerTab->GetContentRectangle().Position.Y, TabStyle.TopInset);
    TEST_EXPECT_EQ(OutlinerTab->GetContentRectangle().Height, TabStyle.StripHeight - TabStyle.TopInset - TabStyle.BottomInset);
    TEST_EXPECT_EQ(DetailsTab->GetContentRectangle().Position.X, OutlinerTab->GetContentRectangle().GetRight() + TabStyle.Spacing);

    TEST_SECTION("With the floor turned off, a closable tab pays the padding on its leading edge and the close inset on its trailing one");
    const int32 LabelWidth = Font->MeasureWidth(StringView("Outliner", 8));
    TEST_EXPECT_EQ(OutlinerTab->GetContentRectangle().Width,
        TabStyle.HorizontalPadding + LabelWidth + TabStyle.LabelCloseGap + TabStyle.CloseSize + TabStyle.CloseInset);

    TEST_EXPECT_EQ(OutlinerTab->GetContentRectangle().GetRight() - OutlinerTab->GetCloseButtonRectangle().GetRight(),
        TabStyle.CloseInset);

    TEST_SECTION("The strip fills itself in the description's colour");
    FDrawCommandList StripCommands;
    DrawElement(Strip, StripCommands);

    TEST_EXPECT(!StripCommands.GetCommands().IsEmpty());
    TEST_EXPECT_EQ(StripCommands.GetCommands()[0].Type, EDrawCommandType::Box);
    TEST_EXPECT(StripCommands.GetCommands()[0].Tint == TabStyle.StripFill);

    TEST_SECTION("A resting tab paints no fill at all, so the rule the description turned back on comes first");
    FDrawCommandList InactiveCommands;
    DrawElement(DetailsTab, InactiveCommands);

    TEST_EXPECT(!DetailsTab->IsActive());

    for (const FDrawCommand& Command : InactiveCommands.GetCommands())
    {
        TEST_EXPECT(!(Command.Tint == TabStyle.Fill));
    }

    const FDrawCommand& Rule = InactiveCommands.GetCommands()[0];
    TEST_EXPECT_EQ(Rule.Type, EDrawCommandType::Box);
    TEST_EXPECT(Rule.Tint == TabStyle.Separator);
    TEST_EXPECT_EQ(Rule.Bounds.Width, TabStyle.SeparatorThickness);
    TEST_EXPECT_EQ(Rule.Bounds.GetRight(), DetailsTab->GetContentRectangle().GetRight());

    TEST_SECTION("The active one takes the active fill from the same description, rounded on every corner");
    TEST_EXPECT(OutlinerTab->IsActive());

    FDrawCommandList ActiveCommands;
    DrawElement(OutlinerTab, ActiveCommands);

    const FDrawCommand& Pill = ActiveCommands.GetCommands()[0];
    TEST_EXPECT(Pill.Tint == TabStyle.FillActive);
    TEST_EXPECT_EQ(Pill.CornerRadius.TopLeft, TabStyle.CornerRadius);
    TEST_EXPECT_EQ(Pill.CornerRadius.BottomRight, TabStyle.CornerRadius);

    TEST_SECTION("Its accent is a band cut from that same pill, so it carries the pill's bounds and radius rather than its own");
    const FDrawCommand* Accent = FindCommand(ActiveCommands, EDrawCommandType::RoundedBottomBar);
    TEST_EXPECT(Accent != nullptr);

    if (Accent)
    {
        TEST_EXPECT(Accent->Tint == TabStyle.ActiveStrip);
        TEST_EXPECT_EQ(Accent->Bounds, OutlinerTab->GetContentRectangle());
        TEST_EXPECT_EQ(Accent->CornerRadius.BottomLeft, TabStyle.CornerRadius);
        TEST_EXPECT_EQ(Accent->Thickness, static_cast<float>(TabStyle.ActiveStripThickness));
        TEST_EXPECT_EQ(Accent->FadeWidth, TabStyle.ActiveStripFadeWidth);
    }

    TEST_SECTION("A resting tab has no accent at all");
    TEST_EXPECT_EQ(CountCommands(InactiveCommands, EDrawCommandType::RoundedBottomBar), 0);

    TEST_SECTION("With no close brush to hand, the cross is drawn as two strokes");
    TEST_EXPECT_EQ(CountCommands(ActiveCommands, EDrawCommandType::Polyline), 2);
    TEST_EXPECT_EQ(CountCommands(ActiveCommands, EDrawCommandType::Image), 0);

    TEST_SECTION("A strip given one draws the glyph instead, centred in the button at the style's size");
    FTabStrip::FDesc IconDesc;
    IconDesc.Font      = Font;
    IconDesc.Style     = TabStyle;
    IconDesc.CloseIcon = FUIBrush(reinterpret_cast<FRHITexture*>(0x10));

    TSharedPtr<FTabStrip> Iconed = FTabStrip::Create(IconDesc);
    Iconed->AddTab("Outliner", "Outliner", true);

    LayoutElement(Iconed, FRectangle(IntVector2(0, 0), 600, TabStyle.StripHeight));

    const TSharedPtr<FTab> IconedTab = FindTab(Iconed, "Outliner");

    FDrawCommandList IconCommands;
    DrawElement(IconedTab, IconCommands);

    TEST_EXPECT_EQ(CountCommands(IconCommands, EDrawCommandType::Polyline), 0);
    TEST_EXPECT_EQ(CountCommands(IconCommands, EDrawCommandType::Image), 1);

    const FDrawCommand& Glyph = IconCommands.GetCommands().Last();
    TEST_EXPECT_EQ(Glyph.Type, EDrawCommandType::Image);
    TEST_EXPECT_EQ(Glyph.Bounds.Width, TabStyle.CloseIconSize);
    TEST_EXPECT_EQ(Glyph.Bounds.Height, TabStyle.CloseIconSize);
    TEST_EXPECT_EQ(Glyph.Bounds.GetCenter(), IconedTab->GetCloseButtonRectangle().GetCenter());

    TEST_SECTION("None of it touched the process-wide style, so the next strip is the shipped one");
    TEST_EXPECT(FUIStyle::GetDefault().Tab.Fill == FUITabStyle().Fill);
    TEST_EXPECT(FUIStyle::GetDefault().Tab.StripFill == FUITabStyle().StripFill);

    FTabStrip::FDesc PlainDesc;
    PlainDesc.Font = Font;

    TSharedPtr<FTabStrip> Plain = FTabStrip::Create(PlainDesc);
    Plain->AddTab("Plain", "Plain", true);
    Plain->PrepareDesiredSize();

    TEST_EXPECT_EQ(Plain->GetCachedDesiredSize().Y, FUIStyle::GetDefault().Tab.StripHeight);

    TEST_END();
}

bool TabMinimumWidth_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FUIStyle::ResetDefault();

    const TSharedPtr<IFontFace> Font     = CreateFont();
    const FUITabStyle&          TabStyle = FUIStyle::GetDefault().Tab;

    FTabStrip::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(Desc);
    Strip->AddTab("About", "About", true);
    Strip->AddTab("Content Browser", "Content Browser", true);

    LayoutElement(Strip, FRectangle(IntVector2(0, 0), 900, FDockMetrics::TabStripHeight));

    TEST_SECTION("A short title is padded out to the style's floor rather than shrinking to its label");
    TEST_EXPECT_EQ(FindTab(Strip, "About")->GetCachedDesiredSize().X, TabStyle.MinWidth);

    TEST_SECTION("A title the floor cannot hold still takes the room it measures");
    TEST_EXPECT(FindTab(Strip, "Content Browser")->GetCachedDesiredSize().X > TabStyle.MinWidth);

    TEST_SECTION("The floor is cut to the editor's own titles, so a middling one reaches it and the longest runs past");
    TSharedPtr<FTrueTypeFontFace> Body = FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/segoeui.ttf", 20);
    TEST_EXPECT(Body != nullptr);

    if (Body)
    {
        const int32 Chrome           = TabStyle.HorizontalPadding + TabStyle.LabelCloseGap + TabStyle.CloseSize + TabStyle.CloseInset;
        const int32 FrameProfiler    = Chrome + Body->MeasureWidth(StringView("Frame Profiler", 14));
        const int32 RendererSettings = Chrome + Body->MeasureWidth(StringView("Renderer Settings", 17));

        TEST_EXPECT(FrameProfiler <= TabStyle.MinWidth);
        TEST_EXPECT(RendererSettings > TabStyle.MinWidth);
    }

    TEST_END();
}

bool TabStripTearOut_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font = CreateFont();

    String     DetachedPanelId;
    IntVector2 DetachedPosition;
    IntVector2 LastMovePosition;
    int32      NumDetachCallbacks   = 0;
    int32      NumMoveCallbacks     = 0;
    int32      NumFinishedCallbacks = 0;

    FTabStrip::FDesc Desc;
    Desc.Font              = Font;
    Desc.OnTabDragDetached = FOnTabDragDetached::CreateLambda([&](const String& PanelId, const IntVector2&, const IntVector2& ScreenPosition)
    {
        DetachedPanelId  = PanelId;
        DetachedPosition = ScreenPosition;
        NumDetachCallbacks++;
    });

    Desc.OnTabDragMoved = FOnTabDragMoved::CreateLambda([&](const String&, const IntVector2&, const IntVector2& ScreenPosition)
    {
        LastMovePosition = ScreenPosition;
        NumMoveCallbacks++;
    });

    Desc.OnTabDragFinished = FOnTabDragFinished::CreateLambda([&](const String&, const IntVector2&, const IntVector2&) { NumFinishedCallbacks++; });

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(Desc);
    Strip->AddTab("Outliner", "Outliner", true);
    Strip->AddTab("Details", "Details", true);

    LayoutElement(Strip, FRectangle(IntVector2(0, 30), 600, FDockMetrics::TabStripHeight));

    const TSharedPtr<FTab> DetailsTab = FindTab(Strip, "Details");
    Strip->OnTabPressed(DetailsTab.Get(), DetailsTab->GetContentRectangle().GetCenter());

    TEST_SECTION("A drag inside the strip is a reorder, however far it goes sideways");
    Strip->OnTabDragged(DetailsTab.Get(), IntVector2(4, 40), IntVector2(4, 40));
    TEST_EXPECT_EQ(NumDetachCallbacks, 0);

    TEST_SECTION("A drag just off the strip is not enough either, so a shaky hand does not tear a panel out");
    const IntVector2 JustOff(200, 30 + FDockMetrics::TabStripHeight + FTabStrip::TearOutDistance - 1);
    Strip->OnTabDragged(DetailsTab.Get(), JustOff, JustOff);
    TEST_EXPECT_EQ(NumDetachCallbacks, 0);

    TEST_SECTION("Past the threshold the panel comes out, once");
    const IntVector2 FarBelow(200, 30 + FDockMetrics::TabStripHeight + FTabStrip::TearOutDistance + 10);

    Strip->OnTabDragged(DetailsTab.Get(), FarBelow, FarBelow);
    Strip->OnTabDragged(DetailsTab.Get(), FarBelow + IntVector2(0, 20), FarBelow + IntVector2(0, 20));

    TEST_EXPECT_EQ(NumDetachCallbacks, 1);
    TEST_EXPECT_EQ(DetachedPanelId, String("Details"));
    TEST_EXPECT_EQ(DetachedPosition.Y, FarBelow.Y);

    TEST_SECTION("The strip stops reordering it, but keeps reporting where it goes");
    TEST_EXPECT(Strip->GetDraggedPanelId().IsEmpty());
    TEST_EXPECT_EQ(Strip->GetDetachedPanelId(), String("Details"));
    TEST_EXPECT_EQ(NumMoveCallbacks, 1);
    TEST_EXPECT_EQ(LastMovePosition.Y, FarBelow.Y + 20);
    TEST_EXPECT_EQ(GetTabOrder(Strip)[1], String("Details"));

    TEST_SECTION("Letting go says so once, and the strip is done with it");
    Strip->OnTabReleased(DetailsTab.Get(), LastMovePosition, LastMovePosition);

    TEST_EXPECT_EQ(NumFinishedCallbacks, 1);
    TEST_EXPECT(Strip->GetDetachedPanelId().IsEmpty());

    TEST_SECTION("Dragging above the strip tears out too");
    const IntVector2 FarAbove(200, 30 - FTabStrip::TearOutDistance - 10);
    Strip->OnTabPressed(DetailsTab.Get(), DetailsTab->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(DetailsTab.Get(), FarAbove, FarAbove);

    TEST_EXPECT_EQ(NumDetachCallbacks, 2);

    TEST_SECTION("A strip with tear-out turned off reorders instead");
    FTabStrip::FDesc FixedDesc;
    FixedDesc.Font              = Font;
    FixedDesc.bAllowTearOut     = false;
    FixedDesc.OnTabDragDetached = FOnTabDragDetached::CreateLambda([&](const String&, const IntVector2&, const IntVector2&) { NumDetachCallbacks++; });

    TSharedPtr<FTabStrip> Fixed = FTabStrip::Create(FixedDesc);
    Fixed->AddTab("First", "First", false);
    Fixed->AddTab("Second", "Second", false);

    LayoutElement(Fixed, FRectangle(IntVector2(0, 0), 600, FDockMetrics::TabStripHeight));

    const TSharedPtr<FTab> SecondTab = FindTab(Fixed, "Second");
    Fixed->OnTabPressed(SecondTab.Get(), SecondTab->GetContentRectangle().GetCenter());
    Fixed->OnTabDragged(SecondTab.Get(), IntVector2(4, 400), IntVector2(4, 400));

    TEST_EXPECT_EQ(NumDetachCallbacks, 2);
    TEST_EXPECT_EQ(GetTabOrder(Fixed)[0], String("Second"));

    TEST_END();
}

bool DockingAreaDockUndock_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FDockingFixture        Fixture(Application, IntVector2(800, 600));

    TEST_SECTION("An area starts empty, and the first panel docked becomes the whole tree");
    TEST_EXPECT(Fixture.Area->GetDockedPanelIds().IsEmpty());

    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().Kind, EDockNodeKind::Tabs);
    TEST_EXPECT(Fixture.Area->IsPanelDocked("Outliner"));

    TEST_SECTION("Docking to the right of it splits horizontally, with the new panel second");
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Right);

    const FDockNode& AfterRight = Fixture.Area->SaveLayout();
    TEST_EXPECT_EQ(AfterRight.Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(AfterRight.Orientation, EDockSplitOrientation::Horizontal);
    TEST_EXPECT_EQ(AfterRight.Children.Size(), 2);
    TEST_EXPECT_EQ(AfterRight.Children[0].TabIds[0], String("Outliner"));
    TEST_EXPECT_EQ(AfterRight.Children[1].TabIds[0], String("Details"));

    TEST_SECTION("Docking below one of them nests a column inside the row");
    Fixture.Area->DockPanel("Content", "Details", EDockDirection::Bottom);

    const FDockNode& AfterBottom = Fixture.Area->SaveLayout();
    TEST_EXPECT_EQ(AfterBottom.Children.Size(), 2);
    TEST_EXPECT_EQ(AfterBottom.Children[1].Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(AfterBottom.Children[1].Orientation, EDockSplitOrientation::Vertical);
    TEST_EXPECT_EQ(AfterBottom.Children[1].Children[1].TabIds[0], String("Content"));

    TEST_SECTION("Every docked panel is still accounted for, in layout order");
    const TArray<String> Docked = Fixture.Area->GetDockedPanelIds();
    TEST_EXPECT_EQ(Docked.Size(), 3);
    TEST_EXPECT_EQ(Docked[0], String("Outliner"));
    TEST_EXPECT_EQ(Docked[1], String("Details"));
    TEST_EXPECT_EQ(Docked[2], String("Content"));

    TEST_SECTION("Docking into the center of a panel stacks it as a tab instead of splitting");
    Fixture.Area->DockPanel("Content", "Outliner", EDockDirection::Center);

    const FDockNode& AfterCenter = Fixture.Area->SaveLayout();
    TEST_EXPECT_EQ(AfterCenter.Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(AfterCenter.Children[0].TabIds.Size(), 2);
    TEST_EXPECT_EQ(AfterCenter.Children[0].TabIds[1], String("Content"));
    TEST_EXPECT_EQ(AfterCenter.Children[0].ActiveTabIndex, 1);

    TEST_SECTION("Moving a panel does not leave a copy where it was");
    TEST_EXPECT_EQ(Fixture.Area->GetDockedPanelIds().Size(), 3);
    TEST_EXPECT_EQ(AfterCenter.Children[1].TabIds.Size(), 1);
    TEST_EXPECT_EQ(AfterCenter.Children[1].TabIds[0], String("Details"));

    TEST_SECTION("Undocking a stacked panel leaves the stack, and the split alone");
    Fixture.Area->UndockPanel("Content");

    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Content"));
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().Children.Size(), 2);

    TEST_SECTION("Undocking the last panel of a leaf collapses the split it was in");
    Fixture.Area->UndockPanel("Details");

    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds[0], String("Outliner"));

    TEST_SECTION("Undocking everything leaves an empty tree rather than a husk");
    Fixture.Area->UndockPanel("Outliner");

    TEST_EXPECT(Fixture.Area->SaveLayout().IsEmpty());
    TEST_EXPECT(Fixture.Area->GetDockedPanelIds().IsEmpty());

    TEST_SECTION("An unregistered id cannot be docked");
    Fixture.Area->DockPanel("Nothing", String(), EDockDirection::Center);
    TEST_EXPECT(Fixture.Area->GetDockedPanelIds().IsEmpty());

    TEST_SECTION("The elements the tree describes are built by the next layout");
    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Fixture.Layout();

    TArray<TSharedPtr<FVisualElement>> Children;
    Fixture.Area->GetChildren(Children);

    TEST_EXPECT_EQ(Children.Size(), 1);
    TEST_EXPECT_EQ(Children[0]->GetContentRectangle().Width, 800);

    TEST_SECTION("Unregistering a panel takes it out of the tree as well as the registry");
    Fixture.Area->UnregisterPanel("Details");

    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));
    TEST_EXPECT_EQ(Fixture.Area->GetRegisteredPanelIds().Size(), 2);

    TEST_END();
}

bool DockingAreaHitTest_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FDockingFixture        Fixture(Application, IntVector2(800, 600), IntVector2(100, 50));

    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Fixture.Layout();

    String         TargetPanelId;
    EDockDirection Direction = EDockDirection::Center;

    TEST_SECTION("A point outside the window is over nothing");
    TEST_EXPECT(!Fixture.Area->HitTestDropTarget(IntVector2(0, 0), TargetPanelId, Direction));

    TEST_SECTION("A leaf offers its strip and one chip a side, and nothing in the middle of the panel");
    TArray<FDropZone> Zones;
    Fixture.Area->GatherDropZones(IntVector2(200, 300), Zones);

    TEST_EXPECT_EQ(Zones.Size(), 5);
    TEST_EXPECT_EQ(Zones[0].Direction, EDockDirection::Center);

    for (const FDropZone& Zone : Zones)
    {
        TEST_EXPECT_EQ(Zone.TargetPanelId, String("Outliner"));
    }

    TEST_SECTION("The middle of a panel is in none of them, so a drop there lands nowhere");
    TEST_EXPECT(!Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + 300), TargetPanelId, Direction));

    TEST_SECTION("Each chip is the side it stands for, over the panel it was gathered from");
    const EDockDirection Directions[] = { EDockDirection::Left, EDockDirection::Right, EDockDirection::Top, EDockDirection::Bottom };
    for (EDockDirection Expected : Directions)
    {
        TEST_EXPECT(Fixture.Area->HitTestDropTarget(Fixture.GetDropZoneCenter(IntVector2(200, 300), Expected), TargetPanelId, Direction));
        TEST_EXPECT_EQ(TargetPanelId, String("Outliner"));
        TEST_EXPECT_EQ(Direction, Expected);
    }

    TEST_SECTION("The tab strip is the one Center target, and it is the whole strip rather than a chip");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + 10), TargetPanelId, Direction));
    TEST_EXPECT_EQ(TargetPanelId, String("Outliner"));
    TEST_EXPECT_EQ(Direction, EDockDirection::Center);

    TEST_SECTION("The other panel has zones of its own, naming it rather than its neighbour");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(Fixture.GetDropZoneCenter(IntVector2(600, 300), EDockDirection::Right), TargetPanelId, Direction));
    TEST_EXPECT_EQ(TargetPanelId, String("Details"));
    TEST_EXPECT_EQ(Direction, EDockDirection::Right);

    TEST_SECTION("Dropping where the hit test points lands the panel there");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(Fixture.GetDropZoneCenter(IntVector2(200, 300), EDockDirection::Right), TargetPanelId, Direction));
    Fixture.Area->DockPanel("Content", TargetPanelId, Direction);

    const FDockNode& Tree = Fixture.Area->SaveLayout();
    TEST_EXPECT_EQ(Tree.Children.Size(), 3);
    TEST_EXPECT_EQ(Tree.Children[1].TabIds[0], String("Content"));

    TEST_SECTION("An area with nothing in it takes a drop anywhere, which is how a panel gets back into one");
    Fixture.Area->UndockPanel("Outliner");
    Fixture.Area->UndockPanel("Details");
    Fixture.Area->UndockPanel("Content");
    Fixture.Layout();

    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + 300), TargetPanelId, Direction));
    TEST_EXPECT(TargetPanelId.IsEmpty());
    TEST_EXPECT_EQ(Direction, EDockDirection::Center);

    TEST_END();
}

bool DockingAreaPersistence_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FDockingFixture        Fixture(Application, IntVector2(800, 600));

    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Fixture.Area->DockPanel("Content", "Details", EDockDirection::Bottom);
    Fixture.Layout();

    const String Filename("DockingTests.tmp.ini");

    TEST_SECTION("A layout writes to a file");
    TEST_EXPECT(Fixture.Area->SaveLayoutToFile(Filename));

    const FDockNode Saved = Fixture.Area->SaveLayout();

    TEST_SECTION("Reading it back into a fresh area rebuilds the same tree");
    FDockingFixture Restored(Application, IntVector2(800, 600));
    TEST_EXPECT(Restored.Area->RestoreLayoutFromFile(Filename));
    TEST_EXPECT(AreNodesEqual(Restored.Area->SaveLayout(), Saved));

    TEST_SECTION("Which means the panels come back where they were");
    const TArray<String> RestoredPanels = Restored.Area->GetDockedPanelIds();
    TEST_EXPECT_EQ(RestoredPanels.Size(), 3);
    TEST_EXPECT_EQ(RestoredPanels[0], String("Outliner"));
    TEST_EXPECT_EQ(RestoredPanels[2], String("Content"));

    TEST_SECTION("Dragged splitter shares survive the round trip");
    FDockNode Resized = Saved;
    Resized.ChildFractions[0] = 0.3f;
    Resized.ChildFractions[1] = 0.7f;

    Fixture.Area->RestoreLayout(Resized);
    TEST_EXPECT(Fixture.Area->SaveLayoutToFile(Filename));

    FDockingFixture ResizedRestore(Application, IntVector2(800, 600));
    TEST_EXPECT(ResizedRestore.Area->RestoreLayoutFromFile(Filename));
    TEST_EXPECT(Math::Abs(ResizedRestore.Area->SaveLayout().ChildFractions[0] - 0.3f) < 0.0001f);

    TEST_SECTION("Which the layout then honours");
    ResizedRestore.Layout();

    TArray<TSharedPtr<FVisualElement>> RootChildren;
    ResizedRestore.Area->GetChildren(RootChildren);

    TArray<TSharedPtr<FVisualElement>> OutsetChildren;
    RootChildren[0]->GetChildren(OutsetChildren);

    TArray<TSharedPtr<FVisualElement>> SplitChildren;
    OutsetChildren[0]->GetChildren(SplitChildren);

    TEST_EXPECT_EQ(SplitChildren.Size(), 2);

    const int32 Gap       = FUIStyle::GetDefault().Panel.Gap;
    const int32 Divisible = 800 - (2 * Gap) - Gap;

    TEST_EXPECT(Math::Abs(SplitChildren[0]->GetContentRectangle().Width - static_cast<int32>(Divisible * 0.3f)) <= 2);

    TEST_SECTION("A saved id nobody registered is dropped rather than leaving a tab with no panel");
    FDockNode WithStranger = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, FDockNode::CreateTabs({ "Outliner" }), FDockNode::CreateTabs({ "Profiler" }));

    Fixture.Area->RestoreLayout(WithStranger);

    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Fixture.Area->GetDockedPanelIds().Size(), 1);
    TEST_EXPECT_EQ(Fixture.Area->GetDockedPanelIds()[0], String("Outliner"));

    TEST_SECTION("A file that is not there leaves the layout alone");
    const FDockNode BeforeMissing = Fixture.Area->SaveLayout();

    TEST_EXPECT(!Fixture.Area->RestoreLayoutFromFile(Filename + ".does-not-exist"));
    TEST_EXPECT(AreNodesEqual(Fixture.Area->SaveLayout(), BeforeMissing));

    ::remove(Filename.Data());

    TEST_END();
}

bool DockingAreaTabReorder_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FDockingFixture        Fixture(Application, IntVector2(800, 600));

    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Center);
    Fixture.Area->DockPanel("Content", "Outliner", EDockDirection::Center);
    Fixture.Layout();

    TEST_SECTION("The three panels start as one stack, in the order they were docked");
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds.Size(), 3);
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds[0], String("Outliner"));
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds[2], String("Content"));

    TSharedPtr<FTabStrip> Strip = FindStrip(Fixture.Area);
    TEST_EXPECT(Strip != nullptr);

    TEST_SECTION("Dragging the last tab over the first moves it along the strip");
    const TSharedPtr<FTab> ContentTab = FindTab(Strip, "Content");

    const IntVector2 OverOutliner(FindTab(Strip, "Outliner")->GetContentRectangle().GetCenter());

    Strip->OnTabPressed(ContentTab.Get(), ContentTab->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(ContentTab.Get(), OverOutliner, OverOutliner);

    TEST_EXPECT_EQ(GetTabOrder(Strip)[0], String("Content"));

    TEST_SECTION("And the tree moves with it, rather than keeping the order it was built with");
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds[0], String("Content"));
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds[1], String("Outliner"));
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().TabIds[2], String("Details"));

    TEST_SECTION("The active index follows the panel it was on rather than the slot it was in");
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().ActiveTabIndex, 0);
    TEST_EXPECT(Fixture.Area->IsPanelVisible("Content"));

    Strip->OnTabReleased(ContentTab.Get(), ContentTab->GetContentRectangle().GetCenter(), ContentTab->GetContentRectangle().GetCenter());

    TEST_SECTION("A rebuild keeps the new order, which is what used to throw it away");
    Fixture.Layout();
    TEST_EXPECT_EQ(GetTabOrder(FindStrip(Fixture.Area))[0], String("Content"));

    TEST_SECTION("And so does a save and restore round trip");
    const String Filename("DockingReorderTests.tmp.ini");
    TEST_EXPECT(Fixture.Area->SaveLayoutToFile(Filename));

    FDockingFixture Restored(Application, IntVector2(800, 600));
    TEST_EXPECT(Restored.Area->RestoreLayoutFromFile(Filename));

    TEST_EXPECT_EQ(Restored.Area->SaveLayout().TabIds[0], String("Content"));
    TEST_EXPECT_EQ(Restored.Area->SaveLayout().TabIds[2], String("Details"));
    TEST_EXPECT_EQ(Restored.Area->SaveLayout().ActiveTabIndex, 0);

    ::remove(Filename.Data());

    TEST_END();
}

bool DockWindowManagerTearOut_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FDockingFixture        Fixture(Application, IntVector2(800, 600));

    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Fixture.Layout();

    FDockWindowManager::FDesc ManagerDesc;
    ManagerDesc.MainArea      = Fixture.Area;
    ManagerDesc.AreaDesc.Font = Fixture.Font;

    FDockWindowManager& Manager = FDockWindowManager::Get();
    Manager.Initialize(ManagerDesc);

    FDockDragState& DragState = FDockDragState::Get();

    const int32 NumWindowsBefore = Application.GetApplication().GetWindows().Size();

    TEST_SECTION("Nothing is torn out to begin with");
    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);

    TEST_SECTION("A drag let go clear of every area has nowhere to drop");
    DragState.BeginDrag("Details", Fixture.Area.Get(), IntVector2(2000, 2000));
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("The drag carries the label the area registered, not the bare id");
    TEST_EXPECT_EQ(DragState.GetDraggedPanelLabel(), String("Details"));

    TEST_SECTION("Letting go there spawns a host window holding the panel");
    DragState.EndDrag();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);
    TEST_EXPECT_EQ(Application.GetApplication().GetWindows().Size(), NumWindowsBefore + 1);

    TEST_SECTION("Which is a top-level window drawing its own caption, not a child that floats over the editor");
    TSharedPtr<FWindow> HostWindow = Manager.GetHostWindow(0);
    TEST_EXPECT(HostWindow != nullptr);
    TEST_EXPECT(HostWindow->GetParentWindow() == nullptr);
    TEST_EXPECT_EQ(HostWindow->GetStyle(), EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar);
    TEST_EXPECT_EQ(HostWindow->GetTitle(), String("Details"));

    TEST_SECTION("The panel moved rather than being copied, so only the host holds it");
    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));
    TEST_EXPECT(Manager.GetHostArea(0)->IsPanelDocked("Details"));
    TEST_EXPECT(Manager.IsPanelDockedAnywhere("Details"));
    TEST_EXPECT(Manager.IsPanelVisibleAnywhere("Details"));
    TEST_EXPECT_EQ(Manager.FindAreaForPanel("Outliner"), Fixture.Area);

    TEST_SECTION("And the area it left collapsed back to the panel still in it");
    TEST_EXPECT_EQ(Fixture.Area->SaveLayout().Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Fixture.Area->GetDockedPanelIds().Size(), 1);

    TEST_SECTION("A host with a panel in it is left alone by a tick");
    Manager.Tick();
    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);

    TEST_SECTION("Dragging the panel back into the main area empties the host");
    Fixture.Layout();
    FApplication::LayoutWindow(HostWindow);

    DragState.BeginDrag("Details", Manager.GetHostArea(0).Get(), Fixture.GetDropZoneCenter(IntVector2(200, 300), EDockDirection::Center));
    TEST_EXPECT_EQ(DragState.GetTargetArea(), Fixture.Area.Get());

    DragState.EndDrag();

    TEST_EXPECT(Fixture.Area->IsPanelDocked("Details"));
    TEST_EXPECT(Manager.GetHostArea(0)->GetDockedPanelIds().IsEmpty());

    TEST_SECTION("Which the next tick notices, closing the window it left behind");
    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);

    Manager.Tick();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);
    TEST_EXPECT_EQ(Application.GetApplication().GetWindows().Size(), NumWindowsBefore);

    TEST_SECTION("A host closed by its own title bar takes what it held with it rather than dropping it back in");
    DragState.BeginDrag("Details", Fixture.Area.Get(), IntVector2(2000, 2000));
    DragState.EndDrag();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);
    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));

    Application.GetApplication().OnWindowClosed(Manager.GetHostWindow(0)->GetPlatformWindow());

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);
    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));
    TEST_EXPECT_EQ(Application.GetApplication().GetWindows().Size(), NumWindowsBefore);

    TEST_SECTION("But the registration lands on the main area, which is what a menu needs to open it again");
    String                     ClosedLabel;
    TSharedPtr<FVisualElement> ClosedPanel;
    TEST_EXPECT(Fixture.Area->GetPanelRegistration("Details", ClosedLabel, ClosedPanel));
    TEST_EXPECT_EQ(ClosedLabel, String("Details"));
    TEST_EXPECT(ClosedPanel != nullptr);

    Fixture.Area->DockPanel("Details", String(), EDockDirection::Center);
    TEST_EXPECT(Fixture.Area->IsPanelDocked("Details"));

    TEST_SECTION("A panel closed inside a host keeps its registration when the emptied host goes");
    DragState.BeginDrag("Details", Fixture.Area.Get(), IntVector2(2000, 2000));
    DragState.EndDrag();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);

    Manager.GetHostArea(0)->UndockPanel("Details");
    TEST_EXPECT(Manager.GetHostArea(0)->GetDockedPanelIds().IsEmpty());
    TEST_EXPECT(Manager.GetHostArea(0)->GetRegisteredPanelIds().Contains(String("Details")));

    Manager.Tick();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);
    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));
    TEST_EXPECT(Fixture.Area->GetRegisteredPanelIds().Contains(String("Details")));

    Fixture.Area->DockPanel("Details", String(), EDockDirection::Center);

    TEST_SECTION("A cancelled drag puts the panel back where it came from without spawning anything");
    DragState.BeginDrag("Details", Fixture.Area.Get(), IntVector2(2000, 2000));
    DragState.CancelDrag();

    TEST_EXPECT(!DragState.IsDragging());
    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);
    TEST_EXPECT(Fixture.Area->IsPanelDocked("Details"));

    TEST_SECTION("A drop outside with the manager gone loses the panel, since tear-out already took it out");
    FDockWindowManager::Shutdown();

    DragState.BeginDrag("Details", Fixture.Area.Get(), IntVector2(2000, 2000));

    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));

    DragState.EndDrag();

    TEST_EXPECT(!Fixture.Area->IsPanelDocked("Details"));
    TEST_EXPECT(!DragState.IsDragging());

    FDockDragState::Shutdown();

    TEST_END();
}

bool DockDecoratorDrag_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    FDockingFixture Source(Application, IntVector2(800, 600), IntVector2(0, 0));
    FDockingFixture Destination(Application, IntVector2(400, 400), IntVector2(1000, 0));

    Source.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Source.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Destination.Area->DockPanel("Content", String(), EDockDirection::Center);

    Source.Layout();
    Destination.Layout();

    FDockWindowManager::FDesc ManagerDesc;
    ManagerDesc.MainArea      = Source.Area;
    ManagerDesc.AreaDesc.Font = Source.Font;

    FDockWindowManager& Manager   = FDockWindowManager::Get();
    FDockDragState&     DragState = FDockDragState::Get();

    Manager.Initialize(ManagerDesc);

    const int32 NumWindowsBefore = Application.GetApplication().GetWindows().Size();

    TEST_SECTION("Nothing is torn out to begin with, so there is no decorator");
    TEST_EXPECT(Manager.GetDecoratorWindow() == nullptr);

    TEST_SECTION("Tearing a tab out puts a window of its own under the cursor");

    const IntVector2 TearOutPosition(600, 700);
    DragState.BeginDrag("Details", Source.Area.Get(), TearOutPosition);

    TSharedPtr<FWindow> Decorator = Manager.GetDecoratorWindow();
    TEST_EXPECT(Decorator != nullptr);
    TEST_EXPECT_EQ(Application.GetApplication().GetWindows().Size(), NumWindowsBefore + 1);
    TEST_EXPECT_EQ(Decorator->GetTitle(), String("Details"));

    TEST_SECTION("Backed off the cursor by the same inset a spawned host uses, so the tab stays under it");
    TEST_EXPECT_EQ(Decorator->GetPosition().X, TearOutPosition.X - FDockWindowManager::SpawnCursorInset);
    TEST_EXPECT_EQ(Decorator->GetPosition().Y, TearOutPosition.Y - FDockWindowManager::SpawnCursorInset);

    TEST_SECTION("A band the width of a host, rather than the host-sized window it stands for");
    TEST_EXPECT_EQ(Decorator->GetSize().X, FDockWindowManager::DecoratorWidth);
    TEST_EXPECT_EQ(Decorator->GetSize().Y, FDockWindowManager::DecoratorHeight);

    TEST_SECTION("And it is click-through, so the cursor keeps resolving to what is behind it");
    TEST_EXPECT(!Decorator->GetAcceptsInput());

    TEST_SECTION("Dimmed, and dimmed on the platform window too, though it was asked for before that existed");
    TEST_EXPECT_EQ(Decorator->GetOpacity(), FDockWindowManager::DecoratorOpacity);
    TEST_EXPECT_EQ(GetStubWindow(Decorator)->GetWindowOpacity(), FDockWindowManager::DecoratorOpacity);

    TEST_SECTION("The panel left its area at that moment rather than on the drop");
    TEST_EXPECT(!Source.Area->IsPanelDocked("Details"));
    TEST_EXPECT(!Source.Area->GetRegisteredPanelIds().Contains(String("Details")));
    TEST_EXPECT(Manager.GetDecoratorArea()->IsPanelDocked("Details"));

    TEST_SECTION("The decorator is no target for its own drag, however far over itself the cursor goes");
    DragState.UpdateDrag(TearOutPosition + IntVector2(40, 40));
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("Over a zone the decorator carries on following the cursor, since the zones show the drop");
    const IntVector2 OverDestinationStrip = Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Center);

    DragState.UpdateDrag(OverDestinationStrip);
    Manager.MoveDecorator(OverDestinationStrip);

    TEST_EXPECT_EQ(DragState.GetTargetArea(), Destination.Area.Get());
    TEST_EXPECT_EQ(DragState.GetTargetDirection(), EDockDirection::Center);
    TEST_EXPECT_EQ(Decorator->GetOpacity(), FDockWindowManager::DecoratorOpacity);
    TEST_EXPECT_EQ(GetStubWindow(Decorator)->GetWindowOpacity(), FDockWindowManager::DecoratorOpacity);
    TEST_EXPECT_EQ(Decorator->GetPosition().X, OverDestinationStrip.X - FDockWindowManager::SpawnCursorInset);
    TEST_EXPECT_EQ(Decorator->GetPosition().Y, OverDestinationStrip.Y - FDockWindowManager::SpawnCursorInset);
    TEST_EXPECT_EQ(Decorator->GetSize().X, FDockWindowManager::DecoratorWidth);
    TEST_EXPECT_EQ(Decorator->GetSize().Y, FDockWindowManager::DecoratorHeight);

    TEST_SECTION("The rectangle a zone stands for is in screen coordinates, so it sits inside its window");
    FRectangle PreviewBounds;
    TEST_EXPECT(Destination.Area->GetDropPreviewBounds(DragState.GetTargetPanelId(), DragState.GetTargetDirection(), PreviewBounds));

    TEST_EXPECT(PreviewBounds.Position.X >= Destination.Window->GetPosition().X);
    TEST_EXPECT(PreviewBounds.Position.Y >= Destination.Window->GetPosition().Y);

    TEST_SECTION("An edge drop takes half the leaf, so the preview shows the split rather than the whole panel");
    const IntVector2 OverDestinationLeftChip = Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Left);

    DragState.UpdateDrag(OverDestinationLeftChip);
    Manager.MoveDecorator(OverDestinationLeftChip);

    TEST_EXPECT_EQ(DragState.GetTargetArea(), Destination.Area.Get());
    TEST_EXPECT_EQ(DragState.GetTargetDirection(), EDockDirection::Left);

    FRectangle LeftBounds;
    TEST_EXPECT(Destination.Area->GetDropPreviewBounds(DragState.GetTargetPanelId(), EDockDirection::Left, LeftBounds));

    TEST_EXPECT_EQ(LeftBounds.Width, PreviewBounds.Width / 2);
    TEST_EXPECT_EQ(LeftBounds.Position.X, PreviewBounds.Position.X);

    TEST_SECTION("Clear of every area the decorator still just follows the cursor, at the size it began at");
    const IntVector2 BetweenWindows(900, 200);

    DragState.UpdateDrag(BetweenWindows);
    Manager.MoveDecorator(BetweenWindows);

    TEST_EXPECT(!DragState.HasTarget());
    TEST_EXPECT_EQ(Decorator->GetOpacity(), FDockWindowManager::DecoratorOpacity);
    TEST_EXPECT_EQ(Manager.GetDecoratorWindow()->GetPosition().X, BetweenWindows.X - FDockWindowManager::SpawnCursorInset);
    TEST_EXPECT_EQ(Manager.GetDecoratorWindow()->GetSize().X, FDockWindowManager::DecoratorWidth);
    TEST_EXPECT_EQ(Manager.GetDecoratorWindow()->GetSize().Y, FDockWindowManager::DecoratorHeight);

    TEST_SECTION("The middle of a panel is no zone either, so the cursor is over an area but aimed at nothing");
    const IntVector2 OverDestinationMiddle(1000 + 200, 200);

    DragState.UpdateDrag(OverDestinationMiddle);
    Manager.MoveDecorator(OverDestinationMiddle);

    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("So letting go there floats the panel into a host of its own rather than docking it");
    const IntVector2 DecoratorPosition = Manager.GetDecoratorWindow()->GetPosition();

    DragState.EndDrag();

    TEST_EXPECT(!Destination.Area->IsPanelDocked("Details"));
    TEST_EXPECT(Manager.GetDecoratorWindow() == nullptr);
    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);
    TEST_EXPECT_EQ(Application.GetApplication().GetWindows().Size(), NumWindowsBefore + 1);
    TEST_EXPECT_EQ(Manager.GetHostWindow(0)->GetPosition().X, DecoratorPosition.X);
    TEST_EXPECT_EQ(Manager.GetHostWindow(0)->GetPosition().Y, DecoratorPosition.Y);
    TEST_EXPECT(Manager.GetHostArea(0)->IsPanelDocked("Details"));

    TEST_SECTION("At the size a host is, rather than the band the decorator followed the cursor at");
    TEST_EXPECT_EQ(Manager.GetHostWindow(0)->GetSize().X, ManagerDesc.DefaultSize.X);
    TEST_EXPECT_EQ(Manager.GetHostWindow(0)->GetSize().Y, ManagerDesc.DefaultSize.Y);

    TEST_SECTION("Letting go over a target docks it there instead, and still takes the decorator down");
    FApplication::LayoutWindow(Manager.GetHostWindow(0));

    DragState.BeginDrag("Details", Manager.GetHostArea(0).Get(), TearOutPosition);
    TEST_EXPECT(Manager.GetDecoratorWindow() != nullptr);

    DragState.UpdateDrag(OverDestinationStrip);
    TEST_EXPECT_EQ(DragState.GetTargetArea(), Destination.Area.Get());

    DragState.EndDrag();

    TEST_EXPECT(Manager.GetDecoratorWindow() == nullptr);
    TEST_EXPECT(Destination.Area->IsPanelDocked("Details"));
    TEST_EXPECT(Manager.GetHostArea(0)->GetDockedPanelIds().IsEmpty());

    TEST_SECTION("And the host the drag emptied is closed by the next tick");
    Manager.Tick();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);
    TEST_EXPECT_EQ(Application.GetApplication().GetWindows().Size(), NumWindowsBefore);

    FDockWindowManager::Shutdown();
    FDockDragState::Shutdown();

    TEST_END();
}

bool DockDecoratorSnapshot_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    FDockingFixture Source(Application, IntVector2(800, 600), IntVector2(0, 0));
    FDockingFixture Destination(Application, IntVector2(400, 400), IntVector2(1000, 0));

    Source.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Source.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Destination.Area->DockPanel("Content", String(), EDockDirection::Center);

    Source.Layout();
    Destination.Layout();

    FDockWindowManager::FDesc ManagerDesc;
    ManagerDesc.MainArea      = Source.Area;
    ManagerDesc.AreaDesc.Font = Source.Font;

    FDockWindowManager& Manager   = FDockWindowManager::Get();
    FDockDragState&     DragState = FDockDragState::Get();

    Manager.Initialize(ManagerDesc);

    IConsoleVariable* SnapshotVariable = FConsoleManager::Get().FindConsoleVariable("Docking.DecoratorSnapshot");
    TEST_EXPECT(SnapshotVariable != nullptr);

    SnapshotVariable->SetAsBool(true, EConsoleVariableFlags::SetByCode);

    TEST_SECTION("Tearing out still puts up a decorator with the panel in it");
    DragState.BeginDrag("Details", Source.Area.Get(), IntVector2(600, 700));

    TEST_EXPECT(Manager.GetDecoratorWindow() != nullptr);
    TEST_EXPECT(Manager.GetDecoratorArea()->IsPanelDocked("Details"));

    TEST_SECTION("With no RHI behind the renderer there is nothing to snapshot into, so the area stays live");
    TEST_EXPECT(Manager.GetDecoratorSnapshot() == nullptr);
    TEST_EXPECT_EQ(Manager.GetDecoratorWindow()->GetContent().Get(), static_cast<FVisualElement*>(Manager.GetDecoratorArea().Get()));

    TEST_SECTION("And the drop commits the same way it does without the trial");
    DragState.UpdateDrag(Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Center));
    DragState.EndDrag();

    TEST_EXPECT(Manager.GetDecoratorWindow() == nullptr);
    TEST_EXPECT(Manager.GetDecoratorSnapshot() == nullptr);
    TEST_EXPECT(Destination.Area->IsPanelDocked("Details"));

    SnapshotVariable->SetAsBool(false, EConsoleVariableFlags::SetByCode);

    FDockWindowManager::Shutdown();
    FDockDragState::Shutdown();

    TEST_END();
}

bool DockDropPreview_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    FDockingFixture Source(Application, IntVector2(800, 600), IntVector2(0, 0));
    FDockingFixture Destination(Application, IntVector2(400, 400), IntVector2(1000, 0));

    Source.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Source.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Destination.Area->DockPanel("Content", String(), EDockDirection::Center);

    Source.Layout();
    Destination.Layout();

    FDockWindowManager::FDesc ManagerDesc;
    ManagerDesc.MainArea      = Source.Area;
    ManagerDesc.AreaDesc.Font = Source.Font;

    FDockWindowManager& Manager   = FDockWindowManager::Get();
    FDockDragState&     DragState = FDockDragState::Get();

    Manager.Initialize(ManagerDesc);

    TEST_SECTION("Nothing is in flight, so there is no picture and asking for one changes that not at all");
    TEST_EXPECT(Manager.GetDropPreviewTexture() == nullptr);

    Manager.UpdateDropPreview();
    TEST_EXPECT(Manager.GetDropPreviewTexture() == nullptr);

    TEST_SECTION("A leaf still offers its strip and one chip a side once the chips are twice the size");
    TArray<FDropZone> Zones;
    Destination.Area->GatherDropZones(IntVector2(200, 200), Zones);
    TEST_EXPECT_EQ(Zones.Size(), 5);

    TEST_SECTION("Tearing out and aiming at a zone puts the picture's rectangle on the target");
    DragState.BeginDrag("Details", Source.Area.Get(), IntVector2(600, 700));
    TEST_EXPECT(Manager.GetDecoratorWindow() != nullptr);

    const IntVector2 OverDestinationLeftChip = Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Left);

    DragState.UpdateDrag(OverDestinationLeftChip);
    Manager.MoveDecorator(OverDestinationLeftChip);
    Manager.UpdateDropPreview();

    TEST_EXPECT_EQ(DragState.GetTargetArea(), Destination.Area.Get());

    TEST_SECTION("With no RHI behind the renderer there is nothing to render into, so the target draws a fill");
    TEST_EXPECT(Manager.GetDropPreviewTexture() == nullptr);

    TEST_SECTION("The area was only borrowed to render at the landing size, so the decorator has it back");
    TEST_EXPECT_EQ(Manager.GetDecoratorWindow()->GetSize().X, FDockWindowManager::DecoratorWidth);
    TEST_EXPECT_EQ(Manager.GetDecoratorWindow()->GetSize().Y, FDockWindowManager::DecoratorHeight);
    TEST_EXPECT_EQ(Manager.GetDecoratorArea()->GetContentRectangle().Width, FDockWindowManager::DecoratorWidth);
    TEST_EXPECT_EQ(Manager.GetDecoratorArea()->GetContentRectangle().Height, FDockWindowManager::DecoratorHeight);

    TEST_SECTION("Aiming at nothing shows no picture, since the chips are islands and the gaps between them "
        "are aimed at nothing on the way from one to the next");
    const IntVector2 BetweenWindows(900, 700);

    DragState.UpdateDrag(BetweenWindows);
    Manager.MoveDecorator(BetweenWindows);
    Manager.UpdateDropPreview();

    TEST_EXPECT(!DragState.HasTarget());
    TEST_EXPECT(Manager.GetDropPreviewTexture() == nullptr);

    TEST_SECTION("And coming back to the chip it left is the same zone again rather than a new one");
    DragState.UpdateDrag(OverDestinationLeftChip);
    Manager.MoveDecorator(OverDestinationLeftChip);
    Manager.UpdateDropPreview();

    TEST_EXPECT_EQ(DragState.GetTargetArea(), Destination.Area.Get());
    TEST_EXPECT_EQ(DragState.GetTargetDirection(), EDockDirection::Left);

    TEST_SECTION("Letting go takes the picture down with the decorator");
    const IntVector2 OverDestinationStrip = Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Center);

    DragState.UpdateDrag(OverDestinationStrip);
    Manager.MoveDecorator(OverDestinationStrip);
    Manager.UpdateDropPreview();

    DragState.EndDrag();

    TEST_EXPECT(Manager.GetDecoratorWindow() == nullptr);
    TEST_EXPECT(Manager.GetDropPreviewTexture() == nullptr);
    TEST_EXPECT(Destination.Area->IsPanelDocked("Details"));

    FDockWindowManager::Shutdown();
    FDockDragState::Shutdown();

    TEST_END();
}

bool DockHostNativeDrag_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    FDockingFixture Main(Application, IntVector2(800, 600), IntVector2(0, 0));

    Main.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Main.Area->DockPanel("Details", "Outliner", EDockDirection::Right);
    Main.Layout();

    FDockWindowManager::FDesc ManagerDesc;
    ManagerDesc.MainArea      = Main.Area;
    ManagerDesc.AreaDesc.Font = Main.Font;

    FDockWindowManager& Manager   = FDockWindowManager::Get();
    FDockDragState&     DragState = FDockDragState::Get();

    Manager.Initialize(ManagerDesc);

    DragState.BeginDrag("Details", Main.Area.Get(), IntVector2(2000, 2000));
    DragState.EndDrag();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);

    Main.Layout();

    TSharedPtr<FWindow> HostWindow = Manager.GetHostWindow(0);
    FApplication::LayoutWindow(HostWindow);

    FApplication& App = Application.GetApplication();

    const IntVector2 GrabOffset(120, 12);
    const IntVector2 GrabPoint = HostWindow->GetPosition() + GrabOffset;

    TEST_SECTION("The OS taking a host by its title bar starts no drag, since only a tab drag docks");
    Application.GetPlatformApplication()->GetCursor()->SetPosition(GrabPoint.X, GrabPoint.Y);
    App.BeginWindowInteraction(HostWindow->GetPlatformWindow(), EWindowInteraction::Move);

    TEST_EXPECT(!DragState.IsDragging());
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("The moves it delivers move the window and resolve nothing");
    const IntVector2 OverMainArea(600, 300);

    App.OnWindowMoved(HostWindow->GetPlatformWindow(), OverMainArea.X - GrabOffset.X, OverMainArea.Y - GrabOffset.Y);

    TEST_EXPECT_EQ(HostWindow->GetPosition().X, OverMainArea.X - GrabOffset.X);
    TEST_EXPECT_EQ(HostWindow->GetPosition().Y, OverMainArea.Y - GrabOffset.Y);
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("And letting go over the main area leaves the host the window it was");
    Application.GetPlatformApplication()->GetCursor()->SetPosition(OverMainArea.X, OverMainArea.Y);
    App.EndWindowInteraction(HostWindow->GetPlatformWindow(), EWindowInteraction::Move);

    TEST_EXPECT(!DragState.HasTarget());
    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);
    TEST_EXPECT(Manager.GetHostArea(0)->IsPanelDocked("Details"));
    TEST_EXPECT(!Main.Area->IsPanelDocked("Details"));

    TEST_SECTION("Dragging the main window over the host is the same the other way round");
    App.BeginWindowInteraction(Main.Window->GetPlatformWindow(), EWindowInteraction::Move);
    App.OnWindowMoved(Main.Window->GetPlatformWindow(), OverMainArea.X, OverMainArea.Y);

    TEST_EXPECT(!DragState.HasTarget());

    App.EndWindowInteraction(Main.Window->GetPlatformWindow(), EWindowInteraction::Move);

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 1);

    TEST_SECTION("Dragging the tab out of that same host still docks, so only the whole-window move changed");
    Main.Layout();
    FApplication::LayoutWindow(Manager.GetHostWindow(0));

    const IntVector2 OverMainStrip = Main.GetDropZoneCenter(IntVector2(400, 300), EDockDirection::Center);

    DragState.BeginDrag("Details", Manager.GetHostArea(0).Get(), IntVector2(2000, 2000));
    DragState.UpdateDrag(OverMainStrip);

    TEST_EXPECT_EQ(DragState.GetTargetArea(), Main.Area.Get());

    DragState.EndDrag();
    Manager.Tick();

    TEST_EXPECT_EQ(Manager.GetNumHosts(), 0);
    TEST_EXPECT(Main.Area->IsPanelDocked("Details"));

    FDockWindowManager::Shutdown();
    FDockDragState::Shutdown();

    TEST_END();
}

bool DockLayoutFileMultiWindow_Test()
{
    TEST_BEGIN();

    const String Filename("DockingMultiWindowTests.tmp.ini");
    const String LegacyFilename("DockingLegacyTests.tmp.ini");

    FDockWindowLayout Main;
    Main.Title    = "Editor";
    Main.Position = IntVector2(0, 0);
    Main.Size     = IntVector2(1600, 900);
    Main.Root     = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal,
        FDockNode::CreateTabs({ "Outliner" }), FDockNode::CreateTabs({ "Viewport", "Stats" }));

    FDockWindowLayout Host;
    Host.Title               = "Details";
    Host.Position            = IntVector2(320, 180);
    Host.Size                = IntVector2(520, 400);
    Host.Root                = FDockNode::CreateTabs({ "Details", "Materials" });
    Host.Root.ActiveTabIndex = 1;

    TArray<FDockWindowLayout> Windows;
    Windows.Add(Main);
    Windows.Add(Host);

    TEST_SECTION("Two windows write to one file");
    TEST_EXPECT(FDockLayoutFile::Save(Filename, Windows));

    TEST_SECTION("And read back with their trees, captions, positions and sizes");
    TArray<FDockWindowLayout> Restored;
    TEST_EXPECT(FDockLayoutFile::Load(Filename, Restored));
    TEST_EXPECT_EQ(Restored.Size(), 2);

    TEST_EXPECT_EQ(Restored[0].Title, String("Editor"));
    TEST_EXPECT_EQ(Restored[0].Size.X, 1600);
    TEST_EXPECT(AreNodesEqual(Restored[0].Root, Main.Root));

    TEST_EXPECT_EQ(Restored[1].Title, String("Details"));
    TEST_EXPECT_EQ(Restored[1].Position.X, 320);
    TEST_EXPECT_EQ(Restored[1].Position.Y, 180);
    TEST_EXPECT_EQ(Restored[1].Size.Y, 400);
    TEST_EXPECT(AreNodesEqual(Restored[1].Root, Host.Root));

    TEST_SECTION("The main window is written first, so the version 1 key still names its tree");
    FIniFile File;
    TEST_EXPECT(File.LoadFromFile(Filename));

    int32 Version    = 0;
    int32 RootIndex  = -1;
    int32 NumWindows = 0;

    TEST_EXPECT(File.GetInt("Layout", "Version", Version));
    TEST_EXPECT_EQ(Version, FDockLayoutFile::CurrentVersion);

    TEST_EXPECT(File.GetInt("Layout", "Root", RootIndex));
    TEST_EXPECT_EQ(RootIndex, 0);

    TEST_EXPECT(File.GetInt("Layout", "NumWindows", NumWindows));
    TEST_EXPECT_EQ(NumWindows, 2);

    TEST_SECTION("A single-window area still round-trips through the same file");
    FScopedStubApplication Application;
    FDockingFixture        Fixture(Application, IntVector2(800, 600));

    Fixture.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Fixture.Area->DockPanel("Details", "Outliner", EDockDirection::Right);

    TEST_EXPECT(Fixture.Area->SaveLayoutToFile(Filename));

    FDockingFixture SingleRestore(Application, IntVector2(800, 600));
    TEST_EXPECT(SingleRestore.Area->RestoreLayoutFromFile(Filename));
    TEST_EXPECT(AreNodesEqual(SingleRestore.Area->SaveLayout(), Fixture.Area->SaveLayout()));

    TEST_SECTION("A version 1 file reads back as a main window on its own, carrying no shape");
    FIniFile Legacy;
    Legacy.Filename = LegacyFilename;

    Legacy.SetOrAddInt("Layout", "Version", FDockLayoutFile::SingleWindowVersion);
    Legacy.SetOrAddInt("Layout", "NumNodes", 1);
    Legacy.SetOrAddInt("Layout", "Root", 0);
    Legacy.SetOrAddString("Node0", "Kind", String("Tabs"));
    Legacy.SetOrAddString("Node0", "Tabs", String("Outliner,Details"));
    Legacy.SetOrAddInt("Node0", "ActiveTab", 1);

    TEST_EXPECT(Legacy.WriteToFile());

    TArray<FDockWindowLayout> LegacyWindows;
    TEST_EXPECT(FDockLayoutFile::Load(LegacyFilename, LegacyWindows));
    TEST_EXPECT_EQ(LegacyWindows.Size(), 1);
    TEST_EXPECT_EQ(LegacyWindows[0].Root.TabIds.Size(), 2);
    TEST_EXPECT_EQ(LegacyWindows[0].Root.ActiveTabIndex, 1);
    TEST_EXPECT_EQ(LegacyWindows[0].Size.X, 0);

    TEST_SECTION("A version nobody knows is refused rather than half-read");
    Legacy.SetOrAddInt("Layout", "Version", 99);
    TEST_EXPECT(Legacy.WriteToFile());

    LegacyWindows.Clear();
    TEST_EXPECT(!FDockLayoutFile::Load(LegacyFilename, LegacyWindows));
    TEST_EXPECT(LegacyWindows.IsEmpty());

    TEST_SECTION("A file that is not there is refused too");
    TEST_EXPECT(!FDockLayoutFile::Load(Filename + ".does-not-exist", LegacyWindows));

    ::remove(Filename.Data());
    ::remove(LegacyFilename.Data());

    TEST_END();
}

bool DockDragState_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    FDockingFixture Source(Application, IntVector2(400, 400), IntVector2(0, 0));
    FDockingFixture Destination(Application, IntVector2(400, 400), IntVector2(500, 0));

    Source.Area->DockPanel("Outliner", String(), EDockDirection::Center);
    Source.Area->DockPanel("Details", "Outliner", EDockDirection::Center);
    Destination.Area->DockPanel("Content", String(), EDockDirection::Center);

    Source.Layout();
    Destination.Layout();

    FDockDragState& DragState = FDockDragState::Get();

    TEST_SECTION("Nothing is in flight to begin with");
    TEST_EXPECT(!DragState.IsDragging());
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("A torn-out tab starts a drag that carries the panel");
    DragState.BeginDrag("Details", Source.Area.Get(), Source.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Center));

    TEST_EXPECT(DragState.IsDragging());
    TEST_EXPECT_EQ(DragState.GetDraggedPanelId(), String("Details"));

    TEST_SECTION("Over a zone of the area it came from, the drop is resolved against that area");
    TEST_EXPECT(DragState.HasTarget());
    TEST_EXPECT_EQ(DragState.GetTargetArea(), Source.Area.Get());

    TEST_SECTION("Between the two windows there is nowhere to drop");
    DragState.UpdateDrag(IntVector2(450, 200));

    TEST_EXPECT(DragState.IsDragging());
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("Over the other window the target moves with the cursor");
    DragState.UpdateDrag(Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Right));

    TEST_EXPECT_EQ(DragState.GetTargetArea(), Destination.Area.Get());
    TEST_EXPECT_EQ(DragState.GetTargetPanelId(), String("Content"));
    TEST_EXPECT_EQ(DragState.GetTargetDirection(), EDockDirection::Right);

    TEST_SECTION("Letting go there docks it, and it is gone from where it started");
    DragState.EndDrag();

    TEST_EXPECT(!DragState.IsDragging());
    TEST_EXPECT(!Source.Area->IsPanelDocked("Details"));
    TEST_EXPECT(Destination.Area->IsPanelDocked("Details"));

    const FDockNode& DestinationTree = Destination.Area->SaveLayout();
    TEST_EXPECT_EQ(DestinationTree.Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(DestinationTree.Children[1].TabIds[0], String("Details"));

    TEST_SECTION("The area it left collapsed back to the one panel still in it");
    TEST_EXPECT_EQ(Source.Area->SaveLayout().Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Source.Area->GetDockedPanelIds().Size(), 1);

    TEST_SECTION("A cancelled drag leaves both trees where they were");
    Source.Layout();
    Destination.Layout();

    const TArray<String> BeforeCancel = Destination.Area->GetDockedPanelIds();

    DragState.BeginDrag("Outliner", Source.Area.Get(), IntVector2(500 + 200, 200));
    DragState.CancelDrag();

    TEST_EXPECT(!DragState.IsDragging());
    TEST_EXPECT(Source.Area->IsPanelDocked("Outliner"));
    TEST_EXPECT_EQ(Destination.Area->GetDockedPanelIds().Size(), BeforeCancel.Size());

    TEST_SECTION("An area that has gone is no longer a drop target");
    const IntVector2 OverDestinationStrip = Destination.GetDropZoneCenter(IntVector2(200, 200), EDockDirection::Center);

    Destination.Area.Reset();
    Destination.Window->SetContent(nullptr);

    DragState.BeginDrag("Outliner", Source.Area.Get(), OverDestinationStrip);

    TEST_EXPECT(!DragState.HasTarget());
    DragState.CancelDrag();

    FDockDragState::Shutdown();

    TEST_END();
}
