#include "ItemViewTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/Array.h>
#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/TileView.h>
#include <Application/Elements/TreeView.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/ToolTipService.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

class FScopedItemViewTestServices
{
public:
    FScopedItemViewTestServices() = default;

    ~FScopedItemViewTestServices()
    {
        FToolTipService::Shutdown();
    }

    FScopedItemViewTestServices(const FScopedItemViewTestServices&) = delete;
    FScopedItemViewTestServices& operator=(const FScopedItemViewTestServices&) = delete;
};

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition, EModifierFlag Modifiers = EModifierFlag::None)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(Modifiers), Type == EInputEventType::MouseButtonDown);
}

static FCursorEvent MakeScrollEvent(float ScrollDelta)
{
    return FCursorEvent(EInputEventType::MouseScrolled, FModifierKeyState(), ScrollDelta, EScrollAxis::Vertical);
}

static FKeyEvent MakeKeyEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

static TArray<TSharedPtr<FTreeItem>> CreateLeafRows(int32 NumRows)
{
    TArray<TSharedPtr<FTreeItem>> Rows;
    Rows.Reserve(NumRows);

    for (int32 Index = 0; Index < NumRows; ++Index)
    {
        Rows.Add(FTreeItem::Create("Row"));
    }

    return Rows;
}

static TArray<FTileItem> CreateTileItems(int32 NumItems)
{
    TArray<FTileItem> Items;
    Items.Reserve(NumItems);

    for (int32 Index = 0; Index < NumItems; ++Index)
    {
        FTileItem Item;
        Item.Label = "Tile";
        Items.Add(Item);
    }

    return Items;
}

bool TreeViewModel_Test()
{
    TEST_BEGIN();

    int32 UserValue = 7;

    TSharedPtr<FTreeItem> Assets   = FTreeItem::Create("Assets");
    TSharedPtr<FTreeItem> Textures = FTreeItem::Create("Textures", &UserValue);
    TSharedPtr<FTreeItem> Rock     = FTreeItem::Create("Rock");
    TSharedPtr<FTreeItem> Meshes   = FTreeItem::Create("Meshes");
    TSharedPtr<FTreeItem> Scenes   = FTreeItem::Create("Scenes");

    TEST_SECTION("A node arrives childless, at the top level, carrying what it was given");
    TEST_EXPECT(Assets->Label.Equals("Assets"));
    TEST_EXPECT(Textures->UserData == &UserValue);
    TEST_EXPECT(!Assets->HasChildren());
    TEST_EXPECT_EQ(Assets->GetDepth(), 0);
    TEST_EXPECT(!Assets->Parent.IsValid());

    TEST_SECTION("Adding a child points it back at the node it was added to");
    Assets->AddChild(Textures);
    Assets->AddChild(Meshes);
    Textures->AddChild(Rock);

    TEST_EXPECT(Assets->HasChildren());
    TEST_EXPECT_EQ(Assets->Children.Size(), 2);
    TEST_EXPECT(Textures->Parent.ToSharedPtr() == Assets);
    TEST_EXPECT(Rock->Parent.ToSharedPtr() == Textures);

    TEST_SECTION("The depth counts the parents standing above the node");
    TEST_EXPECT_EQ(Textures->GetDepth(), 1);
    TEST_EXPECT_EQ(Rock->GetDepth(), 2);

    TEST_SECTION("Clearing the children leaves the node a leaf again");
    TSharedPtr<FTreeItem> Scratch = FTreeItem::Create("Scratch");
    Scratch->AddChild(FTreeItem::Create("Leaf"));
    TEST_EXPECT(Scratch->HasChildren());

    Scratch->ClearChildren();
    TEST_EXPECT(!Scratch->HasChildren());
    TEST_EXPECT_EQ(Scratch->Children.Size(), 0);

    int32                 ExpansionCount = 0;
    TSharedPtr<FTreeItem> LastExpanded   = nullptr;
    bool                  bLastExpanded  = false;

    FTreeView::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.RowHeight          = 20;
    Desc.OnExpansionChanged = FOnTreeItemExpansionChanged::CreateLambda([&](const TSharedPtr<FTreeItem>& Item, bool bIsExpanded)
    {
        ExpansionCount++;
        LastExpanded  = Item;
        bLastExpanded = bIsExpanded;
    });

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, 200));

    TEST_SECTION("The roots are the only rows while every one of them is closed");
    TreeView->SetRootItems({ Assets, Scenes });

    TEST_EXPECT_EQ(TreeView->GetRootItems().Size(), 2);
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 2);
    TEST_EXPECT(TreeView->GetVisibleRows()[0] == Assets);
    TEST_EXPECT(TreeView->GetVisibleRows()[1] == Scenes);

    TEST_SECTION("Opening a row puts its children in below it, in the order they were added");
    TreeView->SetItemExpanded(Assets, true);

    TEST_EXPECT_EQ(ExpansionCount, 1);
    TEST_EXPECT(LastExpanded == Assets);
    TEST_EXPECT(bLastExpanded);
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 4);
    TEST_EXPECT(TreeView->GetVisibleRows()[1] == Textures);
    TEST_EXPECT(TreeView->GetVisibleRows()[2] == Meshes);
    TEST_EXPECT(TreeView->GetVisibleRows()[3] == Scenes);

    TEST_SECTION("Opening a row that is already open moves nothing and reports nothing");
    TreeView->SetItemExpanded(Assets, true);
    TEST_EXPECT_EQ(ExpansionCount, 1);
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 4);

    TEST_SECTION("A grandchild waits for its own parent to open too");
    TreeView->SetItemExpanded(Textures, true);

    TEST_EXPECT_EQ(ExpansionCount, 2);
    TEST_EXPECT(LastExpanded == Textures);
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 5);
    TEST_EXPECT(TreeView->GetVisibleRows()[2] == Rock);

    TEST_SECTION("Opening or closing the whole model reports nothing, however many rows move");
    const int32 CountBeforeSweep = ExpansionCount;

    TreeView->CollapseAll();
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 2);

    TreeView->ExpandAll();
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 5);
    TEST_EXPECT_EQ(ExpansionCount, CountBeforeSweep);

    TEST_SECTION("A refresh is what picks up a model edited behind the view's back");
    TSharedPtr<FTreeItem> Terrain = FTreeItem::Create("Terrain");
    Scenes->AddChild(Terrain);
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 5);

    TreeView->RequestRefresh();
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 6);
    TEST_EXPECT(TreeView->GetVisibleRows()[5] == Terrain);

    TEST_END();
}

