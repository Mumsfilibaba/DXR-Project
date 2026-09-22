#include "ToolBarTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Math/Math.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Elements/SearchBox.h>
#include <Application/Elements/ToolBar.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Menus/MenuStack.h>
#include <Application/Menus/ToolTipService.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

/** @brief An eight by sixteen face, so every measurement in these tests is exact. */
static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

/** @brief A brush over a texture that is never sampled, because nothing here rasterizes. */
static FUIBrush MakeIcon()
{
    return FUIBrush(reinterpret_cast<FRHITexture*>(0x10));
}

/** @brief Takes the menu stack and the tool tip down while the application is still standing. */
class FScopedToolBarTestServices
{
public:
    FScopedToolBarTestServices() = default;

    ~FScopedToolBarTestServices()
    {
        FMenuStack::Shutdown();
        FToolTipService::Shutdown();
    }

    FScopedToolBarTestServices(const FScopedToolBarTestServices&) = delete;
    FScopedToolBarTestServices& operator=(const FScopedToolBarTestServices&) = delete;
};

/** @brief A menu of plainly labelled rows, which is all a dropdown needs to have something to open. */
static TSharedPtr<FMenu> CreateMenu(const TSharedPtr<IFontFace>& Font, const TArray<String>& Labels)
{
    TSharedPtr<FMenu> Menu = FMenu::Create();
    for (const String& Label : Labels)
    {
        FMenuItem::FDesc Desc;
        Desc.SetLabel(Label).SetFont(Font);

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    return Menu;
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

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

/** @brief Presses and releases in the middle of an element, which is one click end to end. */
static void ClickElement(const TSharedPtr<FVisualElement>& Element)
{
    const IntVector2 Center = Element->GetContentRectangle().GetCenter();

    Element->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, Center));
    Element->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, Center));
}

/** @brief Counts how many commands of a type the list holds. */
static int32 CountCommands(const FDrawCommandList& CommandList, EDrawCommandType Type)
{
    int32 Count = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        Count += Command.Type == Type ? 1 : 0;
    }

    return Count;
}

/** @brief Draws an element that has already been arranged. */
static void DrawElement(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList)
{
    Element->OnDraw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), OutCommandList, 0);
}

static bool FindFillHasTint(const FDrawCommandList& CommandList, const TSharedPtr<FVisualElement>& Element, const FFloatColor& Tint)
{
    const FRectangle Bounds = Element->GetContentRectangle();

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Bounds == Bounds)
        {
            return Command.HasTint(Tint);
        }
    }

    return false;
}

