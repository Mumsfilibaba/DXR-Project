#include "ToolBarTests.h"
#include "UITestUtils.h"
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
#include <Core/Templates/Utility/NonCopyable.h>

/** @brief Takes the menu stack and the tool tip down while the application is still standing. */
class FScopedToolBarTestServices : public FNonCopyable
{
public:
    FScopedToolBarTestServices() = default;

    ~FScopedToolBarTestServices()
    {
        FMenuStack::Shutdown();
        FToolTipService::Shutdown();
    }
};

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static FUIBrush MakeIcon()
{
    return FUIBrush(reinterpret_cast<FRHITexture*>(0x10));
}

/** @brief A menu of plainly labelled rows, which is all a dropdown needs to have something to open. */
static TSharedPtr<FMenu> CreateMenu(const TSharedPtr<IFontFace>& Font, const TArray<FString>& Labels)
{
    TSharedPtr<FMenu> Menu = FMenu::Create();
    for (const FString& Label : Labels)
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
    Element->Arrange(Bounds);
}

static FCursorEvent MakeMoveEvent(const FIntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, FIntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const FIntVector2& ClientPosition)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, FIntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

/** @brief Presses and releases in the middle of an element, which is one click end to end. */
static void ClickElement(const TSharedPtr<FVisualElement>& Element)
{
    const FIntVector2 Center = Element->GetContentRectangle().GetCenter();

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

    TSharedPtr<FToolBarButton> Save = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("Save")), FOnClicked());
    TSharedPtr<FToolBarButton> Undo = ToolBar->AddButton(FToolBarItemDesc().SetIcon(MakeIcon()), FOnClicked());
    ToolBar->AddSeparator();
    TSharedPtr<FToolBarButton> Grid = ToolBar->AddToggle(FToolBarItemDesc().SetLabel(TEXT("Grid")), ECheckBoxState::Unchecked, FOnCheckStateChanged());
    TSharedPtr<FMenuAnchor>    View = ToolBar->AddDropDown(FToolBarItemDesc().SetLabel(TEXT("View")), nullptr);

    TEST_SECTION(TEXT("The entries are listed in the order they were added, rules included"));
    TEST_EXPECT_EQ(ToolBar->GetNumItems(), 5);
    TEST_EXPECT(ToolBar->GetItems()[0].Type == EToolBarItemType::Button);
    TEST_EXPECT(ToolBar->GetItems()[2].Type == EToolBarItemType::Separator);
    TEST_EXPECT(ToolBar->GetItems()[3].Type == EToolBarItemType::Toggle);
    TEST_EXPECT(ToolBar->GetItems()[4].Type == EToolBarItemType::DropDown);

    TEST_SECTION(TEXT("A rule is not an entry a caller can reach, and a dropdown is the button rather than its anchor"));
    TEST_EXPECT(ToolBar->GetButton(2) == nullptr);
    TEST_EXPECT_EQ(ToolBar->GetButton(0), Save);
    TEST_EXPECT_EQ(ToolBar->GetItems()[4].Element, StaticCastSharedPtr<FVisualElement>(View));
    TEST_EXPECT_EQ(ToolBar->GetButton(4)->GetLabel(), FString(TEXT("View")));

    TEST_SECTION(TEXT("Entries can be found by the label they show"));
    TEST_EXPECT_EQ(ToolBar->FindButton(TEXT("Grid")), Grid);
    TEST_EXPECT(ToolBar->FindButton(TEXT("Missing")) == nullptr);

    TEST_SECTION(TEXT("An entry is as wide as what it shows, and as tall as a row whatever that is"));
    LayoutElement(ToolBar, FRectangle(FIntVector2(0, 0), 400, 40));

    TEST_EXPECT_EQ(Save->GetCachedDesiredSize(), FIntVector2(52, 24));
    TEST_EXPECT_EQ(Undo->GetCachedDesiredSize(), FIntVector2(28, 24));

    TEST_SECTION(TEXT("A dropdown reserves the room its arrow is drawn in"));
    TEST_EXPECT_EQ(ToolBar->GetButton(4)->GetCachedDesiredSize(), FIntVector2(58, 24));

    TEST_SECTION(TEXT("A word too short to fill an entry is widened to the shared floor, which an icon is spared"));
    TSharedPtr<FToolBar>       Narrow = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Fit    = Narrow->AddButton(FToolBarItemDesc().SetLabel(TEXT("Fit")), FOnClicked());
    TSharedPtr<FToolBarButton> Icon   = Narrow->AddButton(FToolBarItemDesc().SetIcon(MakeIcon()), FOnClicked());

    LayoutElement(Narrow, FRectangle(FIntVector2(0, 0), 400, 40));

    const FUIStyleMetrics& Metrics = FUIStyle::GetDefault().Metrics;
    const int32 LabelFloor = Metrics.ResolveButtonMinWidth(Font.Get(), Fit->GetPadding());

    TEST_EXPECT_EQ(LabelFloor, Font->MeasureWidth(FStringView(Metrics.ButtonMinLabel)) + Fit->GetPadding().GetTotalHorizontal());
    TEST_EXPECT_EQ(Fit->GetCachedDesiredSize().X, LabelFloor);
    TEST_EXPECT_EQ(Save->GetCachedDesiredSize().X, LabelFloor);
    TEST_EXPECT_EQ(Icon->GetCachedDesiredSize().X, 28);

    TEST_SECTION(TEXT("An entry showing both an icon and a label is as wide as the two and the gap between them"));
    TSharedPtr<FToolBar> IconAndLabel = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Both   = IconAndLabel->AddButton(FToolBarItemDesc().SetIcon(MakeIcon()).SetLabel(TEXT("Save")), FOnClicked());

    LayoutElement(IconAndLabel, FRectangle(FIntVector2(0, 0), 400, 40));
    TEST_EXPECT_EQ(Both->GetCachedDesiredSize(), FIntVector2(64, 24));

    TEST_SECTION(TEXT("The strip runs left to right, spaced, inside its own padding"));
    TEST_EXPECT_EQ(Save->GetContentRectangle().Position.X, 4);
    TEST_EXPECT_EQ(Undo->GetContentRectangle().Position.X, 58);

    TEST_EXPECT_EQ(Grid->GetContentRectangle().Position.X, 104);
    TEST_EXPECT_EQ(ToolBar->GetButton(4)->GetContentRectangle().Position.X, 158);

    TEST_SECTION(TEXT("Every entry is the same height and centred, however tall the strip is"));
    TEST_EXPECT_EQ(Save->GetContentRectangle().Height, 24);
    TEST_EXPECT_EQ(Save->GetContentRectangle().Position.Y, 8);
    TEST_EXPECT_EQ(Grid->GetContentRectangle().Position.Y, 8);

    TEST_SECTION(TEXT("A rule stops short of the strip's full height, so it reads as a divider rather than a wall"));
    const FRectangle RuleBounds = ToolBar->GetItems()[2].Element->GetContentRectangle();
    TEST_EXPECT_EQ(RuleBounds.Position.X, 94);
    TEST_EXPECT_EQ(RuleBounds.Position.Y, 5);
    TEST_EXPECT_EQ(RuleBounds.Height, 30);

    TEST_SECTION(TEXT("A rule is drawn heavier than a hairline, and stands further off its neighbours than they do"));
    TEST_EXPECT_EQ(RuleBounds.Width, 2);
    TEST_EXPECT_EQ(RuleBounds.Position.X - Undo->GetContentRectangle().GetRight(), 8);
    TEST_EXPECT_EQ(Grid->GetContentRectangle().Position.X - RuleBounds.GetRight(), 8);

    TEST_SECTION(TEXT("The strip asks for what its entries add up to, plus the gaps and its own padding"));
    TEST_EXPECT_EQ(ToolBar->GetCachedDesiredSize(), FIntVector2(220, 28));

    TEST_SECTION(TEXT("A vertical strip stacks its entries down and gives them all one width"));
    FToolBar::FDesc VerticalDesc;
    VerticalDesc.Font        = Font;
    VerticalDesc.Orientation = EOrientation::Vertical;

    TSharedPtr<FToolBar>       Vertical = FToolBar::Create(VerticalDesc);
    TSharedPtr<FToolBarButton> First    = Vertical->AddButton(FToolBarItemDesc().SetLabel(TEXT("One")), FOnClicked());
    TSharedPtr<FToolBarButton> Second   = Vertical->AddButton(FToolBarItemDesc().SetLabel(TEXT("Two")), FOnClicked());

    LayoutElement(Vertical, FRectangle(FIntVector2(0, 0), 120, 200));

    TEST_EXPECT_EQ(First->GetContentRectangle().Position, FIntVector2(4, 2));
    TEST_EXPECT_EQ(Second->GetContentRectangle().Position, FIntVector2(4, 28));
    TEST_EXPECT_EQ(First->GetContentRectangle().Width, 112);
    TEST_EXPECT_EQ(Second->GetContentRectangle().Width, 112);

    TEST_SECTION(TEXT("Clearing drops the entries and the anchors with them"));
    ToolBar->ClearItems();
    TEST_EXPECT_EQ(ToolBar->GetNumItems(), 0);
    TEST_EXPECT(ToolBar->GetAnchors().IsEmpty());
    TEST_EXPECT(ToolBar->FindButton(TEXT("Grid")) == nullptr);

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
        FToolBarItemDesc().SetLabel(TEXT("Save")).SetToolTipText(TEXT("Save the scene")),
        FOnClicked::CreateLambda([&ClickCount]() { ClickCount++; }));

    TSharedPtr<FToolBarButton> Grid = ToolBar->AddToggle(
        FToolBarItemDesc().SetLabel(TEXT("Grid")),
        ECheckBoxState::Unchecked,
        FOnCheckStateChanged::CreateLambda([&ChangeCount, &ReportedState](ECheckBoxState NewState)
        {
            ChangeCount++;
            ReportedState = NewState;
        }));

    LayoutElement(ToolBar, FRectangle(FIntVector2(0, 0), 400, 40));

    TEST_SECTION(TEXT("A button fires once per click and holds nothing afterwards"));
    ClickElement(Save);
    TEST_EXPECT_EQ(ClickCount, 1);
    TEST_EXPECT(!Save->IsHighlighted());

    TEST_SECTION(TEXT("A toggle latches on the first click and lets go on the second"));
    ClickElement(Grid);
    TEST_EXPECT(Grid->IsChecked());
    TEST_EXPECT(Grid->IsHighlighted());
    TEST_EXPECT_EQ(ChangeCount, 1);
    TEST_EXPECT(ReportedState == ECheckBoxState::Checked);

    ClickElement(Grid);
    TEST_EXPECT(!Grid->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 2);
    TEST_EXPECT(ReportedState == ECheckBoxState::Unchecked);

    TEST_SECTION(TEXT("A state pushed in by a host does not report back out"));
    Grid->SetCheckState(ECheckBoxState::Checked);
    TEST_EXPECT(Grid->IsChecked());
    TEST_EXPECT_EQ(ChangeCount, 2);

    TEST_SECTION(TEXT("A disabled entry ignores the click"));
    Save->SetEnabled(false);
    ClickElement(Save);
    TEST_EXPECT_EQ(ClickCount, 1);
    Save->SetEnabled(true);

    TEST_SECTION(TEXT("An idle entry carries a fill of its own, so a strip of them reads as buttons before it is touched"));
    Grid->SetCheckState(ECheckBoxState::Unchecked);

    // A release leaves the cursor where it was, so both entries are still hovered from the clicks above
    Save->OnMouseLeft(MakeMoveEvent(FIntVector2(400, 400)));
    Grid->OnMouseLeft(MakeMoveEvent(FIntVector2(400, 400)));

    const FUIStyle& Style = FUIStyle::GetDefault();

    FDrawCommandList IdleList;
    DrawElement(ToolBar, IdleList);

    TEST_EXPECT_EQ(CountCommands(IdleList, EDrawCommandType::Box), 3);
    TEST_EXPECT_EQ(CountCommands(IdleList, EDrawCommandType::Text), 2);
    TEST_EXPECT(FindFillHasTint(IdleList, Save, Style.Colors.ButtonNormal));
    TEST_EXPECT(FindFillHasTint(IdleList, Grid, Style.Colors.ButtonNormal));

    TEST_SECTION(TEXT("Hovering lifts the fill of the entry under the cursor and leaves its neighbours alone"));
    Save->OnMouseEntered(MakeMoveEvent(Save->GetContentRectangle().GetCenter()));

    FDrawCommandList HoveredList;
    DrawElement(ToolBar, HoveredList);
    TEST_EXPECT(FindFillHasTint(HoveredList, Save, Style.Colors.ButtonHovered));
    TEST_EXPECT(FindFillHasTint(HoveredList, Grid, Style.Colors.ButtonNormal));

    TEST_SECTION(TEXT("Latching switches the fill to the accent, and hovering a latched entry brightens it"));
    Save->OnMouseLeft(MakeMoveEvent(FIntVector2(400, 400)));
    Grid->SetCheckState(ECheckBoxState::Checked);

    FDrawCommandList CheckedList;
    DrawElement(ToolBar, CheckedList);
    TEST_EXPECT(FindFillHasTint(CheckedList, Grid, Style.Colors.Accent));

    Grid->OnMouseEntered(MakeMoveEvent(Grid->GetContentRectangle().GetCenter()));

    FDrawCommandList CheckedHoveredList;
    DrawElement(ToolBar, CheckedHoveredList);
    TEST_EXPECT(FindFillHasTint(CheckedHoveredList, Grid, Style.Colors.AccentHovered));

    Grid->OnMouseLeft(MakeMoveEvent(FIntVector2(400, 400)));

    TEST_SECTION(TEXT("A plain button can be lit from outside, which is how a set of them reads as a radio group"));
    TEST_EXPECT(!Save->IsHighlighted());

    Save->SetHighlighted(true);
    TEST_EXPECT(Save->IsHighlighted());

    FDrawCommandList LitList;
    DrawElement(ToolBar, LitList);
    TEST_EXPECT(FindFillHasTint(LitList, Save, Style.Colors.Accent));

    Save->SetHighlighted(false);
    TEST_EXPECT(!Save->IsHighlighted());

    TEST_SECTION(TEXT("An entry with a tip asks for one when the cursor arrives and drops it when it leaves"));
    FToolTipService& ToolTips = FToolTipService::Get();

    Save->OnMouseEntered(MakeMoveEvent(Save->GetContentRectangle().GetCenter()));
    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT_EQ(ToolTips.GetOwner(), StaticCastSharedPtr<FVisualElement>(Save));

    Save->OnMouseLeft(MakeMoveEvent(FIntVector2(400, 400)));
    TEST_EXPECT(!ToolTips.IsPending());

    TEST_SECTION(TEXT("An entry with no tip never asks for one"));
    Grid->OnMouseEntered(MakeMoveEvent(Grid->GetContentRectangle().GetCenter()));
    TEST_EXPECT(!ToolTips.IsPending());
    Grid->OnMouseLeft(MakeMoveEvent(FIntVector2(400, 400)));

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
    TSharedPtr<FToolBarButton> Move = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("Move")).SetMinWidth(80), FOnClicked());
    TSharedPtr<FToolBarButton> Turn = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("Turn")).SetMinWidth(60), FOnClicked());
    TSharedPtr<FToolBarButton> Size = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("Size")).SetMinWidth(60), FOnClicked());
    ToolBar->EndGroup();

    ToolBar->BeginGroup();
    TSharedPtr<FToolBarButton> Local = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("Local")).SetMinWidth(70), FOnClicked());
    TSharedPtr<FToolBarButton> World = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("World")).SetMinWidth(70), FOnClicked());
    ToolBar->EndGroup();

    TSharedPtr<FToolBarButton> Loose = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("Loose")), FOnClicked());

    TEST_SECTION(TEXT("Only the outer ends of a group are rounded, so the run reads as one pill"));
    TEST_EXPECT(Move->GetCornerRadius() == FCornerRadii::Left(Radius));
    TEST_EXPECT(Turn->GetCornerRadius().IsZero());
    TEST_EXPECT(Size->GetCornerRadius() == FCornerRadii::Right(Radius));

    TEST_SECTION(TEXT("A pair rounds both of its entries, since neither of them is in the middle"));
    TEST_EXPECT(Local->GetCornerRadius() == FCornerRadii::Left(Radius));
    TEST_EXPECT(World->GetCornerRadius() == FCornerRadii::Right(Radius));

    TEST_SECTION(TEXT("An entry outside a group keeps every corner rounded"));
    TEST_EXPECT(Loose->GetCornerRadius() == FCornerRadii(Radius));

    TEST_SECTION(TEXT("A minimum width is honoured however narrow the label is"));
    LayoutElement(ToolBar, FRectangle(FIntVector2(0, 0), 600, 40));

    TEST_EXPECT_EQ(Move->GetContentRectangle().Width, 80);
    TEST_EXPECT_EQ(Turn->GetContentRectangle().Width, 60);
    TEST_EXPECT_EQ(Loose->GetContentRectangle().Width, 52);

    TEST_SECTION(TEXT("A group's entries sit flush, and the gap only comes back between groups"));
    TEST_EXPECT_EQ(Turn->GetContentRectangle().Position.X, Move->GetContentRectangle().GetRight());
    TEST_EXPECT_EQ(Size->GetContentRectangle().Position.X, Turn->GetContentRectangle().GetRight());
    TEST_EXPECT_EQ(World->GetContentRectangle().Position.X, Local->GetContentRectangle().GetRight());

    TEST_EXPECT_EQ(Local->GetContentRectangle().Position.X - Size->GetContentRectangle().GetRight(), 8);
    TEST_EXPECT_EQ(Loose->GetContentRectangle().Position.X - World->GetContentRectangle().GetRight(), 8);

    TEST_SECTION(TEXT("A fixed-width entry centres its label rather than leaving it against the leading edge"));
    FDrawCommandList CommandList;
    DrawElement(ToolBar, CommandList);

    int32 MoveLabelLeft = -1;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Text && CommandList.GetCommandText(Command) == FStringView(TEXT("Move")))
        {
            MoveLabelLeft = Command.Bounds.Position.X;
        }
    }

    TEST_EXPECT_EQ(MoveLabelLeft, Move->GetContentRectangle().Position.X + 24);

    TEST_SECTION(TEXT("Clearing the bar closes whatever group was open, so the next build starts clean"));
    ToolBar->BeginGroup();
    ToolBar->ClearItems();

    ToolBar->BeginGroup();
    TSharedPtr<FToolBarButton> Rebuilt = ToolBar->AddButton(FToolBarItemDesc().SetLabel(TEXT("One")), FOnClicked());
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

    TEST_SECTION(TEXT("A field given a share of the leftover width is stretched well past what its hint asks for"));

    FSearchBox::FDesc SearchDesc;
    SearchDesc.HintText = TEXT("Filter");
    SearchDesc.Font     = Font;

    TSharedPtr<FSearchBox> SearchBox = FSearchBox::Create(SearchDesc);

    TSharedPtr<FToolBar>       Filtered = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Clear    = Filtered->AddButton(FToolBarItemDesc().SetLabel(TEXT("Clear")), FOnClicked());

    Filtered->AddWidget(SearchBox, 1.0f);
    Filtered->AddFlexibleSpace();

    TEST_EXPECT_EQ(Filtered->GetNumItems(), 3);
    TEST_EXPECT(Filtered->GetItems()[1].Type == EToolBarItemType::Custom);
    TEST_EXPECT(Filtered->GetItems()[2].Type == EToolBarItemType::FlexibleSpace);

    LayoutElement(Filtered, FRectangle(FIntVector2(0, 0), 400, 40));

    const FIntVector2 Intrinsic = SearchBox->GetCachedDesiredSize();
    const FRectangle  Field     = SearchBox->GetContentRectangle();

    TEST_EXPECT(Intrinsic.X > 0);
    TEST_EXPECT(Field.Width > Intrinsic.X);

    TEST_SECTION(TEXT("A sized entry beside it keeps the width it asked for"));
    TEST_EXPECT_EQ(Clear->GetContentRectangle().Width, Clear->GetCachedDesiredSize().X);

    TEST_SECTION(TEXT("Two spaces either side of a group leave it the same distance from each of its neighbours"));

    TSharedPtr<FToolBar>       Centred = FToolBar::Create(Desc);
    TSharedPtr<FToolBarButton> Head    = Centred->AddButton(FToolBarItemDesc().SetLabel(TEXT("Head")), FOnClicked());

    Centred->AddFlexibleSpace();

    TSharedPtr<FToolBarButton> Play  = Centred->AddButton(FToolBarItemDesc().SetLabel(TEXT("Play")), FOnClicked());
    TSharedPtr<FToolBarButton> Pause = Centred->AddButton(FToolBarItemDesc().SetLabel(TEXT("Pause")), FOnClicked());

    Centred->AddFlexibleSpace();

    TSharedPtr<FToolBarButton> Tail = Centred->AddButton(FToolBarItemDesc().SetLabel(TEXT("Tail")), FOnClicked());

    LayoutElement(Centred, FRectangle(FIntVector2(0, 0), 500, 40));

    const int32 LeadingGap  = Play->GetContentRectangle().Position.X - Head->GetContentRectangle().GetRight();
    const int32 TrailingGap = Tail->GetContentRectangle().Position.X - Pause->GetContentRectangle().GetRight();

    TEST_EXPECT(LeadingGap > 0);
    TEST_EXPECT(Math::Abs(LeadingGap - TrailingGap) <= 1);

    const int32 TransportLeft   = Play->GetContentRectangle().Position.X;
    const int32 TransportRight  = Pause->GetContentRectangle().GetRight();
    const int32 TransportCentre = (TransportLeft + TransportRight) / 2;

    TEST_EXPECT(Math::Abs(TransportCentre - 250) <= 1);

    TEST_SECTION(TEXT("The trailing entry is pushed against the far edge, inside the strip's own padding"));
    TEST_EXPECT_EQ(Tail->GetContentRectangle().GetRight(), 500 - 4);

    TEST_SECTION(TEXT("A stretched field still reaches the draw data, rather than collapsing to nothing"));

    FDrawCommandList CommandList;
    DrawElement(Filtered, CommandList);

    TEST_EXPECT(CountCommands(CommandList, EDrawCommandType::Box) >= 2);
    TEST_EXPECT(CountCommands(CommandList, EDrawCommandType::BoxOutline) >= 1);
    TEST_EXPECT_EQ(CountCommands(CommandList, EDrawCommandType::Text), 2);

    FUIDrawData DrawData;
    DrawData.BuildFromCommandList(CommandList);

    TEST_EXPECT(!DrawData.IsEmpty());

    TEST_SECTION(TEXT("The field's own background survives the clip and the layer sort intact"));

    const FVector2 FieldCentre(
        (static_cast<float>(Field.Position.X) + static_cast<float>(Field.GetRight())) * 0.5f,
        (static_cast<float>(Field.Position.Y) + static_cast<float>(Field.GetBottom())) * 0.5f);

    bool bFoundField = false;
    for (const FUIShapeVertex& Vertex : GetShapeVertices(DrawData))
    {
        const FVector2 Origin = Vertex.Position - Vertex.LocalPos;
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
    TSharedPtr<FWindow>         Window = Application.CreateWindow(FIntVector2(800, 600), FIntVector2(0, 0));

    FToolBar::FDesc Desc;
    Desc.Font           = Font;
    Desc.bHasBackground = false;

    TSharedPtr<FToolBar>    ToolBar    = FToolBar::Create(Desc);
    TSharedPtr<FMenuAnchor> ViewAnchor = ToolBar->AddDropDown(FToolBarItemDesc().SetLabel(TEXT("View")), CreateMenu(Font, { TEXT("Wireframe"), TEXT("Lit") }));
    TSharedPtr<FMenuAnchor> SnapAnchor = ToolBar->AddDropDown(FToolBarItemDesc().SetLabel(TEXT("Snap")), CreateMenu(Font, { TEXT("Ten"), TEXT("Twenty") }));

    Window->SetContent(ToolBar);
    FApplication::LayoutWindow(Window);

    TSharedPtr<FToolBarButton> ViewButton = ToolBar->GetButton(0);
    TSharedPtr<FToolBarButton> SnapButton = ToolBar->GetButton(1);

    TEST_SECTION(TEXT("Nothing is open until a dropdown is clicked"));
    TEST_EXPECT(!ToolBar->IsAnyMenuOpen());
    TEST_EXPECT(!ViewButton->IsHighlighted());

    TEST_SECTION(TEXT("Clicking one opens its menu directly under the entry"));
    ClickElement(ViewButton);
    TEST_EXPECT(ViewAnchor->IsOpen());
    TEST_EXPECT(ToolBar->IsAnyMenuOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);

    const FRectangle ButtonBounds = ViewButton->GetContentRectangle();
    TEST_EXPECT_EQ(ViewAnchor->GetMenu()->ScreenBounds.Position, FIntVector2(ButtonBounds.Position.X, ButtonBounds.GetBottom()));

    TEST_SECTION(TEXT("The open entry stays lit, so it is clear which menu belongs to which entry"));
    TEST_EXPECT(ViewButton->IsHighlighted());
    TEST_EXPECT(!SnapButton->IsHighlighted());

    TEST_SECTION(TEXT("With one open, hovering a sibling switches to it rather than stacking a second"));
    SnapButton->OnMouseEntered(MakeMoveEvent(SnapButton->GetContentRectangle().GetCenter()));
    TEST_EXPECT(!ViewAnchor->IsOpen());
    TEST_EXPECT(SnapAnchor->IsOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 1);

    TEST_SECTION(TEXT("Clicking the open entry closes it again"));
    ClickElement(SnapButton);
    TEST_EXPECT(!SnapAnchor->IsOpen());
    TEST_EXPECT(!ToolBar->IsAnyMenuOpen());

    TEST_SECTION(TEXT("A dropdown draws an arrow, and only a dropdown does"));
    FDrawCommandList CommandList;
    DrawElement(ToolBar, CommandList);
    TEST_EXPECT_EQ(CountCommands(CommandList, EDrawCommandType::ConvexPolygon), 2);

    TEST_SECTION(TEXT("Closing the bar takes whatever it had open down with it"));
    ViewAnchor->Open();
    TEST_EXPECT(ToolBar->IsAnyMenuOpen());

    ToolBar->CloseActiveMenu();
    TEST_EXPECT(!ToolBar->IsAnyMenuOpen());
    TEST_EXPECT_EQ(FMenuStack::Get().GetDepth(), 0);

    TEST_END();
}