bool TreeViewSelection_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTreeItem> Alpha = FTreeItem::Create("Alpha");
    TSharedPtr<FTreeItem> Beta  = FTreeItem::Create("Beta");
    TSharedPtr<FTreeItem> Gamma = FTreeItem::Create("Gamma");
    TSharedPtr<FTreeItem> Delta = FTreeItem::Create("Delta");

    const TArray<TSharedPtr<FTreeItem>> Roots =
    {
        Alpha,
        Beta,
        Gamma,
        Delta,
    };

    int32                         ChangeCount = 0;
    TArray<TSharedPtr<FTreeItem>> LastSelection;

    FTreeView::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.RowHeight          = 20;
    Desc.OnSelectionChanged = FOnTreeSelectionChanged::CreateLambda([&](const TArray<TSharedPtr<FTreeItem>>& Selection)
    {
        ChangeCount++;
        LastSelection = Selection;
    });

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems(Roots);
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, 200));

    TEST_SECTION("A click takes the row under it and reports the selection it leaves behind");
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 10)));

    TEST_EXPECT_EQ(ChangeCount, 1);
    TEST_EXPECT_EQ(TreeView->GetSelection().Size(), 1);
    TEST_EXPECT(TreeView->IsSelected(Alpha));
    TEST_EXPECT_EQ(LastSelection.Size(), 1);
    TEST_EXPECT(LastSelection[0] == Alpha);

    TEST_SECTION("A selected row uses rounded highlight geometry even when it is the top row");
    FDrawCommandList SelectionCommands;
    TreeView->OnDraw(FDrawGeometry(TreeView->GetContentRectangle(), 1.0f), SelectionCommands, 0);

    const FFloatColor InactiveSelection = Desc.Style.InactiveSelectedFill;
    bool              bFoundRoundedSelection = false;
    for (const FDrawCommand& Command : SelectionCommands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Tint == InactiveSelection)
        {
            bFoundRoundedSelection = !Command.CornerRadius.IsZero();
            break;
        }
    }
    TEST_EXPECT(bFoundRoundedSelection);

    TEST_SECTION("A selected row draws its name in the color meant for the selection fill, and its neighbours keep theirs");
    FFloatColor SelectedLabelTint;
    FFloatColor RestingLabelTint;
    for (const FDrawCommand& Command : SelectionCommands.GetCommands())
    {
        if (Command.Type != EDrawCommandType::Text)
        {
            continue;
        }

        if (Command.Text == "Alpha")
        {
            SelectedLabelTint = Command.Tint;
        }
        else if (Command.Text == "Beta")
        {
            RestingLabelTint = Command.Tint;
        }
    }

    TEST_EXPECT(SelectedLabelTint == Desc.Style.SelectedLabelText);
    TEST_EXPECT(RestingLabelTint == Desc.Style.LabelText);

    TEST_SECTION("A chord click adds a row without dropping the one already held");
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 50), EModifierFlag::Ctrl));

    TEST_EXPECT_EQ(ChangeCount, 2);
    TEST_EXPECT_EQ(TreeView->GetSelection().Size(), 2);
    TEST_EXPECT(TreeView->IsSelected(Alpha));
    TEST_EXPECT(TreeView->IsSelected(Gamma));
    TEST_EXPECT_EQ(LastSelection.Size(), 2);

    TEST_SECTION("A shift click takes the whole run between the anchor and the row clicked");
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 10), EModifierFlag::Shift));

    TEST_EXPECT_EQ(ChangeCount, 3);
    TEST_EXPECT_EQ(TreeView->GetSelection().Size(), 3);
    TEST_EXPECT(TreeView->IsSelected(Alpha));
    TEST_EXPECT(TreeView->IsSelected(Beta));
    TEST_EXPECT(TreeView->IsSelected(Gamma));
    TEST_EXPECT(!TreeView->IsSelected(Delta));

    TEST_SECTION("A selection pushed in from the host replaces the one held and reports nothing");
    const int32 CountBeforePush = ChangeCount;
    TreeView->SetSelection({ Delta });

    TEST_EXPECT_EQ(TreeView->GetSelection().Size(), 1);
    TEST_EXPECT(TreeView->IsSelected(Delta));
    TEST_EXPECT(!TreeView->IsSelected(Alpha));
    TEST_EXPECT_EQ(ChangeCount, CountBeforePush);

    TEST_SECTION("A shift click after a pushed selection runs from that selection, which carries no anchor");
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 10), EModifierFlag::Shift));

    TEST_EXPECT_EQ(TreeView->GetSelection().Size(), 4);
    TEST_EXPECT(TreeView->IsSelected(Alpha));
    TEST_EXPECT(TreeView->IsSelected(Beta));
    TEST_EXPECT(TreeView->IsSelected(Gamma));
    TEST_EXPECT(TreeView->IsSelected(Delta));

    TEST_SECTION("Emptying the selection from the host reports nothing either");
    const int32 CountBeforeClear = ChangeCount;
    TreeView->ClearSelection();

    TEST_EXPECT(TreeView->GetSelection().IsEmpty());
    TEST_EXPECT(!TreeView->IsSelected(Delta));
    TEST_EXPECT_EQ(ChangeCount, CountBeforeClear);

    TEST_SECTION("A single-select view answers a chord click with the row clicked and nothing else");
    FTreeView::FDesc SingleDesc;
    SingleDesc.Font              = CreateFont();
    SingleDesc.RowHeight         = 20;
    SingleDesc.bAllowMultiSelect = false;

    TSharedPtr<FTreeView> SingleSelect = FTreeView::Create(SingleDesc);
    SingleSelect->SetRootItems(Roots);
    LayoutElement(SingleSelect, FRectangle(IntVector2(0, 0), 200, 200));

    SingleSelect->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 10)));
    SingleSelect->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 50), EModifierFlag::Ctrl));

    TEST_EXPECT_EQ(SingleSelect->GetSelection().Size(), 1);
    TEST_EXPECT(SingleSelect->IsSelected(Gamma));
    TEST_EXPECT(!SingleSelect->IsSelected(Alpha));

    TEST_END();
}