bool ToolBarComposition_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FToolBar::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);

    TSharedPtr<FToolBarButton> Save = ToolBar->AddButton(FToolBarItemDesc().SetLabel("Save"), FOnClicked());
    TSharedPtr<FToolBarButton> Undo = ToolBar->AddButton(FToolBarItemDesc().SetIcon(MakeIcon()), FOnClicked());
    ToolBar->AddSeparator();
    TSharedPtr<FToolBarButton> Grid = ToolBar->AddToggle(FToolBarItemDesc().SetLabel("Grid"), ECheckBoxState::Unchecked, FOnCheckStateChanged());
    TSharedPtr<FMenuAnchor>    View = ToolBar->AddDropDown(FToolBarItemDesc().SetLabel("View"), nullptr);

    TEST_SECTION("The entries are listed in the order they were added, rules included");
    TEST_EXPECT_EQ(ToolBar->GetNumItems(), 5);
    TEST_EXPECT(ToolBar->GetItems()[0].Type == EToolBarItemType::Button);
    TEST_EXPECT(ToolBar->GetItems()[2].Type == EToolBarItemType::Separator);
    TEST_EXPECT(ToolBar->GetItems()[3].Type == EToolBarItemType::Toggle);
    TEST_EXPECT(ToolBar->GetItems()[4].Type == EToolBarItemType::DropDown);

    TEST_SECTION("A rule is not an entry a caller can reach, and a dropdown is the button rather than its anchor");
    TEST_EXPECT(ToolBar->GetButton(2) == nullptr);
    TEST_EXPECT_EQ(ToolBar->GetButton(0), Save);
    TEST_EXPECT_EQ(ToolBar->GetItems()[4].Element, StaticCastSharedPtr<FVisualElement>(View));
    TEST_EXPECT_EQ(ToolBar->GetButton(4)->GetLabel(), String("View"));

    TEST_SECTION("Entries can be found by the label they show");
    TEST_EXPECT_EQ(ToolBar->FindButton("Grid"), Grid);
    TEST_EXPECT(ToolBar->FindButton("Missing") == nullptr);

    TEST_SECTION("An entry is as wide as what it shows, and as tall as a row whatever that is");
    LayoutElement(ToolBar, FRectangle(IntVector2(0, 0), 400, 40));

    TEST_EXPECT_EQ(Save->GetCachedDesiredSize(), IntVector2(52, 24));
    TEST_EXPECT_EQ(Undo->GetCachedDesiredSize(), IntVector2(28, 24));

    TEST_SECTION("A dropdown reserves the room its arrow is drawn in");
    TEST_EXPECT_EQ(ToolBar->GetButton(4)->GetCachedDesiredSize(), IntVector2(58, 24));

    TEST_SECTION("A word too short to fill an entry is widened to the shared floor, which an icon is spared");
    TSharedPtr<FToolBar>       Narrow = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Fit    = Narrow->AddButton(FToolBarItemDesc().SetLabel("Fit"), FOnClicked());
    TSharedPtr<FToolBarButton> Icon   = Narrow->AddButton(FToolBarItemDesc().SetIcon(MakeIcon()), FOnClicked());

    LayoutElement(Narrow, FRectangle(IntVector2(0, 0), 400, 40));

    const FUIStyleMetrics& Metrics = FUIStyle::GetDefault().Metrics;
    const int32 LabelFloor = Metrics.ResolveButtonMinWidth(Font.Get(), Fit->GetPadding());

    TEST_EXPECT_EQ(LabelFloor, Font->MeasureWidth(StringView(Metrics.ButtonMinLabel)) + Fit->GetPadding().GetTotalHorizontal());
    TEST_EXPECT_EQ(Fit->GetCachedDesiredSize().X, LabelFloor);
    TEST_EXPECT_EQ(Save->GetCachedDesiredSize().X, LabelFloor);
    TEST_EXPECT_EQ(Icon->GetCachedDesiredSize().X, 28);

    TEST_SECTION("An entry showing both an icon and a label is as wide as the two and the gap between them");
    TSharedPtr<FToolBar> IconAndLabel = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Both   = IconAndLabel->AddButton(FToolBarItemDesc().SetIcon(MakeIcon()).SetLabel("Save"), FOnClicked());

    LayoutElement(IconAndLabel, FRectangle(IntVector2(0, 0), 400, 40));
    TEST_EXPECT_EQ(Both->GetCachedDesiredSize(), IntVector2(64, 24));

    TEST_SECTION("The strip runs left to right, spaced, inside its own padding");
    TEST_EXPECT_EQ(Save->GetContentRectangle().Position.X, 4);
    TEST_EXPECT_EQ(Undo->GetContentRectangle().Position.X, 58);

    TEST_EXPECT_EQ(Grid->GetContentRectangle().Position.X, 104);
    TEST_EXPECT_EQ(ToolBar->GetButton(4)->GetContentRectangle().Position.X, 158);

    TEST_SECTION("Every entry is the same height and centred, however tall the strip is");
    TEST_EXPECT_EQ(Save->GetContentRectangle().Height, 24);
    TEST_EXPECT_EQ(Save->GetContentRectangle().Position.Y, 8);
    TEST_EXPECT_EQ(Grid->GetContentRectangle().Position.Y, 8);

    TEST_SECTION("A rule stops short of the strip's full height, so it reads as a divider rather than a wall");
    const FRectangle RuleBounds = ToolBar->GetItems()[2].Element->GetContentRectangle();
    TEST_EXPECT_EQ(RuleBounds.Position.X, 94);
    TEST_EXPECT_EQ(RuleBounds.Position.Y, 5);
    TEST_EXPECT_EQ(RuleBounds.Height, 30);

    TEST_SECTION("A rule is drawn heavier than a hairline, and stands further off its neighbours than they do");
    TEST_EXPECT_EQ(RuleBounds.Width, 2);
    TEST_EXPECT_EQ(RuleBounds.Position.X - Undo->GetContentRectangle().GetRight(), 8);
    TEST_EXPECT_EQ(Grid->GetContentRectangle().Position.X - RuleBounds.GetRight(), 8);

    TEST_SECTION("The strip asks for what its entries add up to, plus the gaps and its own padding");
    TEST_EXPECT_EQ(ToolBar->GetCachedDesiredSize(), IntVector2(220, 28));

    TEST_SECTION("A vertical strip stacks its entries down and gives them all one width");
    FToolBar::FDesc VerticalDesc;
    VerticalDesc.Font        = Font;
    VerticalDesc.Orientation = EOrientation::Vertical;

    TSharedPtr<FToolBar>       Vertical = FToolBar::Create(VerticalDesc);
    TSharedPtr<FToolBarButton> First    = Vertical->AddButton(FToolBarItemDesc().SetLabel("One"), FOnClicked());
    TSharedPtr<FToolBarButton> Second   = Vertical->AddButton(FToolBarItemDesc().SetLabel("Two"), FOnClicked());

    LayoutElement(Vertical, FRectangle(IntVector2(0, 0), 120, 200));

    TEST_EXPECT_EQ(First->GetContentRectangle().Position, IntVector2(4, 2));
    TEST_EXPECT_EQ(Second->GetContentRectangle().Position, IntVector2(4, 28));
    TEST_EXPECT_EQ(First->GetContentRectangle().Width, 112);
    TEST_EXPECT_EQ(Second->GetContentRectangle().Width, 112);

    TEST_SECTION("Clearing drops the entries and the anchors with them");
    ToolBar->ClearItems();
    TEST_EXPECT_EQ(ToolBar->GetNumItems(), 0);
    TEST_EXPECT(ToolBar->GetAnchors().IsEmpty());
    TEST_EXPECT(ToolBar->FindButton("Grid") == nullptr);

    TEST_END();
}

