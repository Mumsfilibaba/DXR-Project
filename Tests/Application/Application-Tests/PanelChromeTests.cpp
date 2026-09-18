#include "PanelChromeTests.h"
#include "UISnapshot.h"

#include <TestCommon/TestMacros.h>

#include <Application/Docking/DockingArea.h>
#include <Application/Docking/DockNode.h>
#include <Application/Docking/Splitter.h>
#include <Application/Docking/TabStrip.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Separator.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Input/Keys.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

constexpr int32 SHELL_WIDTH  = 1280;
constexpr int32 SHELL_HEIGHT = 720;

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static TSharedPtr<FVisualElement> MakePanelBody(const String& Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = Font;

    return FTextBlock::Create(Desc);
}

static TSharedPtr<FDockingArea> BuildShellArea(const TSharedPtr<IFontFace>& Font)
{
    FDockingArea::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FDockingArea> Area = FDockingArea::Create(Desc);

    Area->RegisterPanel("Hierarchy", "Scene Hierarchy", MakePanelBody("Hierarchy", Font));
    Area->RegisterPanel("Viewport", "Viewport", MakePanelBody("Viewport", Font));
    Area->RegisterPanel("Details", "Details", MakePanelBody("Details", Font));
    Area->RegisterPanel("Output", "Output Log", MakePanelBody("Output Log", Font));
    Area->RegisterPanel("Content", "Content Browser", MakePanelBody("Content Browser", Font));

    const FDockNode Hierarchy = FDockNode::CreateTabs({ String("Hierarchy") });
    const FDockNode Viewport  = FDockNode::CreateTabs({ String("Viewport") });
    const FDockNode Details   = FDockNode::CreateTabs({ String("Details") });
    const FDockNode Bottom    = FDockNode::CreateTabs({ String("Output"), String("Content") });

    const FDockNode Centre = FDockNode::CreateSplit(EDockSplitOrientation::Vertical, Viewport, Bottom);
    const FDockNode Left   = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Hierarchy, Centre);
    const FDockNode Root   = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Left, Details);

    Area->RestoreLayout(Root);
    return Area;
}

static TSharedPtr<FDockingArea> BuildPairArea(const TSharedPtr<IFontFace>& Font)
{
    FDockingArea::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FDockingArea> Area = FDockingArea::Create(Desc);
    Area->RegisterPanel("Left", "Left Panel", MakePanelBody("Left", Font));
    Area->RegisterPanel("Right", "Right Panel", MakePanelBody("Right", Font));

    Area->RestoreLayout(FDockNode::CreateSplit(
        EDockSplitOrientation::Horizontal,
        FDockNode::CreateTabs({ String("Left") }),
        FDockNode::CreateTabs({ String("Right") })));

    return Area;
}

static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
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
            ++Count;
        }
    }

    return Count;
}

static const FDrawCommand* FindCommand(const FDrawCommandList& CommandList, EDrawCommandType Type, const FRectangle& Bounds)
{
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == Type &&
            Command.Bounds.Position.X == Bounds.Position.X &&
            Command.Bounds.Position.Y == Bounds.Position.Y &&
            Command.Bounds.Width == Bounds.Width &&
            Command.Bounds.Height == Bounds.Height)
        {
            return &Command;
        }
    }

    return nullptr;
}

static TSharedPtr<FSplitter> FindSplitter(const TSharedPtr<FDockingArea>& Area)
{
    TArray<TSharedPtr<FVisualElement>> AreaChildren;
    Area->GetChildren(AreaChildren);

    if (AreaChildren.IsEmpty())
    {
        return nullptr;
    }

    TArray<TSharedPtr<FVisualElement>> OutsetChildren;
    AreaChildren[0]->GetChildren(OutsetChildren);

    return OutsetChildren.IsEmpty() ? nullptr : StaticCastSharedPtr<FSplitter>(OutsetChildren[0]);
}

static uint32 BackdropClearColor()
{
    return FUIStyle::GetDefault().Colors.WindowBackground.ToColor().ToPackedRGBA();
}

bool PanelChromeGeometry_Test()
{
    TEST_BEGIN();

    const FUIStyle&             Style = FUIStyle::GetDefault();
    const TSharedPtr<IFontFace> Font  = CreateFont();

    TSharedPtr<FDockingArea> Area = BuildPairArea(Font);
    LayoutElement(Area, FRectangle(IntVector2(0, 0), 800, 400));

    FDrawCommandList CommandList;
    DrawElement(Area, CommandList);

    TEST_SECTION("Every leaf carries one rounded fill and one thin outline, so a panel reads as its own card");

    int32 RoundedFills    = 0;
    int32 RoundedOutlines = 0;

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.CornerRadius.IsZero())
        {
            continue;
        }

        if (Command.Type == EDrawCommandType::Box && Command.Tint == Style.Panel.Fill)
        {
            ++RoundedFills;
        }
        else if (Command.Type == EDrawCommandType::BoxOutline && Command.Tint == Style.Panel.Border)
        {
            ++RoundedOutlines;
            TEST_EXPECT(Command.Thickness == Style.Panel.BorderThickness);
        }
    }

    TEST_EXPECT_EQ(RoundedFills, 2);
    TEST_EXPECT_EQ(RoundedOutlines, 2);

    TEST_SECTION("The radius the chrome uses is the one the style names");

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::BoxOutline && Command.Tint == Style.Panel.Border)
        {
            TEST_EXPECT(Command.CornerRadius.TopLeft == Style.Panel.CornerRadius);
            TEST_EXPECT(Command.CornerRadius.BottomRight == Style.Panel.CornerRadius);
        }
    }

    TEST_END();
}