bool TreeViewFiltering_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTreeItem> Assets      = FTreeItem::Create("Assets");
    TSharedPtr<FTreeItem> Textures    = FTreeItem::Create("Textures");
    TSharedPtr<FTreeItem> RockAlbedo  = FTreeItem::Create("RockAlbedo");
    TSharedPtr<FTreeItem> GrassAlbedo = FTreeItem::Create("GrassAlbedo");
    TSharedPtr<FTreeItem> Meshes      = FTreeItem::Create("Meshes");
    TSharedPtr<FTreeItem> RockMesh    = FTreeItem::Create("RockMesh");
    TSharedPtr<FTreeItem> Scenes      = FTreeItem::Create("Scenes");
    TSharedPtr<FTreeItem> Level       = FTreeItem::Create("Level01");

    Assets->AddChild(Textures);
    Assets->AddChild(Meshes);
    Textures->AddChild(RockAlbedo);
    Textures->AddChild(GrassAlbedo);
    Meshes->AddChild(RockMesh);
    Scenes->AddChild(Level);

    FTreeView::FDesc Desc;
    Desc.Font      = CreateFont();
    Desc.RowHeight = 20;

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems({ Assets, Scenes });
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, 200));

    TEST_SECTION("Nothing is filtered to begin with, so the closed roots are the rows");
    TEST_EXPECT(TreeView->GetFilterText().IsEmpty());
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 2);

    TEST_SECTION("A filter drops the rows whose labels miss it");
    TreeView->SetFilterText("grass");

    TEST_EXPECT(TreeView->GetFilterText().Equals("grass"));
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 3);
    TEST_EXPECT(!TreeView->GetVisibleRows().Contains(Meshes));
    TEST_EXPECT(!TreeView->GetVisibleRows().Contains(RockAlbedo));
    TEST_EXPECT(!TreeView->GetVisibleRows().Contains(Scenes));

    TEST_SECTION("A parent kept only for the match below it is opened, so the match needs no walking to");
    TEST_EXPECT(TreeView->GetVisibleRows()[0] == Assets);
    TEST_EXPECT(TreeView->GetVisibleRows()[1] == Textures);
    TEST_EXPECT(TreeView->GetVisibleRows()[2] == GrassAlbedo);

    TEST_SECTION("The opening lasts as long as the filter rather than being written into the model");
    TEST_EXPECT(!Assets->bIsExpanded);
    TEST_EXPECT(!Textures->bIsExpanded);

    TEST_SECTION("Every branch holding a match is kept, and the rows stay in model order");
    TreeView->SetFilterText("rock");

    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 5);
    TEST_EXPECT(TreeView->GetVisibleRows()[2] == RockAlbedo);
    TEST_EXPECT(TreeView->GetVisibleRows()[3] == Meshes);
    TEST_EXPECT(TreeView->GetVisibleRows()[4] == RockMesh);
    TEST_EXPECT(!TreeView->GetVisibleRows().Contains(GrassAlbedo));

    TEST_SECTION("The match pays no attention to case");
    TreeView->SetFilterText("ROCK");

    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 5);
    TEST_EXPECT(TreeView->GetVisibleRows().Contains(RockAlbedo));
    TEST_EXPECT(TreeView->GetVisibleRows().Contains(RockMesh));

    TEST_SECTION("A row that answers on its own keeps its subtree closed");
    TreeView->SetFilterText("Assets");

    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 1);
    TEST_EXPECT(TreeView->GetVisibleRows()[0] == Assets);

    TEST_SECTION("An empty filter puts every row back");
    TreeView->SetFilterText("");

    TEST_EXPECT(TreeView->GetFilterText().IsEmpty());
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 2);
    TEST_EXPECT(TreeView->GetVisibleRows()[0] == Assets);
    TEST_EXPECT(TreeView->GetVisibleRows()[1] == Scenes);

    TEST_SECTION("The type column answers the filter as well as the label does");
    TSharedPtr<FTreeItem> Scope = FTreeItem::Create("SSAO");
    Scope->TypeLabel            = "3 0.120";

    TSharedPtr<FTreeView> TypeColumnTree = FTreeView::Create(Desc);
    TypeColumnTree->SetRootItems({ Scope });
    LayoutElement(TypeColumnTree, FRectangle(IntVector2(0, 0), 200, 200));
    TypeColumnTree->SetFilterText("0.120");

    TEST_EXPECT_EQ(TypeColumnTree->GetVisibleRows().Size(), 1);

    TEST_SECTION("A view of numbers can keep the filter off the type column, where any digit would answer");
    FTreeView::FDesc LabelOnlyDesc         = Desc;
    LabelOnlyDesc.bFilterMatchesTypeColumn = false;

    TSharedPtr<FTreeView> LabelOnlyTree = FTreeView::Create(LabelOnlyDesc);
    LabelOnlyTree->SetRootItems({ Scope });
    LayoutElement(LabelOnlyTree, FRectangle(IntVector2(0, 0), 200, 200));
    LabelOnlyTree->SetFilterText("0.120");

    TEST_EXPECT_EQ(LabelOnlyTree->GetVisibleRows().Size(), 0);

    LabelOnlyTree->SetFilterText("ssao");

    TEST_EXPECT_EQ(LabelOnlyTree->GetVisibleRows().Size(), 1);

    TEST_END();
}

bool TreeViewKeyboard_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTreeItem> Assets   = FTreeItem::Create("Assets");
    TSharedPtr<FTreeItem> Textures = FTreeItem::Create("Textures");
    TSharedPtr<FTreeItem> Meshes   = FTreeItem::Create("Meshes");
    TSharedPtr<FTreeItem> Scenes   = FTreeItem::Create("Scenes");

    Assets->AddChild(Textures);
    Assets->AddChild(Meshes);

    int32                 ActivationCount = 0;
    TSharedPtr<FTreeItem> LastActivated   = nullptr;

    FTreeView::FDesc Desc;
    Desc.Font            = CreateFont();
    Desc.RowHeight       = 20;
    Desc.OnItemActivated = FOnTreeItemActivated::CreateLambda([&](const TSharedPtr<FTreeItem>& Item)
    {
        ActivationCount++;
        LastActivated = Item;
    });

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems({ Assets, Scenes });
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, 200));

    TEST_SECTION("Down takes the first row when nothing is selected yet, and a row at a time after that");
    TEST_EXPECT(TreeView->OnKeyDown(MakeKeyEvent(Keys::Down)).IsEventHandled());
    TEST_EXPECT(TreeView->IsSelected(Assets));

    TreeView->OnKeyDown(MakeKeyEvent(Keys::Down));
    TEST_EXPECT(TreeView->IsSelected(Scenes));
    TEST_EXPECT_EQ(TreeView->GetSelection().Size(), 1);

    TEST_SECTION("Up walks back the same way, and the first row is as far as it goes");
    TreeView->OnKeyDown(MakeKeyEvent(Keys::Up));
    TEST_EXPECT(TreeView->IsSelected(Assets));

    TreeView->OnKeyDown(MakeKeyEvent(Keys::Up));
    TEST_EXPECT(TreeView->IsSelected(Assets));

    TEST_SECTION("Right opens a closed row and leaves the selection where it was");
    TEST_EXPECT(TreeView->OnKeyDown(MakeKeyEvent(Keys::Right)).IsEventHandled());

    TEST_EXPECT(Assets->bIsExpanded);
    TEST_EXPECT(TreeView->IsSelected(Assets));
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 4);

    TEST_SECTION("Right again moves to the first child now that there is one to move to");
    TreeView->OnKeyDown(MakeKeyEvent(Keys::Right));
    TEST_EXPECT(TreeView->IsSelected(Textures));

    TEST_SECTION("Left on a leaf moves up to the parent");
    TreeView->OnKeyDown(MakeKeyEvent(Keys::Left));
    TEST_EXPECT(TreeView->IsSelected(Assets));

    TEST_SECTION("Left on an open row closes it before it moves anywhere");
    TreeView->OnKeyDown(MakeKeyEvent(Keys::Left));

    TEST_EXPECT(!Assets->bIsExpanded);
    TEST_EXPECT(TreeView->IsSelected(Assets));
    TEST_EXPECT_EQ(TreeView->GetVisibleRows().Size(), 2);

    TEST_SECTION("Left on a closed root has nowhere to go and stays put");
    TreeView->OnKeyDown(MakeKeyEvent(Keys::Left));
    TEST_EXPECT(TreeView->IsSelected(Assets));

    TEST_SECTION("Enter opens the row the keyboard is on");
    TEST_EXPECT(TreeView->OnKeyDown(MakeKeyEvent(Keys::Enter)).IsEventHandled());

    TEST_EXPECT_EQ(ActivationCount, 1);
    TEST_EXPECT(LastActivated == Assets);

    TEST_SECTION("A double click opens the row under the cursor the same way");
    TreeView->OnMouseDoubleClick(MakeButtonEvent(EInputEventType::MouseButtonDoubleClick, IntVector2(100, 30)));

    TEST_EXPECT_EQ(ActivationCount, 2);
    TEST_EXPECT(LastActivated == Scenes);

    TEST_SECTION("A double click below the last row opens nothing");
    TEST_EXPECT(!TreeView->OnMouseDoubleClick(MakeButtonEvent(EInputEventType::MouseButtonDoubleClick, IntVector2(100, 150))).IsEventHandled());
    TEST_EXPECT_EQ(ActivationCount, 2);

    TEST_SECTION("A key the view has no use for is left to whatever else is listening");
    TEST_EXPECT(!TreeView->OnKeyDown(MakeKeyEvent(Keys::S)).IsEventHandled());

    TEST_END();
}