bool ToolBarInteraction_Test()
{
    TEST_BEGIN();

    FScopedStubApplication     Application;
    FScopedToolBarTestServices MenuServices;

    const TSharedPtr<IFontFace> Font = CreateFont();

    int32          ClickCount   = 0;
    int32          ChangeCount  = 0;
    ECheckBoxState ReportedState = ECheckBoxState::Undetermined;

    FToolBar::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);

    TSharedPtr<FToolBarButton> Save = ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Save").SetToolTipText("Save the scene"),
        FOnClicked::CreateLambda([&ClickCount]() { ClickCount++; }));

    TSharedPtr<FToolBarButton> Grid = ToolBar->AddToggle(
        FToolBarItemDesc().SetLabel("Grid"),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([&ChangeCount, &ReportedState](ECheckBoxState NewState)
        {
            ChangeCount++;
            ReportedState = NewState;
        }));

    LayoutElement(ToolBar, FRectangle(IntVector2(0, 0), 400, 40));

    TEST_SECTION("A button fires once per click and holds nothing afterwards");
    ClickElement(Save);
    TEST_EXPECT_EQ(ClickCount, 1);
    TEST_EXPECT(!Save->IsHighlighted());

    TEST_SECTION("A toggle latches on the first click and lets go on the second");
    ClickElement(Grid);
    TEST_EXPECT(Grid->IsChecked());
    TEST_EXPECT(Grid->IsHighlighted());
    TEST_EXPECT_EQ(ChangeCount, 1);
    TEST_EXPECT(ReportedState == ECheckBoxState::Checked);

    ClickElement(Grid);
    TEST_EXPECT(!Grid->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 2);
    TEST_EXPECT(ReportedState == ECheckBoxState::Unchecked);

    TEST_SECTION("A state pushed in by a host does not report back out");
    Grid->SetCheckState(ECheckBoxState::Checked);
    TEST_EXPECT(Grid->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 2);

    TEST_SECTION("A disabled entry ignores the click");
    Save->SetEnabled(false);
    ClickElement(Save);
    TEST_EXPECT_EQ(ClickCount, 1);
    Save->SetEnabled(true);

    TEST_SECTION("An idle entry carries a fill of its own, so a strip of them reads as buttons before it is touched");
    Grid->SetCheckState(ECheckBoxState::Unchecked);

    // A release leaves the cursor where it was, so both entries are still hovered from the clicks above
    Save->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    Grid->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    const FUIStyle& Style = FUIStyle::GetDefault();

    FDrawCommandList IdleList;
    DrawElement(ToolBar, IdleList);

    TEST_EXPECT_EQ(CountCommands(IdleList, EDrawCommandType::Box), 3);
    TEST_EXPECT_EQ(CountCommands(IdleList, EDrawCommandType::Text), 2);
    TEST_EXPECT(FindFillHasTint(IdleList, Save, Style.Colors.ButtonNormal));
    TEST_EXPECT(FindFillHasTint(IdleList, Grid, Style.Colors.ButtonNormal));

    TEST_SECTION("Hovering lifts the fill of the entry under the cursor and leaves its neighbours alone");
    Save->OnMouseEntered(MakeMoveEvent(Save->GetContentRectangle().GetCenter()));

    FDrawCommandList HoveredList;
    DrawElement(ToolBar, HoveredList);
    TEST_EXPECT(FindFillHasTint(HoveredList, Save, Style.Colors.ButtonHovered));
    TEST_EXPECT(FindFillHasTint(HoveredList, Grid, Style.Colors.ButtonNormal));

    TEST_SECTION("Latching switches the fill to the accent, and hovering a latched entry brightens it");
    Save->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    Grid->SetCheckState(ECheckBoxState::Checked);

    FDrawCommandList CheckedList;
    DrawElement(ToolBar, CheckedList);
    TEST_EXPECT(FindFillHasTint(CheckedList, Grid, Style.Colors.Accent));

    Grid->OnMouseEntered(MakeMoveEvent(Grid->GetContentRectangle().GetCenter()));

    FDrawCommandList CheckedHoveredList;
    DrawElement(ToolBar, CheckedHoveredList);
    TEST_EXPECT(FindFillHasTint(CheckedHoveredList, Grid, Style.Colors.AccentHovered));

    Grid->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    TEST_SECTION("A plain button can be lit from outside, which is how a set of them reads as a radio group");
    TEST_EXPECT(!Save->IsHighlighted());

    Save->SetHighlighted(true);
    TEST_EXPECT(Save->IsHighlighted());

    FDrawCommandList LitList;
    DrawElement(ToolBar, LitList);
    TEST_EXPECT(FindFillHasTint(LitList, Save, Style.Colors.Accent));

    Save->SetHighlighted(false);
    TEST_EXPECT(!Save->IsHighlighted());

    TEST_SECTION("An entry with a tip asks for one when the cursor arrives and drops it when it leaves");
    FToolTipService& ToolTips = FToolTipService::Get();

    Save->OnMouseEntered(MakeMoveEvent(Save->GetContentRectangle().GetCenter()));
    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT_EQ(ToolTips.GetOwner(), StaticCastSharedPtr<FVisualElement>(Save));

    Save->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    TEST_EXPECT(!ToolTips.IsPending());

    TEST_SECTION("An entry with no tip never asks for one");
    Grid->OnMouseEntered(MakeMoveEvent(Grid->GetContentRectangle().GetCenter()));
    TEST_EXPECT(!ToolTips.IsPending());
    Grid->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    TEST_END();
}