bool PanelChromeGap_Test()
{
    TEST_BEGIN();

    const FUIStyle&             Style  = FUIStyle::GetDefault();
    const TSharedPtr<IFontFace> Font   = CreateFont();
    const FRectangle            Bounds = FRectangle(IntVector2(0, 0), 800, 400);

    TSharedPtr<FDockingArea> Area = BuildPairArea(Font);
    LayoutElement(Area, Bounds);

    FDrawCommandList CommandList;
    DrawElement(Area, CommandList);

    TEST_SECTION("The backdrop is painted once across the whole area and nothing else covers it edge to edge");

    const FDrawCommand* Backdrop = FindCommand(CommandList, EDrawCommandType::Box, Area->GetContentRectangle());
    TEST_EXPECT(Backdrop != nullptr);
    if (Backdrop)
    {
        TEST_EXPECT(Backdrop->Tint == Style.Colors.WindowBackground);
        TEST_EXPECT(Backdrop->CornerRadius.IsZero());
    }

    TEST_SECTION("The gutter between two leaves is as wide as the style's gap, so the backdrop shows between the cards");

    TSharedPtr<FSplitter> Splitter = FindSplitter(Area);
    TEST_EXPECT(Splitter != nullptr);

    if (Splitter)
    {
        const FRectangle Handle = Splitter->GetHandleRectangle(0);
        TEST_EXPECT_EQ(Handle.Width, Style.Panel.Gap);
        TEST_EXPECT(Handle.Height == Bounds.Height - (2 * Style.Panel.Gap));

        TEST_SECTION("Neither card reaches into that gutter");

        TArray<TSharedPtr<FVisualElement>> Children;
        Splitter->GetChildren(Children);
        TEST_EXPECT_EQ(Children.Size(), 2);

        if (Children.Size() == 2)
        {
            TEST_EXPECT(Children[0]->GetContentRectangle().GetRight() <= Handle.Position.X);
            TEST_EXPECT(Children[1]->GetContentRectangle().Position.X >= Handle.GetRight());
        }
    }

    TEST_END();
}

bool SplitterHintThickness_Test()
{
    TEST_BEGIN();

    const FUIStyle&             Style  = FUIStyle::GetDefault();
    const TSharedPtr<IFontFace> Font   = CreateFont();
    const FRectangle            Bounds = FRectangle(IntVector2(0, 0), 800, 400);

    TSharedPtr<FDockingArea> Area = BuildPairArea(Font);
    LayoutElement(Area, Bounds);

    TSharedPtr<FSplitter> Splitter = FindSplitter(Area);
    TEST_EXPECT(Splitter != nullptr);

    if (!Splitter)
    {
        TEST_END();
    }

    TEST_SECTION("A splitter at rest paints no hint at all, so the gutter reads as plain backdrop");

    {
        FDrawCommandList CommandList;
        DrawElement(Area, CommandList);

        const int32 HintCount = CountCommands(CommandList, EDrawCommandType::Box);

        Splitter->OnMouseMove(FCursorEvent(EInputEventType::MouseMoved, Splitter->GetHandleRectangle(0).GetCenter(), IntVector2(0, 0), FModifierKeyState()));

        FDrawCommandList HoveredList;
        DrawElement(Area, HoveredList);

        TEST_SECTION("Hovering adds exactly one hint");
        TEST_EXPECT_EQ(CountCommands(HoveredList, EDrawCommandType::Box), HintCount + 1);

        TEST_SECTION("That hint is a thin line centered in the gutter rather than the whole gutter");

        const FRectangle Handle = Splitter->GetHandleRectangle(0);

        const FDrawCommand* Hint = nullptr;
        for (const FDrawCommand& Command : HoveredList.GetCommands())
        {
            if (Command.Type == EDrawCommandType::Box && Command.Tint == Style.Colors.SeparatorHovered)
            {
                Hint = &Command;
            }
        }

        TEST_EXPECT(Hint != nullptr);
        if (Hint)
        {
            TEST_EXPECT(Hint->Bounds.Width < Handle.Width);
            TEST_EXPECT(Hint->Bounds.Width > 0);

            const int32 LeftGap  = Hint->Bounds.Position.X - Handle.Position.X;
            const int32 RightGap = Handle.GetRight() - Hint->Bounds.GetRight();
            TEST_EXPECT(Math::Abs(LeftGap - RightGap) <= 1);
        }
    }

    TEST_END();
}