bool TreeViewScrolling_Test()
{
    TEST_BEGIN();

    constexpr int32 RowHeight  = 20;
    constexpr int32 NumRows    = 20;
    constexpr int32 ViewHeight = 100;

    FTreeView::FDesc Desc;
    Desc.Font      = CreateFont();
    Desc.RowHeight = RowHeight;

    const TArray<TSharedPtr<FTreeItem>> Rows = CreateLeafRows(NumRows);

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems(Rows);
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    TEST_SECTION("One wheel step moves the rows by the step the view is built with");
    TEST_EXPECT(TreeView->OnMouseScroll(MakeScrollEvent(-1.0f)).IsEventHandled());
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), FTreeView::RowsPerWheelStep * RowHeight);

    TEST_SECTION("Wheeling back past the first row stops at the top");
    TreeView->OnMouseScroll(MakeScrollEvent(1.0f));
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), 0);

    TreeView->OnMouseScroll(MakeScrollEvent(1.0f));
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), 0);

    TEST_SECTION("Wheeling on past the last row stops where the rows end");
    for (int32 Step = 0; Step < NumRows; ++Step)
    {
        TreeView->OnMouseScroll(MakeScrollEvent(-1.0f));
    }

    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (NumRows * RowHeight) - ViewHeight);

    TEST_SECTION("Revealing a row above the view brings it down to the top edge");
    TreeView->ScrollToItem(Rows[0]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), 0);

    TEST_SECTION("Revealing a row below the view lifts it to the bottom edge and no further");
    TreeView->ScrollToItem(Rows[6]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (7 * RowHeight) - ViewHeight);

    TEST_SECTION("A row already fully in view is left where it is");
    TreeView->ScrollToItem(Rows[6]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (7 * RowHeight) - ViewHeight);

    TreeView->ScrollToItem(Rows[5]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (7 * RowHeight) - ViewHeight);

    TEST_SECTION("The row above the view is reached by scrolling back only as far as its top");
    TreeView->ScrollToItem(Rows[1]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), RowHeight);

    TEST_SECTION("The last row is reachable and lands at the far end of the range");
    TreeView->ScrollToItem(Rows[NumRows - 1]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (NumRows * RowHeight) - ViewHeight);

    TEST_SECTION("An item with no row of its own moves nothing");
    TreeView->ScrollToItem(FTreeItem::Create("Orphan"));
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (NumRows * RowHeight) - ViewHeight);

    TEST_SECTION("A view whose rows all fit leaves the wheel to whatever else is listening");
    TSharedPtr<FTreeView> ShortView = FTreeView::Create(Desc);
    ShortView->SetRootItems(CreateLeafRows(2));
    LayoutElement(ShortView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    TEST_EXPECT(!ShortView->OnMouseScroll(MakeScrollEvent(-1.0f)).IsEventHandled());
    TEST_EXPECT_EQ(ShortView->GetScrollOffset(), 0);

    TEST_SECTION("A selected fill stops short of the scrollbar so its right corners stay round");
    FTreeView::FDesc BarDesc;
    BarDesc.Font           = CreateFont();
    BarDesc.RowHeight      = RowHeight;
    BarDesc.bShowScrollBar = true;

    TSharedPtr<FTreeView> BarView = FTreeView::Create(BarDesc);
    BarView->SetRootItems(CreateLeafRows(NumRows));
    BarView->SetSelection({ BarView->GetVisibleRows()[0] });
    LayoutElement(BarView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    FDrawCommandList HighlightCommands;
    BarView->OnDraw(FDrawGeometry(BarView->GetContentRectangle(), 1.0f), HighlightCommands, 0);

    const FFloatColor InactiveSelection = BarDesc.Style.InactiveSelectedFill;
    FRectangle        DrawnHighlight;
    bool              bFoundInsetHighlight = false;
    for (const FDrawCommand& Command : HighlightCommands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Tint == InactiveSelection)
        {
            const int32 Thickness = FUIStyle::GetDefault().Metrics.ScrollBarThickness;
            TEST_EXPECT(Command.Bounds.GetRight() <= BarView->GetContentRectangle().GetRight() - Thickness);
            DrawnHighlight       = Command.Bounds;
            bFoundInsetHighlight = true;
            break;
        }
    }
    TEST_EXPECT(bFoundInsetHighlight);

    TEST_SECTION("That band is handed out, so a host marking a drop target lands on the selection's shape");
    const TSharedPtr<FTreeItem>& FirstRow = BarView->GetVisibleRows()[0];

    TEST_EXPECT(BarView->GetItemHighlightBounds(FirstRow) == DrawnHighlight);
    TEST_EXPECT(BarView->GetItemHighlightBounds(FirstRow).GetRight() < BarView->GetItemRowBounds(FirstRow).GetRight());
    TEST_EXPECT(BarView->GetItemHighlightBounds(nullptr).IsEmpty());

    TEST_END();
}