bool ToolBarGroups_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font  = CreateFont();
    const FUIStyle&             Style = FUIStyle::GetDefault();
    const float                 Radius = Style.Metrics.ButtonCornerRadius;

    FToolBar::FDesc Desc;
    Desc.Font        = Font;
    Desc.ItemSpacing = 8;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(Desc);

    ToolBar->BeginGroup();
    TSharedPtr<FToolBarButton> Move = ToolBar->AddButton(FToolBarItemDesc().SetLabel("Move").SetMinWidth(80), FOnClicked());
    TSharedPtr<FToolBarButton> Turn = ToolBar->AddButton(FToolBarItemDesc().SetLabel("Turn").SetMinWidth(60), FOnClicked());
    TSharedPtr<FToolBarButton> Size = ToolBar->AddButton(FToolBarItemDesc().SetLabel("Size").SetMinWidth(60), FOnClicked());
    ToolBar->EndGroup();

    ToolBar->BeginGroup();
    TSharedPtr<FToolBarButton> Local = ToolBar->AddButton(FToolBarItemDesc().SetLabel("Local").SetMinWidth(70), FOnClicked());
    TSharedPtr<FToolBarButton> World = ToolBar->AddButton(FToolBarItemDesc().SetLabel("World").SetMinWidth(70), FOnClicked());
    ToolBar->EndGroup();

    TSharedPtr<FToolBarButton> Loose = ToolBar->AddButton(FToolBarItemDesc().SetLabel("Loose"), FOnClicked());

    TEST_SECTION("Only the outer ends of a group are rounded, so the run reads as one pill");
    TEST_EXPECT(Move->GetCornerRadius() == FCornerRadii::Left(Radius));
    TEST_EXPECT(Turn->GetCornerRadius().IsZero());
    TEST_EXPECT(Size->GetCornerRadius() == FCornerRadii::Right(Radius));

    TEST_SECTION("A pair rounds both of its entries, since neither of them is in the middle");
    TEST_EXPECT(Local->GetCornerRadius() == FCornerRadii::Left(Radius));
    TEST_EXPECT(World->GetCornerRadius() == FCornerRadii::Right(Radius));

    TEST_SECTION("An entry outside a group keeps every corner rounded");
    TEST_EXPECT(Loose->GetCornerRadius() == FCornerRadii(Radius));

    TEST_SECTION("A minimum width is honoured however narrow the label is");
    LayoutElement(ToolBar, FRectangle(IntVector2(0, 0), 600, 40));

    TEST_EXPECT_EQ(Move->GetContentRectangle().Width, 80);
    TEST_EXPECT_EQ(Turn->GetContentRectangle().Width, 60);
    TEST_EXPECT_EQ(Loose->GetContentRectangle().Width, 52);

    TEST_SECTION("A group's entries sit flush, and the gap only comes back between groups");
    TEST_EXPECT_EQ(Turn->GetContentRectangle().Position.X, Move->GetContentRectangle().GetRight());
    TEST_EXPECT_EQ(Size->GetContentRectangle().Position.X, Turn->GetContentRectangle().GetRight());
    TEST_EXPECT_EQ(World->GetContentRectangle().Position.X, Local->GetContentRectangle().GetRight());

    TEST_EXPECT_EQ(Local->GetContentRectangle().Position.X - Size->GetContentRectangle().GetRight(), 8);
    TEST_EXPECT_EQ(Loose->GetContentRectangle().Position.X - World->GetContentRectangle().GetRight(), 8);

    TEST_SECTION("A fixed-width entry centres its label rather than leaving it against the leading edge");
    FDrawCommandList CommandList;
    DrawElement(ToolBar, CommandList);

    int32 MoveLabelLeft = -1;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Text && CommandList.GetCommandText(Command) == StringView("Move"))
        {
            MoveLabelLeft = Command.Bounds.Position.X;
        }
    }

    TEST_EXPECT_EQ(MoveLabelLeft, Move->GetContentRectangle().Position.X + 24);

    TEST_SECTION("Clearing the bar closes whatever group was open, so the next build starts clean");
    ToolBar->BeginGroup();
    ToolBar->ClearItems();

    ToolBar->BeginGroup();
    TSharedPtr<FToolBarButton> Rebuilt = ToolBar->AddButton(FToolBarItemDesc().SetLabel("One"), FOnClicked());
    ToolBar->EndGroup();

    TEST_EXPECT_EQ(ToolBar->GetNumItems(), 1);
    TEST_EXPECT(Rebuilt->GetCornerRadius() == FCornerRadii(Radius));

    TEST_END();
}

