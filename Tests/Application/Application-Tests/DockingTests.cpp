#include "DockingTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Docking/DockDragState.h>
#include <Application/Docking/DockNode.h>
#include <Application/Docking/DockingArea.h>
#include <Application/Docking/Splitter.h>
#include <Application/Docking/TabStrip.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>

/** @brief An eight by sixteen face, so every tab width in these tests is exact. */
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

/** @brief A panel with a fixed size, so a splitter has something with a minimum to arrange. */
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

/** @brief Presses a handle, drags it and lets go, which is one splitter gesture end to end. */
static void DragSplitterHandle(const TSharedPtr<FSplitter>& Splitter, int32 HandleIndex, const IntVector2& Delta)
{
    const IntVector2 Start = Splitter->GetHandleRectangle(HandleIndex).GetCenter();
    const IntVector2 End   = Start + Delta;

    Splitter->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Start, true));
    Splitter->OnMouseMove(MakeMoveEvent(End));
    Splitter->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, End, false));
}

/** @brief The tab standing for a panel, or null when the strip does not hold it. */
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

/** @brief The panel ids of a strip, in strip order. */
static TArray<String> GetTabOrder(const TSharedPtr<FTabStrip>& Strip)
{
    TArray<String> Order;
    for (const TSharedPtr<FTab>& Tab : Strip->GetTabs())
    {
        Order.Add(Tab->GetPanelId());
    }

    return Order;
}

/** @brief Compares two trees field by field, which is what a save and restore round trip has to preserve. */
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

    // The fractions went through a printf and back, so they are only equal to the precision written
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

/** @brief A three-panel area built into a window, which is the shape most of these tests need. */
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

    TSharedPtr<IFontFace>    Font;
    TSharedPtr<FWindow>      Window;
    TSharedPtr<FDockingArea> Area;
};