bool TreeViewColumnsAndIndent_Test()
{
    TEST_BEGIN();

    constexpr int32 RowHeight    = 20;
    constexpr int32 HeaderHeight = 24;
    constexpr int32 NumRows      = 20;
    constexpr int32 ViewHeight   = 100;

    FTreeView::FDesc Desc;
    Desc.Font              = CreateFont();
    Desc.RowHeight         = RowHeight;
    Desc.HeaderHeight      = HeaderHeight;
    Desc.LabelColumnHeader = "Item Label";
    Desc.TypeColumnHeader  = "Type";
    Desc.TypeColumnWidth   = 60;

    const TArray<TSharedPtr<FTreeItem>> Rows = CreateLeafRows(NumRows);

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems(Rows);
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    TEST_SECTION("The header takes its height off the band the rows scroll in");
    TreeView->OnMouseScroll(MakeScrollEvent(-100.0f));
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), (NumRows * RowHeight) - (ViewHeight - HeaderHeight));

    TreeView->ScrollToItem(Rows[0]);
    TEST_EXPECT_EQ(TreeView->GetScrollOffset(), 0);

    TEST_SECTION("The first row starts below the header rather than at the top of the view");
    TEST_EXPECT_EQ(TreeView->GetItemRowBounds(Rows[0]).Position.Y, HeaderHeight);

    TEST_SECTION("A click on the header selects nothing, and one just below it takes the first row");
    TEST_EXPECT(!TreeView->FindItemAt(IntVector2(10, HeaderHeight / 2)));
    TEST_EXPECT(TreeView->FindItemAt(IntVector2(10, HeaderHeight + 1)) == Rows[0]);

    TEST_SECTION("A view with no captions keeps its rows hard against the top");
    Desc.LabelColumnHeader.Clear();
    Desc.TypeColumnHeader.Clear();

    TSharedPtr<FTreeView> BareView = FTreeView::Create(Desc);
    BareView->SetRootItems(CreateLeafRows(NumRows));
    LayoutElement(BareView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    TEST_EXPECT_EQ(BareView->GetItemRowBounds(BareView->GetVisibleRows()[0]).Position.Y, 0);

    BareView->OnMouseScroll(MakeScrollEvent(-100.0f));
    TEST_EXPECT_EQ(BareView->GetScrollOffset(), (NumRows * RowHeight) - ViewHeight);

    TEST_SECTION("Rows at one depth line up whether or not each of them carries an icon");
    TSharedPtr<FTreeItem> Filter    = FTreeItem::Create("Filter");
    TSharedPtr<FTreeItem> LooseItem = FTreeItem::Create("Loose");
    TSharedPtr<FTreeItem> Child     = FTreeItem::Create("Child");

    Filter->Icon = FUIBrush(reinterpret_cast<FRHITexture*>(0x10));
    Filter->AddChild(Child);

    FTreeView::FDesc IndentDesc;
    IndentDesc.Font           = CreateFont();
    IndentDesc.RowHeight      = RowHeight;
    IndentDesc.IndentPerLevel = 18;

    TSharedPtr<FTreeView> IndentView = FTreeView::Create(IndentDesc);
    IndentView->SetRootItems({ Filter, LooseItem });
    IndentView->SetItemExpanded(Filter, true);
    LayoutElement(IndentView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    const int32 FilterLabelX = IndentView->GetItemLabelBounds(Filter).Position.X;
    const int32 LooseLabelX  = IndentView->GetItemLabelBounds(LooseItem).Position.X;
    const int32 ChildLabelX  = IndentView->GetItemLabelBounds(Child).Position.X;

    TEST_EXPECT_EQ(LooseLabelX, FilterLabelX);

    TEST_SECTION("A child indents a full level past the parent it hangs off");
    TEST_EXPECT_EQ(ChildLabelX, FilterLabelX + IndentDesc.IndentPerLevel);

    TEST_SECTION("The disclosure arrow sits inside the row rather than against its left edge");
    TEST_EXPECT(FilterLabelX >= FUIStyle::GetDefault().TreeRow.ContentInset);

    TEST_SECTION("A view where nothing has an icon reserves no column for one");
    TSharedPtr<FTreeItem> PlainParent = FTreeItem::Create("Parent");
    TSharedPtr<FTreeItem> PlainChild  = FTreeItem::Create("Child");
    PlainParent->AddChild(PlainChild);

    TSharedPtr<FTreeView> PlainView = FTreeView::Create(IndentDesc);
    PlainView->SetRootItems({ PlainParent });
    PlainView->SetItemExpanded(PlainParent, true);
    LayoutElement(PlainView, FRectangle(IntVector2(0, 0), 200, ViewHeight));

    TEST_EXPECT(PlainView->GetItemLabelBounds(PlainParent).Position.X < FilterLabelX);
    TEST_EXPECT_EQ(PlainView->GetItemLabelBounds(PlainChild).Position.X,
        PlainView->GetItemLabelBounds(PlainParent).Position.X + IndentDesc.IndentPerLevel);

    TEST_END();
}

bool TreeViewHeaderToolTips_Test()
{
    TEST_BEGIN();

    FScopedStubApplication     Application;
    FScopedItemViewTestServices Services;

    constexpr int32 HeaderHeight    = 24;
    constexpr int32 ViewWidth       = 320;
    constexpr int32 TypeColumnWidth = 120;
    constexpr int32 TypeColumnX     = ViewWidth - TypeColumnWidth;

    const String CallsToolTip = "How many times the scope was entered";
    const String InclToolTip  = "Time spent in the scope, its children included";

    FTreeView::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.RowHeight          = 20;
    Desc.HeaderHeight       = HeaderHeight;
    Desc.LabelColumnHeader  = "Scope";
    Desc.TypeColumnHeader   = "Calls   Incl ms";
    Desc.TypeColumnWidth    = TypeColumnWidth;
    Desc.LabelColumnToolTip = "The scopes, nested the way they ran";
    Desc.TypeColumnToolTips = { CallsToolTip, InclToolTip };

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems(CreateLeafRows(4));
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), ViewWidth, 100));

    FToolTipService& ToolTips = FToolTipService::Get();

    TEST_SECTION("Resting on a caption asks for the tip explaining that column");
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(10, HeaderHeight / 2)));

    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT_EQ(ToolTips.GetOwner(), StaticCastSharedPtr<FVisualElement>(TreeView));
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), Desc.LabelColumnToolTip);

    TEST_SECTION("Every caption in the type column carries its own tip, rather than one for the lot");
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 10, HeaderHeight / 2)));

    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), CallsToolTip);

    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 60, HeaderHeight / 2)));

    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), InclToolTip);

    TEST_SECTION("The gap between two captions belongs to the nearer of them");
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 45, HeaderHeight / 2)));
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), CallsToolTip);

    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 55, HeaderHeight / 2)));
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), InclToolTip);

    TEST_SECTION("Past the last caption the column still reads as that caption's");
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(ViewWidth - 2, HeaderHeight / 2)));
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), InclToolTip);

    TEST_SECTION("A column with one tip hangs it off the whole of it");
    FTreeView::FDesc SharedDesc = Desc;
    SharedDesc.TypeColumnToolTips = { CallsToolTip };

    TSharedPtr<FTreeView> SharedView = FTreeView::Create(SharedDesc);
    SharedView->SetRootItems(CreateLeafRows(4));
    LayoutElement(SharedView, FRectangle(IntVector2(0, 0), ViewWidth, 100));

    SharedView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 10, HeaderHeight / 2)));
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), CallsToolTip);

    SharedView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 100, HeaderHeight / 2)));
    TEST_EXPECT_EQ(ToolTips.GetRequestedText(), CallsToolTip);

    SharedView->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));

    TEST_SECTION("Dropping onto the rows takes the tip down, since only the captions explain themselves");
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(TypeColumnX + 10, HeaderHeight / 2)));
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(10, HeaderHeight + 10)));

    TEST_EXPECT(!ToolTips.IsPending());

    TEST_SECTION("Leaving the view takes the tip with it");
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(10, HeaderHeight / 2)));
    TEST_EXPECT(ToolTips.IsPending());

    TreeView->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    TEST_EXPECT(!ToolTips.IsPending());

    TEST_SECTION("A view whose columns speak for themselves asks for nothing");
    Desc.LabelColumnToolTip.Clear();
    Desc.TypeColumnToolTips.Clear();

    TSharedPtr<FTreeView> BareView = FTreeView::Create(Desc);
    BareView->SetRootItems(CreateLeafRows(4));
    LayoutElement(BareView, FRectangle(IntVector2(0, 0), ViewWidth, 100));

    BareView->OnMouseMove(MakeMoveEvent(IntVector2(10, HeaderHeight / 2)));

    TEST_EXPECT(!ToolTips.IsPending());

    TEST_END();
}