bool ToolBarFlexibleSpace_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FToolBar::FDesc Desc;
    Desc.Font = Font;

    TEST_SECTION("A field given a share of the leftover width is stretched well past what its hint asks for");

    FSearchBox::FDesc SearchDesc;
    SearchDesc.HintText = "Filter";
    SearchDesc.Font     = Font;

    TSharedPtr<FSearchBox> SearchBox = FSearchBox::Create(SearchDesc);

    TSharedPtr<FToolBar>       Filtered = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Clear    = Filtered->AddButton(FToolBarItemDesc().SetLabel("Clear"), FOnClicked());

    Filtered->AddWidget(SearchBox, 1.0f);
    Filtered->AddFlexibleSpace();

    TEST_EXPECT_EQ(Filtered->GetNumItems(), 3);
    TEST_EXPECT(Filtered->GetItems()[1].Type == EToolBarItemType::Custom);
    TEST_EXPECT(Filtered->GetItems()[2].Type == EToolBarItemType::FlexibleSpace);

    LayoutElement(Filtered, FRectangle(IntVector2(0, 0), 400, 40));

    const IntVector2 Intrinsic = SearchBox->GetCachedDesiredSize();
    const FRectangle Field     = SearchBox->GetContentRectangle();

    TEST_EXPECT(Intrinsic.X > 0);
    TEST_EXPECT(Field.Width > Intrinsic.X);

    TEST_SECTION("A sized entry beside it keeps the width it asked for");
    TEST_EXPECT_EQ(Clear->GetContentRectangle().Width, Clear->GetCachedDesiredSize().X);

    TEST_SECTION("Two spaces either side of a group leave it the same distance from each of its neighbours");

    TSharedPtr<FToolBar>       Centred = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Head    = Centred->AddButton(FToolBarItemDesc().SetLabel("Head"), FOnClicked());

    Centred->AddFlexibleSpace();

    TSharedPtr<FToolBarButton> Play  = Centred->AddButton(FToolBarItemDesc().SetLabel("Play"), FOnClicked());
    TSharedPtr<FToolBarButton> Pause = Centred->AddButton(FToolBarItemDesc().SetLabel("Pause"), FOnClicked());

    Centred->AddFlexibleSpace();

    TSharedPtr<FToolBarButton> Tail = Centred->AddButton(FToolBarItemDesc().SetLabel("Tail"), FOnClicked());

    LayoutElement(Centred, FRectangle(IntVector2(0, 0), 500, 40));

    const int32 LeadingGap  = Play->GetContentRectangle().Position.X - Head->GetContentRectangle().GetRight();
    const int32 TrailingGap = Tail->GetContentRectangle().Position.X - Pause->GetContentRectangle().GetRight();

    TEST_EXPECT(LeadingGap > 0);
    TEST_EXPECT(Math::Abs(LeadingGap - TrailingGap) <= 1);

    const int32 TransportLeft   = Play->GetContentRectangle().Position.X;
    const int32 TransportRight  = Pause->GetContentRectangle().GetRight();
    const int32 TransportCentre = (TransportLeft + TransportRight) / 2;

    TEST_EXPECT(Math::Abs(TransportCentre - 250) <= 1);

    TEST_SECTION("The trailing entry is pushed against the far edge, inside the strip's own padding");
    TEST_EXPECT_EQ(Tail->GetContentRectangle().GetRight(), 500 - 4);

    TEST_SECTION("A stretched field still reaches the draw data, rather than collapsing to nothing");

    FDrawCommandList CommandList;
    DrawElement(Filtered, CommandList);

    TEST_EXPECT(CountCommands(CommandList, EDrawCommandType::Box) >= 2);
    TEST_EXPECT(CountCommands(CommandList, EDrawCommandType::BoxOutline) >= 1);
    TEST_EXPECT_EQ(CountCommands(CommandList, EDrawCommandType::Text), 2);

    FUIDrawData DrawData;
    DrawData.BuildFromCommandList(CommandList);

    TEST_EXPECT(!DrawData.IsEmpty());

    TEST_SECTION("The field's own background survives the clip and the layer sort intact");

    const Vector2 FieldCentre(
        (static_cast<float>(Field.Position.X) + static_cast<float>(Field.GetRight())) * 0.5f,
        (static_cast<float>(Field.Position.Y) + static_cast<float>(Field.GetBottom())) * 0.5f);

    bool bFoundField = false;
    for (const FUIShapeVertex& Vertex : DrawData.GetShapeVertices())
    {
        const Vector2 Origin = Vertex.Position - Vertex.LocalPos;
        bFoundField |= FieldCentre.X >= Origin.X && FieldCentre.X <= Origin.X + Vertex.RectSize.X
            && FieldCentre.Y >= Origin.Y && FieldCentre.Y <= Origin.Y + Vertex.RectSize.Y;
    }

    if (!bFoundField)
    {
        for (const FUIVertex& Vertex : DrawData.GetVertices())
        {
            bFoundField |= Vertex.Position == FieldCentre;
        }
    }

    TEST_EXPECT(bFoundField);

    TEST_END();
}