bool TabStripBlendsIntoPanel_Test()
{
    TEST_BEGIN();

    const FUIStyle& Style = FUIStyle::GetDefault();

    TEST_SECTION("The strip fill is transparent, so the panel's own rounded fill shows through its top corners");
    TEST_EXPECT(Style.Tab.StripFill.A == 0.0f);

    TEST_SECTION("A strip therefore paints no opaque box of its own across the top of a card");

    const TSharedPtr<IFontFace> Font = CreateFont();

    FTabStrip::FDesc StripDesc;
    StripDesc.Font  = Font;
    StripDesc.Style = Style.Tab;

    TSharedPtr<FTabStrip> Strip = FTabStrip::Create(StripDesc);
    Strip->AddTab("One", "One", true);
    Strip->SetActiveTab("One");

    LayoutElement(Strip, FRectangle(IntVector2(0, 0), 400, Style.Tab.StripHeight));

    FDrawCommandList CommandList;
    DrawElement(Strip, CommandList);

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        const bool bCoversWholeStrip = Command.Bounds.Width >= 400 && Command.Bounds.Height >= Style.Tab.StripHeight;
        if (Command.Type == EDrawCommandType::Box && bCoversWholeStrip)
        {
            TEST_EXPECT(Command.Tint.A == 0.0f);
        }
    }

    TEST_END();
}

bool PanelChromeFocusStroke_Test()
{
    TEST_BEGIN();

    const FUIStyle& Style = FUIStyle::GetDefault();

    TEST_SECTION("The focused stroke is a colour of its own, so a focused card can be told apart at all");
    TEST_EXPECT(!(Style.Panel.BorderFocused == Style.Panel.Border));

    TEST_SECTION("With focus nowhere near the area, every card carries the resting stroke");

    const TSharedPtr<IFontFace> Font = CreateFont();

    TSharedPtr<FDockingArea> Area = BuildPairArea(Font);
    LayoutElement(Area, FRectangle(IntVector2(0, 0), 800, 400));

    FDrawCommandList CommandList;
    DrawElement(Area, CommandList);

    int32 RestingStrokes = 0;
    int32 FocusedStrokes = 0;

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type != EDrawCommandType::BoxOutline || Command.CornerRadius.IsZero())
        {
            continue;
        }

        if (Command.Tint == Style.Panel.Border)
        {
            ++RestingStrokes;
        }
        else if (Command.Tint == Style.Panel.BorderFocused)
        {
            ++FocusedStrokes;
        }
    }

    TEST_EXPECT_EQ(RestingStrokes, 2);
    TEST_EXPECT_EQ(FocusedStrokes, 0);

    TEST_END();
}

bool PanelChromeSnapshots_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font      = CreateFont();
    const String                Directory = GetSnapshotDirectory("Current");

    TEST_SECTION("The snapshot directory can be created");
    TEST_EXPECT(EnsureSnapshotDirectory(Directory));

    const uint32 ClearColor = BackdropClearColor();

    TEST_SECTION("The editor-shaped shell rasterizes and writes");
    {
        TSharedPtr<FDockingArea> Area = BuildShellArea(Font);
        TEST_EXPECT(SnapshotElementToPng(Area, IntVector2(SHELL_WIDTH, SHELL_HEIGHT), ClearColor, Directory + "/Shell.png"));
    }

    TEST_SECTION("A single splitter between two leaves rasterizes and writes");
    {
        TSharedPtr<FDockingArea> Area = BuildPairArea(Font);
        TEST_EXPECT(SnapshotElementToPng(Area, IntVector2(800, 400), ClearColor, Directory + "/Pair.png"));
    }

    TEST_SECTION("A hovered splitter rasterizes and writes");
    {
        TSharedPtr<FDockingArea> Area = BuildPairArea(Font);
        LayoutElement(Area, FRectangle(IntVector2(0, 0), 800, 400));

        if (TSharedPtr<FSplitter> Splitter = FindSplitter(Area))
        {
            Splitter->OnMouseMove(FCursorEvent(EInputEventType::MouseMoved, Splitter->GetHandleRectangle(0).GetCenter(), IntVector2(0, 0), FModifierKeyState()));
        }

        TEST_EXPECT(SnapshotElementToPng(Area, IntVector2(800, 400), ClearColor, Directory + "/SplitterHover.png"));
    }

    TEST_SECTION("A stack of tabs rasterizes and writes");
    {
        FDockingArea::FDesc Desc;
        Desc.Font = Font;

        TSharedPtr<FDockingArea> Area = FDockingArea::Create(Desc);
        Area->RegisterPanel("Output", "Output Log", MakePanelBody("Output Log", Font));
        Area->RegisterPanel("Content", "Content Browser", MakePanelBody("Content Browser", Font));
        Area->RegisterPanel("Console", "Console", MakePanelBody("Console", Font));
        Area->RestoreLayout(FDockNode::CreateTabs({ String("Output"), String("Content"), String("Console") }));

        TEST_EXPECT(SnapshotElementToPng(Area, IntVector2(800, 300), ClearColor, Directory + "/Tabs.png"));
    }

    TEST_END();
}