static FFloatColor MakeTreeHoverFill(const FUITreeRowStyle& Style)
{
    FFloatColor Fill = Style.HoveredFill;
    Fill.A           = 0.5f;
    return Fill;
}

static bool DrawsHoverForItem(const TSharedPtr<FTreeView>& Tree, const TSharedPtr<FTreeItem>& Item, const FFloatColor& HoverFill)
{
    FDrawCommandList Commands;
    Tree->OnDraw(FDrawGeometry(Tree->GetContentRectangle(), 1.0f), Commands, 0);

    const FRectangle Highlight = Tree->GetItemHighlightBounds(Item);
    for (const FDrawCommand& Command : Commands.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box && Command.Tint == HoverFill && Command.Bounds == Highlight)
        {
            return true;
        }
    }

    return false;
}

bool TreeViewHoverAndRowClick_Test()
{
    TEST_BEGIN();

    constexpr int32 RowHeight = 20;

    TSharedPtr<FTreeItem> Assets   = FTreeItem::Create("Assets");
    TSharedPtr<FTreeItem> Textures = FTreeItem::Create("Textures");
    TSharedPtr<FTreeItem> Meshes   = FTreeItem::Create("Meshes");
    Assets->AddChild(Textures);
    Assets->AddChild(Meshes);

    int32 ExpansionCount = 0;
    int32 DragCount      = 0;

    FTreeView::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.RowHeight          = RowHeight;
    Desc.OnExpansionChanged = FOnTreeItemExpansionChanged::CreateLambda([&](const TSharedPtr<FTreeItem>&, bool)
    {
        ExpansionCount++;
    });
    Desc.OnDragDetected = FOnTreeItemDragDetected::CreateLambda([&](const TSharedPtr<FTreeItem>&, const FCursorEvent&)
    {
        DragCount++;
    });

    TSharedPtr<FTreeView> TreeView = FTreeView::Create(Desc);
    TreeView->SetRootItems({ Assets, FTreeItem::Create("Scenes") });
    LayoutElement(TreeView, FRectangle(IntVector2(0, 0), 200, 200));

    const FFloatColor HoverFill = MakeTreeHoverFill(Desc.Style);
    const IntVector2  FirstRow  = IntVector2(100, RowHeight / 2);

    TEST_SECTION("A hover fill stays after the roots are replaced without a further mouse move");
    TreeView->OnMouseMove(MakeMoveEvent(FirstRow));
    TEST_EXPECT(DrawsHoverForItem(TreeView, Assets, HoverFill));

    TreeView->SetRootItems({ Assets, FTreeItem::Create("Scenes") });
    TEST_EXPECT(DrawsHoverForItem(TreeView, Assets, HoverFill));

    TEST_SECTION("Scrolling under a stationary cursor moves the hover onto the row now under it");
    const TArray<TSharedPtr<FTreeItem>> Rows = CreateLeafRows(20);
    TSharedPtr<FTreeView> ScrollView = FTreeView::Create(Desc);
    ScrollView->SetRootItems(Rows);
    LayoutElement(ScrollView, FRectangle(IntVector2(0, 0), 200, 100));

    ScrollView->OnMouseMove(MakeMoveEvent(FirstRow));
    TEST_EXPECT(DrawsHoverForItem(ScrollView, Rows[0], HoverFill));

    ScrollView->OnMouseScroll(MakeScrollEvent(-1.0f));
    TEST_EXPECT(!DrawsHoverForItem(ScrollView, Rows[0], HoverFill));
    TEST_EXPECT(DrawsHoverForItem(ScrollView, Rows[FTreeView::RowsPerWheelStep], HoverFill));

    TEST_SECTION("A completed click on an expandable row opens it, not only a click on the arrow");
    const FRectangle LabelBounds = TreeView->GetItemLabelBounds(Assets);
    const IntVector2 LabelClick(LabelBounds.Position.X + 8, LabelBounds.Position.Y + (LabelBounds.Height / 2));

    TEST_EXPECT(!Assets->bIsExpanded);
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, LabelClick));
    TreeView->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, LabelClick));
    TEST_EXPECT(Assets->bIsExpanded);
    TEST_EXPECT_EQ(ExpansionCount, 1);

    TEST_SECTION("A press that travels far enough to become a drag does not toggle expansion");
    const int32 CountBeforeDrag = ExpansionCount;
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, LabelClick));
    TreeView->OnMouseMove(MakeMoveEvent(IntVector2(LabelClick.X + FTreeView::DragThreshold + 1, LabelClick.Y)));
    TreeView->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(LabelClick.X + FTreeView::DragThreshold + 1, LabelClick.Y)));
    TEST_EXPECT_EQ(DragCount, 1);
    TEST_EXPECT_EQ(ExpansionCount, CountBeforeDrag);
    TEST_EXPECT(Assets->bIsExpanded);

    TEST_SECTION("A chord click extends the selection without toggling expansion");
    const int32 CountBeforeChord = ExpansionCount;
    TreeView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, LabelClick, EModifierFlag::Ctrl));
    TreeView->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, LabelClick, EModifierFlag::Ctrl));
    TEST_EXPECT_EQ(ExpansionCount, CountBeforeChord);

    TEST_END();
}