bool ToolBarDropDown_Test()
{
    TEST_BEGIN();

    FScopedStubApplication     Application;
    FScopedToolBarTestServices MenuServices;

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    FToolBar::FDesc Desc;
    Desc.Font           = Font;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar>    ToolBar    = FToolBar::Create(Desc);
    TSharedPtr<FMenuAnchor> ViewAnchor = ToolBar->AddDropDown(FToolBarItemDesc().SetLabel("View"), CreateMenu(Font, { "Wireframe", "Lit" }));
    TSharedPtr<FMenuAnchor> SnapAnchor = ToolBar->AddDropDown(FToolBarItemDesc().SetLabel("Snap"), CreateMenu(Font, { "Ten", "Twenty" }));

    Window->SetContent(ToolBar);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FToolBarButton> ViewButton = ToolBar->GetButton(0);
    TSharedPtr<FToolBarButton> SnapButton = ToolBar->GetButton(1);

    TEST_SECTION("Nothing is open until a dropdown is clicked");
    TEST_EXPECT(!ToolBar->IsAnyMenuOpen());
    TEST_EXPECT(!ViewButton->IsHighlighted());

    TEST_SECTION("Clicking one opens its menu directly under the entry");
    ClickElement(ViewButton);
    TEST_EXPECT(ViewAnchor->IsOpen());
    TEST_EXPECT(ToolBar->IsAnyMenuOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);

    const FRectangle ButtonBounds = ViewButton->GetContentRectangle();
    TEST_EXPECT_EQ(ViewAnchor->GetMenu()->ScreenBounds.Position, IntVector2(ButtonBounds.Position.X, ButtonBounds.GetBottom()));

    TEST_SECTION("The open entry stays lit, so it is clear which menu belongs to which entry");
    TEST_EXPECT(ViewButton->IsHighlighted());
    TEST_EXPECT(!SnapButton->IsHighlighted());

    TEST_SECTION("With one open, hovering a sibling switches to it rather than stacking a second");
    SnapButton->OnMouseEntered(MakeMoveEvent(SnapButton->GetContentRectangle().GetCenter()));
    TEST_EXPECT(!ViewAnchor->IsOpen());
    TEST_EXPECT(SnapAnchor->IsOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);

    TEST_SECTION("Clicking the open entry closes it again");
    ClickElement(SnapButton);
    TEST_EXPECT(!SnapAnchor->IsOpen());
    TEST_EXPECT(!ToolBar->IsAnyMenuOpen());

    TEST_SECTION("A dropdown draws an arrow, and only a dropdown does");
    FDrawCommandList CommandList;
    DrawElement(ToolBar, CommandList);
    TEST_EXPECT_EQ(CountCommands(CommandList, EDrawCommandType::ConvexPolygon), 2);

    TEST_SECTION("Closing the bar takes whatever it had open down with it");
    ViewAnchor->Open();
    TEST_EXPECT(ToolBar->IsAnyMenuOpen());

    ToolBar->CloseActiveMenu();
    TEST_EXPECT(!ToolBar->IsAnyMenuOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 0);

    TEST_END();
}