bool DockNodeMinimumSize_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A leaf is a panel with a strip over it");
    const FDockNode Leaf = FDockNode::MakeTabs({ "Outliner" });

    TEST_EXPECT_EQ(Leaf.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth);
    TEST_EXPECT_EQ(Leaf.ComputeMinimumSize().Y, FDockMetrics::MinimumPanelHeight + FDockMetrics::TabStripHeight);

    TEST_SECTION("A row adds its children's widths and the handle between them, and takes the taller one");
    const FDockNode Row = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, Leaf, Leaf);

    TEST_EXPECT_EQ(Row.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth * 2 + FDockMetrics::SplitterThickness);
    TEST_EXPECT_EQ(Row.ComputeMinimumSize().Y, Leaf.ComputeMinimumSize().Y);

    TEST_SECTION("A column does the same the other way round");
    const FDockNode Column = FDockNode::MakeSplit(EDockSplitOrientation::Vertical, Leaf, Leaf);

    TEST_EXPECT_EQ(Column.ComputeMinimumSize().X, FDockMetrics::MinimumPanelWidth);
    TEST_EXPECT_EQ(Column.ComputeMinimumSize().Y, Leaf.ComputeMinimumSize().Y * 2 + FDockMetrics::SplitterThickness);

    TEST_SECTION("Nesting accumulates, so an outer drag cannot squeeze an inner panel out of existence");
    const FDockNode Nested = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, Leaf, Column);

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

    const FDockNode Outliner = FDockNode::MakeTabs({ "Outliner" });
    const FDockNode Details  = FDockNode::MakeTabs({ "Details" });

    TEST_SECTION("An emptied leaf is dropped and the split of one that leaves becomes its other child");
    FDockNode Row = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, Outliner, Details);
    Row.Children[0].TabIds.Clear();
    Row.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Row.Kind, EDockNodeKind::Tabs);
    TEST_EXPECT_EQ(Row.TabIds.Size(), 1);
    TEST_EXPECT_EQ(Row.TabIds[0], String("Details"));

    TEST_SECTION("A split whose child splits the same way absorbs it rather than nesting");
    FDockNode Inner = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, Outliner, Details);
    FDockNode Outer = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, Inner, FDockNode::MakeTabs({ "Content" }));

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
    FDockNode Mixed = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, FDockNode::MakeSplit(EDockSplitOrientation::Vertical, Outliner, Details), FDockNode::MakeTabs({ "Content" }));
    Mixed.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Mixed.Children.Size(), 2);
    TEST_EXPECT_EQ(Mixed.Children[0].Kind, EDockNodeKind::Split);
    TEST_EXPECT_EQ(Mixed.Children[0].Orientation, EDockSplitOrientation::Vertical);

    TEST_SECTION("Emptying everything leaves an empty leaf rather than a split with no children");
    FDockNode Everything = FDockNode::MakeSplit(EDockSplitOrientation::Vertical, Outliner, Details);
    Everything.Children[0].TabIds.Clear();
    Everything.Children[1].TabIds.Clear();
    Everything.CollapseDegenerateNodes();

    TEST_EXPECT_EQ(Everything.Kind, EDockNodeKind::Tabs);
    TEST_EXPECT(Everything.IsEmpty());

    TEST_SECTION("A collapse that empties two levels does not leave a rung behind");
    FDockNode Deep = FDockNode::MakeSplit(
        EDockSplitOrientation::Horizontal,
        FDockNode::MakeSplit(EDockSplitOrientation::Vertical, Outliner, Details),
        FDockNode::MakeTabs({ "Content" }));

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
    FDockNode Stack = FDockNode::MakeTabs({ "Outliner", "Details" });
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

        // The trailing child stops at the 250 the description named, not the 10 AddChild was given
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

    TEST_SECTION("The tabs are laid out end to end, each as wide as its label needs");
    TEST_EXPECT_EQ(FindTab(Strip, "Outliner")->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(FindTab(Strip, "Details")->GetContentRectangle().Position.X, FindTab(Strip, "Outliner")->GetContentRectangle().GetRight());

    TEST_SECTION("Pressing a tab shows it, so a drag starts from the panel it is about to move");
    const TSharedPtr<FTab> DetailsTab = FindTab(Strip, "Details");
    DetailsTab->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, DetailsTab->GetContentRectangle().GetCenter(), true));

    TEST_EXPECT_EQ(Strip->GetActivePanelId(), String("Details"));
    TEST_EXPECT_EQ(ActivatedPanelId, String("Details"));
    TEST_EXPECT_EQ(Strip->GetDraggedPanelId(), String("Details"));

    TEST_SECTION("Dragging it over the first tab moves it there and reports the new index");
    const IntVector2 OverFirst(FindTab(Strip, "Outliner")->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(DetailsTab.Get(), OverFirst);

    TEST_EXPECT_EQ(GetTabOrder(Strip)[0], String("Details"));
    TEST_EXPECT_EQ(GetTabOrder(Strip)[1], String("Outliner"));
    TEST_EXPECT_EQ(ReorderedPanelId, String("Details"));
    TEST_EXPECT_EQ(ReorderedIndex, 0);

    TEST_SECTION("The moved tab is arranged where it now sits rather than where it was");
    TEST_EXPECT_EQ(DetailsTab->GetContentRectangle().Position.X, 0);

    TEST_SECTION("Letting go ends the drag");
    DetailsTab->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, DetailsTab->GetContentRectangle().GetCenter(), false));
    TEST_EXPECT(Strip->GetDraggedPanelId().IsEmpty());

    TEST_SECTION("A drag past the far end takes the tab to the far end");
    const TSharedPtr<FTab> ContentTab = FindTab(Strip, "Content");
    Strip->OnTabPressed(ContentTab.Get(), ContentTab->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(ContentTab.Get(), IntVector2(-40, 10));

    TEST_EXPECT_EQ(GetTabOrder(Strip)[0], String("Content"));

    Strip->OnTabReleased(ContentTab.Get(), ContentTab->GetContentRectangle().GetCenter());

    TEST_SECTION("Clicking the cross closes the panel, and clicking the label does not");
    const TSharedPtr<FTab> OutlinerTab = FindTab(Strip, "Outliner");

    Strip->OnTabPressed(OutlinerTab.Get(), OutlinerTab->GetContentRectangle().Position + IntVector2(4, 4));
    Strip->OnTabClicked(OutlinerTab.Get());
    TEST_EXPECT(ClosedPanelId.IsEmpty());

    Strip->OnTabPressed(OutlinerTab.Get(), OutlinerTab->GetCloseButtonRectangle().GetCenter());
    Strip->OnTabClicked(OutlinerTab.Get());
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
    Fixed->OnTabDragged(SecondTab.Get(), IntVector2(2, 10));

    TEST_EXPECT_EQ(GetTabOrder(Fixed)[0], String("First"));

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
    Desc.OnTabDragDetached = FOnTabDragDetached::CreateLambda([&](const String& PanelId, const IntVector2& Position)
    {
        DetachedPanelId  = PanelId;
        DetachedPosition = Position;
        NumDetachCallbacks++;
    });

    Desc.OnTabDragMoved = FOnTabDragMoved::CreateLambda([&](const String&, const IntVector2& Position)
    {
        LastMovePosition = Position;
        NumMoveCallbacks++;
    });

    Desc.OnTabDragFinished = FOnTabDragFinished::CreateLambda([&](const String&, const IntVector2&) { NumFinishedCallbacks++; });

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(Desc);
    Strip->AddTab("Outliner", "Outliner", true);
    Strip->AddTab("Details", "Details", true);

    LayoutElement(Strip, FRectangle(IntVector2(0, 30), 600, FDockMetrics::TabStripHeight));

    const TSharedPtr<FTab> DetailsTab = FindTab(Strip, "Details");
    Strip->OnTabPressed(DetailsTab.Get(), DetailsTab->GetContentRectangle().GetCenter());

    TEST_SECTION("A drag inside the strip is a reorder, however far it goes sideways");
    Strip->OnTabDragged(DetailsTab.Get(), IntVector2(4, 40));
    TEST_EXPECT_EQ(NumDetachCallbacks, 0);

    TEST_SECTION("A drag just off the strip is not enough either, so a shaky hand does not tear a panel out");
    Strip->OnTabDragged(DetailsTab.Get(), IntVector2(200, 30 + FDockMetrics::TabStripHeight + FTabStrip::TearOutDistance - 1));
    TEST_EXPECT_EQ(NumDetachCallbacks, 0);

    TEST_SECTION("Past the threshold the panel comes out, once");
    const IntVector2 FarBelow(200, 30 + FDockMetrics::TabStripHeight + FTabStrip::TearOutDistance + 10);

    Strip->OnTabDragged(DetailsTab.Get(), FarBelow);
    Strip->OnTabDragged(DetailsTab.Get(), FarBelow + IntVector2(0, 20));

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
    Strip->OnTabReleased(DetailsTab.Get(), LastMovePosition);

    TEST_EXPECT_EQ(NumFinishedCallbacks, 1);
    TEST_EXPECT(Strip->GetDetachedPanelId().IsEmpty());

    TEST_SECTION("Dragging above the strip tears out too");
    Strip->OnTabPressed(DetailsTab.Get(), DetailsTab->GetContentRectangle().GetCenter());
    Strip->OnTabDragged(DetailsTab.Get(), IntVector2(200, 30 - FTabStrip::TearOutDistance - 10));

    TEST_EXPECT_EQ(NumDetachCallbacks, 2);

    TEST_SECTION("A strip with tear-out turned off reorders instead");
    FTabStrip::FDesc FixedDesc;
    FixedDesc.Font              = Font;
    FixedDesc.bAllowTearOut     = false;
    FixedDesc.OnTabDragDetached = FOnTabDragDetached::CreateLambda([&](const String&, const IntVector2&) { NumDetachCallbacks++; });

    TSharedPtr<FTabStrip> Fixed = FTabStrip::Create(FixedDesc);
    Fixed->AddTab("First", "First", false);
    Fixed->AddTab("Second", "Second", false);

    LayoutElement(Fixed, FRectangle(IntVector2(0, 0), 600, FDockMetrics::TabStripHeight));

    const TSharedPtr<FTab> SecondTab = FindTab(Fixed, "Second");
    Fixed->OnTabPressed(SecondTab.Get(), SecondTab->GetContentRectangle().GetCenter());
    Fixed->OnTabDragged(SecondTab.Get(), IntVector2(4, 400));

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

    TEST_SECTION("The middle of the left panel is a center drop onto it");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + 300), TargetPanelId, Direction));
    TEST_EXPECT_EQ(TargetPanelId, String("Outliner"));
    TEST_EXPECT_EQ(Direction, EDockDirection::Center);

    TEST_SECTION("The middle of the right panel resolves to the other one");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 600, 50 + 300), TargetPanelId, Direction));
    TEST_EXPECT_EQ(TargetPanelId, String("Details"));
    TEST_EXPECT_EQ(Direction, EDockDirection::Center);

    TEST_SECTION("Each edge of a panel is its own drop zone");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 10, 50 + 300), TargetPanelId, Direction));
    TEST_EXPECT_EQ(Direction, EDockDirection::Left);

    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 380, 50 + 300), TargetPanelId, Direction));
    TEST_EXPECT_EQ(TargetPanelId, String("Outliner"));
    TEST_EXPECT_EQ(Direction, EDockDirection::Right);

    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + 590), TargetPanelId, Direction));
    TEST_EXPECT_EQ(Direction, EDockDirection::Bottom);

    TEST_SECTION("A drop on the tab strip is another tab, not the top edge underneath it");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + 10), TargetPanelId, Direction));
    TEST_EXPECT_EQ(TargetPanelId, String("Outliner"));
    TEST_EXPECT_EQ(Direction, EDockDirection::Center);

    TEST_SECTION("The top edge below the strip is a top drop");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 200, 50 + FDockMetrics::TabStripHeight + 10), TargetPanelId, Direction));
    TEST_EXPECT_EQ(Direction, EDockDirection::Top);

    TEST_SECTION("A corner picks the edge it is nearer to");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 4, 50 + 560), TargetPanelId, Direction));
    TEST_EXPECT_EQ(Direction, EDockDirection::Left);

    TEST_SECTION("Dropping where the hit test points lands the panel there");
    TEST_EXPECT(Fixture.Area->HitTestDropTarget(IntVector2(100 + 380, 50 + 300), TargetPanelId, Direction));
    Fixture.Area->DockPanel("Content", TargetPanelId, Direction);

    const FDockNode& Tree = Fixture.Area->SaveLayout();
    TEST_EXPECT_EQ(Tree.Children.Size(), 3);
    TEST_EXPECT_EQ(Tree.Children[1].TabIds[0], String("Content"));

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

    // Relative, so it lands in the working directory next to TestResults_Application.log
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

    TArray<TSharedPtr<FVisualElement>> SplitChildren;
    RootChildren[0]->GetChildren(SplitChildren);

    TEST_EXPECT_EQ(SplitChildren.Size(), 2);
    TEST_EXPECT(Math::Abs(SplitChildren[0]->GetContentRectangle().Width - 239) <= 2);

    TEST_SECTION("A saved id nobody registered is dropped rather than leaving a tab with no panel");
    FDockNode WithStranger = FDockNode::MakeSplit(EDockSplitOrientation::Horizontal, FDockNode::MakeTabs({ "Outliner" }), FDockNode::MakeTabs({ "Profiler" }));

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
    DragState.BeginDrag("Details", Source.Area.Get(), IntVector2(200, 200));

    TEST_EXPECT(DragState.IsDragging());
    TEST_EXPECT_EQ(DragState.GetDraggedPanelId(), String("Details"));

    TEST_SECTION("Over the area it came from, the drop is resolved against that area");
    TEST_EXPECT(DragState.HasTarget());
    TEST_EXPECT_EQ(DragState.GetTargetArea(), Source.Area.Get());

    TEST_SECTION("Between the two windows there is nowhere to drop");
    DragState.UpdateDrag(IntVector2(450, 200));

    TEST_EXPECT(DragState.IsDragging());
    TEST_EXPECT(!DragState.HasTarget());

    TEST_SECTION("Over the other window the target moves with the cursor");
    DragState.UpdateDrag(IntVector2(500 + 380, 200));

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
    Destination.Area.Reset();
    Destination.Window->SetContent(nullptr);

    DragState.BeginDrag("Outliner", Source.Area.Get(), IntVector2(500 + 200, 200));

    TEST_EXPECT(!DragState.HasTarget());
    DragState.CancelDrag();

    FDockDragState::Shutdown();

    TEST_END();
}