bool TileViewLayout_Test()
{
    TEST_BEGIN();

    constexpr int32 TileSide    = 50;
    constexpr int32 TileSpacing = 10;
    constexpr int32 ViewWidth   = 250;
    constexpr int32 ViewHeight  = 100;

    const FRectangle Bounds(IntVector2(0, 0), ViewWidth, ViewHeight);

    FTileView::FDesc Desc;
    Desc.Font        = CreateFont();
    Desc.TileSize    = IntVector2(TileSide, TileSide);
    Desc.TileSpacing = TileSpacing;
    Desc.IconSize    = 32;

    TSharedPtr<FTileView> TileView = FTileView::Create(Desc);
    TileView->SetItems(CreateTileItems(4));
    LayoutElement(TileView, Bounds);

    TEST_SECTION("The items given are the items shown");
    TEST_EXPECT_EQ(TileView->GetItems().Size(), 4);
    TEST_EXPECT(TileView->GetItems()[0].Label.Equals("Tile"));

    TEST_SECTION("A grid that fits inside the view has nothing to scroll");
    TEST_EXPECT_EQ(TileView->GetMaxScrollOffset(), 0);

    TEST_SECTION("Replacing the items drops the selection along with them");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(25, 25)));
    TEST_EXPECT_EQ(TileView->GetSelection().Size(), 1);

    TileView->SetItems(CreateTileItems(6));
    TEST_EXPECT_EQ(TileView->GetItems().Size(), 6);
    TEST_EXPECT(TileView->GetSelection().IsEmpty());
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), FTileView::InvalidTileIndex);

    LayoutElement(TileView, Bounds);

    TEST_SECTION("The tiles wrap once the width runs out, which is four of them across here");
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(205, 25)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), 3);

    TileView->OnMouseMove(MakeMoveEvent(IntVector2(25, 85)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), 4);

    TEST_SECTION("A second row of tiles overhangs the view, and that overhang is the range");
    // Two rows of fifty with a ten pixel gap stand a hundred and ten tall against a hundred of view
    TEST_EXPECT_EQ(TileView->GetMaxScrollOffset(), 10);

    TEST_SECTION("A wider tile re-flows the grid into fewer columns");
    TileView->SetTileSize(IntVector2(110, TileSide));
    LayoutElement(TileView, Bounds);

    TEST_EXPECT_EQ(TileView->GetTileSize().X, 110);

    TileView->OnMouseMove(MakeMoveEvent(IntVector2(55, 85)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), 2);

    TEST_SECTION("The taller grid the re-flow leaves has a longer range to scroll");
    TEST_EXPECT_EQ(TileView->GetMaxScrollOffset(), 70);

    TEST_SECTION("An offset pushed in from the host is clamped into that range");
    TileView->SetScrollOffset(9999);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 70);

    TileView->SetScrollOffset(-40);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 0);

    TileView->SetScrollOffset(30);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 30);

    TEST_SECTION("Scrolling to a tile moves the least it can to bring the whole tile into view");
    TileView->ScrollToTile(5);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 70);

    TileView->ScrollToTile(0);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 0);

    TEST_SECTION("A tile already inside the view leaves the grid where it stands");
    TileView->SetScrollOffset(30);
    TileView->ScrollToTile(2);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 30);

    TEST_SECTION("Scrolling to something that is not a tile does nothing");
    TileView->ScrollToTile(99);
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 30);

    TEST_SECTION("One wheel step moves the grid by the step the view is built with");
    TileView->SetScrollOffset(0);

    TEST_EXPECT(TileView->OnMouseScroll(MakeScrollEvent(-1.0f)).IsEventHandled());
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), FTileView::DefaultScrollAmountPerWheelStep);

    TEST_SECTION("A grid with nothing to scroll leaves the wheel to whatever else is listening");
    TileView->SetItems(CreateTileItems(2));
    LayoutElement(TileView, Bounds);

    TEST_EXPECT_EQ(TileView->GetMaxScrollOffset(), 0);
    TEST_EXPECT(!TileView->OnMouseScroll(MakeScrollEvent(-1.0f)).IsEventHandled());
    TEST_EXPECT_EQ(TileView->GetScrollOffset(), 0);

    TEST_END();
}

bool TileViewSelection_Test()
{
    TEST_BEGIN();

    int32         ChangeCount = 0;
    TArray<int32> LastSelection;

    int32 ActivationCount = 0;
    int32 LastActivated   = FTileView::InvalidTileIndex;

    FTileView::FDesc Desc;
    Desc.Font               = CreateFont();
    Desc.TileSize           = IntVector2(50, 50);
    Desc.TileSpacing        = 10;
    Desc.IconSize           = 32;
    Desc.OnSelectionChanged = FOnTileSelectionChanged::CreateLambda([&](const TArray<int32>& SelectedIndices)
    {
        ChangeCount++;
        LastSelection = SelectedIndices;
    });

    Desc.OnItemActivated = FOnTileActivated::CreateLambda([&](int32 Index)
    {
        ActivationCount++;
        LastActivated = Index;
    });

    TSharedPtr<FTileView> TileView = FTileView::Create(Desc);
    TileView->SetItems(CreateTileItems(8));
    LayoutElement(TileView, FRectangle(IntVector2(0, 0), 250, 100));

    TEST_SECTION("A click takes the tile under it and reports which one that was");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(85, 25)));

    TEST_EXPECT_EQ(ChangeCount, 1);
    TEST_EXPECT_EQ(TileView->GetSelection().Size(), 1);
    TEST_EXPECT(TileView->IsSelected(1));
    TEST_EXPECT(!TileView->IsSelected(0));
    TEST_EXPECT_EQ(LastSelection.Size(), 1);
    TEST_EXPECT_EQ(LastSelection[0], 1);

    TEST_SECTION("A chord click adds a tile to the selection and a second one takes it back out");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(145, 25), EModifierFlag::Ctrl));

    TEST_EXPECT_EQ(ChangeCount, 2);
    TEST_EXPECT_EQ(TileView->GetSelection().Size(), 2);
    TEST_EXPECT(TileView->IsSelected(1));
    TEST_EXPECT(TileView->IsSelected(2));

    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(145, 25), EModifierFlag::Ctrl));

    TEST_EXPECT_EQ(ChangeCount, 3);
    TEST_EXPECT_EQ(TileView->GetSelection().Size(), 1);
    TEST_EXPECT(!TileView->IsSelected(2));

    TEST_SECTION("A shift click takes the whole run between the anchor and the tile clicked");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(85, 85), EModifierFlag::Shift));

    TEST_EXPECT_EQ(ChangeCount, 4);
    TEST_EXPECT_EQ(TileView->GetSelection().Size(), 4);
    TEST_EXPECT(TileView->IsSelected(2));
    TEST_EXPECT(TileView->IsSelected(3));
    TEST_EXPECT(TileView->IsSelected(4));
    TEST_EXPECT(TileView->IsSelected(5));
    TEST_EXPECT(!TileView->IsSelected(1));
    TEST_EXPECT_EQ(LastSelection.Size(), 4);

    TEST_SECTION("A click that lands on no tile at all drops the selection");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(245, 25)));

    TEST_EXPECT_EQ(ChangeCount, 5);
    TEST_EXPECT(TileView->GetSelection().IsEmpty());
    TEST_EXPECT(LastSelection.IsEmpty());

    TEST_SECTION("Dropping a selection reports it once, and dropping nothing reports nothing");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(25, 25)));
    TEST_EXPECT_EQ(ChangeCount, 6);

    TileView->ClearSelection();
    TEST_EXPECT(TileView->GetSelection().IsEmpty());
    TEST_EXPECT_EQ(ChangeCount, 7);

    TileView->ClearSelection();
    TEST_EXPECT_EQ(ChangeCount, 7);

    TEST_SECTION("A double click opens the tile under the cursor");
    TileView->OnMouseDoubleClick(MakeButtonEvent(EInputEventType::MouseButtonDoubleClick, IntVector2(145, 85)));

    TEST_EXPECT_EQ(ActivationCount, 1);
    TEST_EXPECT_EQ(LastActivated, 6);

    TEST_SECTION("A double click away from every tile opens nothing");
    TEST_EXPECT(!TileView->OnMouseDoubleClick(MakeButtonEvent(EInputEventType::MouseButtonDoubleClick, IntVector2(245, 25))).IsEventHandled());
    TEST_EXPECT_EQ(ActivationCount, 1);

    TEST_SECTION("The hover follows the cursor from tile to tile");
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(25, 25)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), 0);

    TileView->OnMouseMove(MakeMoveEvent(IntVector2(205, 85)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), 7);

    TEST_SECTION("The hover is dropped where there is no tile, and dropped again once the cursor leaves");
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(245, 25)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), FTileView::InvalidTileIndex);

    TileView->OnMouseMove(MakeMoveEvent(IntVector2(25, 25)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), 0);

    TileView->OnMouseLeft(MakeMoveEvent(IntVector2(900, 900)));
    TEST_EXPECT_EQ(TileView->GetHoveredTile(), FTileView::InvalidTileIndex);

    TEST_END();
}

bool TileViewDrag_Test()
{
    TEST_BEGIN();

    constexpr int32 TileSide    = 50;
    constexpr int32 TileSpacing = 10;

    int32 DragCount   = 0;
    int32 DraggedTile = FTileView::InvalidTileIndex;

    FTileView::FDesc Desc;
    Desc.Font        = CreateFont();
    Desc.TileSize    = IntVector2(TileSide, TileSide);
    Desc.TileSpacing = TileSpacing;
    Desc.IconSize    = 32;
    Desc.OnDragDetected.BindLambda([&](int32 Index, const FCursorEvent&)
    {
        ++DragCount;
        DraggedTile = Index;
    });

    TSharedPtr<FTileView> TileView = FTileView::Create(Desc);
    TileView->SetItems(CreateTileItems(8));
    LayoutElement(TileView, FRectangle(IntVector2(0, 0), 250, 100));

    TEST_SECTION("Where a tile sits is the grid position it was laid out at");
    const FRectangle FirstTile = TileView->GetTileBounds(0);
    TEST_EXPECT_EQ(FirstTile.Position.X, 0);
    TEST_EXPECT_EQ(FirstTile.Position.Y, 0);
    TEST_EXPECT_EQ(FirstTile.Width, TileSide);
    TEST_EXPECT_EQ(FirstTile.Height, TileSide);

    const FRectangle SecondRowTile = TileView->GetTileBounds(4);
    TEST_EXPECT_EQ(SecondRowTile.Position.X, 0);
    TEST_EXPECT_EQ(SecondRowTile.Position.Y, TileSide + TileSpacing);

    TEST_SECTION("A tile that is not there has no bounds to give back");
    TEST_EXPECT_EQ(TileView->GetTileBounds(-1).Width, 0);
    TEST_EXPECT_EQ(TileView->GetTileBounds(99).Width, 0);

    TEST_SECTION("Moving without a press behind it is not a drag");
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(200, 80)));
    TEST_EXPECT_EQ(DragCount, 0);

    TEST_SECTION("A press followed by a move short of the threshold is still a click");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(25, 25)));
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(25 + FTileView::DragThreshold, 25)));
    TEST_EXPECT_EQ(DragCount, 0);

    TEST_SECTION("One pixel past the threshold turns the press into a drag of the tile it started on");
    TEST_EXPECT(TileView->OnMouseMove(MakeMoveEvent(IntVector2(25 + FTileView::DragThreshold + 1, 25))).IsEventHandled());
    TEST_EXPECT_EQ(DragCount, 1);
    TEST_EXPECT_EQ(DraggedTile, 0);

    TEST_SECTION("The drag is raised once, not again for every move that follows");
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(145, 85)));
    TEST_EXPECT_EQ(DragCount, 1);

    TEST_SECTION("Releasing the button drops the press, so a later move is not a drag");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(85, 25)));
    TileView->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(85, 25)));
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(205, 85)));
    TEST_EXPECT_EQ(DragCount, 1);

    TEST_SECTION("A press that lands on no tile has nothing to drag");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(245, 25)));
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(145, 85)));
    TEST_EXPECT_EQ(DragCount, 1);

    TEST_SECTION("The cursor leaving drops the press along with the hover");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(25, 25)));
    TileView->OnMouseLeft(MakeMoveEvent(IntVector2(900, 900)));
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(145, 85)));
    TEST_EXPECT_EQ(DragCount, 1);

    TEST_SECTION("A drag that starts on the second tile reports that tile");
    TileView->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(85, 25)));
    TileView->OnMouseMove(MakeMoveEvent(IntVector2(85, 60)));
    TEST_EXPECT_EQ(DragCount, 2);
    TEST_EXPECT_EQ(DraggedTile, 1);

    TEST_END();
}
