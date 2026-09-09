#include "PropertyTableTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Math/Math.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/ElementPath.h>
#include <Application/Elements/PropertyTable.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/ToolTipService.h>
#include <Application/Text/FixedWidthFontFace.h>

class FScopedPropertyTableTestServices
{
public:
    FScopedPropertyTableTestServices() = default;

    ~FScopedPropertyTableTestServices()
    {
        FToolTipService::Shutdown();
    }

    FScopedPropertyTableTestServices(const FScopedPropertyTableTestServices&) = delete;
    FScopedPropertyTableTestServices& operator=(const FScopedPropertyTableTestServices&) = delete;
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

static TSharedPtr<FVisualElement> CreateEditor(const String& Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = Font;

    return FTextBlock::Create(Desc);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition, FKey Key = Keys::MouseButtonLeft)
{
    return FCursorEvent(Type, Key, ClientPosition, IntVector2(0, 0), FModifierKeyState(), Type == EInputEventType::MouseButtonDown);
}

static void DrawElement(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList)
{
    Element->OnDraw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), OutCommandList, 0);
}

bool PropertyTableRows_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FPropertyTable::FDesc Desc;
    Desc.Font      = Font;
    Desc.RowHeight = 20;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(Desc);

    TEST_SECTION("A row is a label beside an editor, handed back so the caller can finish describing it");
    TSharedPtr<FVisualElement> LocationEditor = CreateEditor("0, 0, 0", Font);

    FPropertyRow& LocationRow = Table->AddRow("Location", LocationEditor);
    LocationRow.ToolTipText   = "Where the actor sits in the world";
    LocationRow.IndentLevel   = 1;

    TEST_EXPECT_EQ(Table->GetNumRows(), 1);
    TEST_EXPECT(Table->GetRow(0).Label == String("Location"));
    TEST_EXPECT(Table->GetRow(0).Editor == LocationEditor);
    TEST_EXPECT(Table->GetRow(0).ToolTipText == String("Where the actor sits in the world"));
    TEST_EXPECT_EQ(Table->GetRow(0).IndentLevel, 1);
    TEST_EXPECT(!Table->GetRow(0).bIsHeader);

    TEST_SECTION("A heading spans both columns, so it is marked as one and carries no editor");
    FPropertyRow& RenderingRow = Table->AddHeaderRow("Rendering");
    RenderingRow.ToolTipText   = "How the actor is drawn";

    TEST_EXPECT(Table->GetRow(1).bIsHeader);
    TEST_EXPECT(Table->GetRow(1).Editor == nullptr);
    TEST_EXPECT(Table->GetRow(1).Label == String("Rendering"));
    TEST_EXPECT(Table->GetRow(1).ToolTipText == String("How the actor is drawn"));

    TEST_SECTION("The count takes in the headings, and the rows read back in the order they arrived");
    TSharedPtr<FVisualElement> VisibleEditor = CreateEditor("true", Font);
    Table->AddRow("Visible", VisibleEditor);

    TEST_EXPECT_EQ(Table->GetNumRows(), 3);
    TEST_EXPECT(Table->GetRow(0).Label == String("Location"));
    TEST_EXPECT(Table->GetRow(1).Label == String("Rendering"));
    TEST_EXPECT(Table->GetRow(2).Label == String("Visible"));
    TEST_EXPECT(Table->GetRow(2).Editor == VisibleEditor);

    TEST_SECTION("The table is as tall as its rows together and asks for no width of its own");
    Table->PrepareDesiredSize();
    TEST_EXPECT_EQ(Table->GetCachedDesiredSize().Y, 3 * 20);
    TEST_EXPECT_EQ(Table->GetCachedDesiredSize().X, 0);

    TEST_SECTION("Clearing empties the table so it can be filled again");
    Table->ClearRows();
    TEST_EXPECT_EQ(Table->GetNumRows(), 0);

    Table->PrepareDesiredSize();
    TEST_EXPECT_EQ(Table->GetCachedDesiredSize().Y, 0);

    TEST_END();
}

bool PropertyTableColumnDrag_Test()
{
    TEST_BEGIN();

    FPropertyTable::FDesc Desc;
    Desc.Font                = CreateFont();
    Desc.RowHeight           = 20;
    Desc.LabelColumnFraction = 0.5f;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(Desc);
    Table->AddRow("Location", nullptr);
    Table->AddRow("Rotation", nullptr);
    Table->AddRow("Scale", nullptr);

    const FRectangle Bounds(IntVector2(0, 0), 200, 60);
    LayoutElement(Table, Bounds);

    TEST_SECTION("The fraction is held between the narrowest and the widest label column");
    Table->SetLabelColumnFraction(0.0f);
    TEST_EXPECT_EQ(Table->GetLabelColumnFraction(), FPropertyTable::MinLabelColumnFraction);

    Table->SetLabelColumnFraction(1.0f);
    TEST_EXPECT_EQ(Table->GetLabelColumnFraction(), FPropertyTable::MaxLabelColumnFraction);

    Table->SetLabelColumnFraction(0.5f);
    TEST_EXPECT_EQ(Table->GetLabelColumnFraction(), 0.5f);

    TEST_SECTION("The band the divider is grabbed by is centred on it and as tall as the rows");
    const FRectangle Divider = Table->GetDividerRectangle(Bounds);
    TEST_EXPECT_EQ(Divider.Width, FPropertyTable::DividerGrabWidth);
    TEST_EXPECT_EQ(Divider.Position.X, (Bounds.Width / 2) - (FPropertyTable::DividerGrabWidth / 2));
    TEST_EXPECT_EQ(Divider.Position.Y, Bounds.Position.Y);
    TEST_EXPECT_EQ(Divider.Height, 3 * 20);

    TEST_SECTION("A press on the band and a drag sideways carries the divider along with the cursor");
    TEST_EXPECT(Table->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(100, 30))).IsEventHandled());

    Table->OnMouseMove(MakeMoveEvent(IntVector2(140, 30)));
    TEST_EXPECT(Math::Abs(Table->GetLabelColumnFraction() - 0.7f) < 0.001f);
    TEST_EXPECT_EQ(Table->GetDividerRectangle(Bounds).Position.X, 140 - (FPropertyTable::DividerGrabWidth / 2));

    TEST_SECTION("Dragging past the end of the range clamps rather than running off");
    Table->OnMouseMove(MakeMoveEvent(IntVector2(2000, 30)));
    TEST_EXPECT_EQ(Table->GetLabelColumnFraction(), FPropertyTable::MaxLabelColumnFraction);

    TEST_SECTION("The release ends the drag, so the move after it leaves the divider alone");
    TEST_EXPECT(Table->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(2000, 30))).IsEventHandled());

    Table->OnMouseMove(MakeMoveEvent(IntVector2(20, 30)));
    TEST_EXPECT_EQ(Table->GetLabelColumnFraction(), FPropertyTable::MaxLabelColumnFraction);

    TEST_SECTION("A press away from the band starts nothing, so the drag after it moves nothing");
    Table->SetLabelColumnFraction(0.5f);
    TEST_EXPECT(!Table->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(20, 30))).IsEventHandled());

    Table->OnMouseMove(MakeMoveEvent(IntVector2(160, 30)));
    TEST_EXPECT_EQ(Table->GetLabelColumnFraction(), 0.5f);

    TEST_SECTION("A horizontal resize cursor is asked for on the divider and withheld off it");
    ECursor Cursor = ECursor::Arrow;

    Table->OnMouseMove(MakeMoveEvent(IntVector2(100, 30)));
    TEST_EXPECT(Table->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::ResizeEW);

    Table->OnMouseMove(MakeMoveEvent(IntVector2(20, 30)));
    TEST_EXPECT(!Table->GetCursor(Cursor));

    TEST_END();
}

bool PropertyTableToolTips_Test()
{
    TEST_BEGIN();

    FScopedStubApplication           Application;
    FScopedPropertyTableTestServices Services;

    const TSharedPtr<IFontFace> Font = CreateFont();

    FPropertyTable::FDesc Desc;
    Desc.Font                = Font;
    Desc.RowHeight           = 20;
    Desc.LabelColumnFraction = 0.5f;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(Desc);

    Table->AddRow("Location", nullptr).ToolTipText = "Where the actor sits in the world";
    Table->AddRow("Rotation", nullptr);
    Table->AddRow("Scale", nullptr).ToolTipText    = "How large the actor is drawn";

    LayoutElement(Table, FRectangle(IntVector2(0, 0), 200, 60));

    FToolTipService& ToolTips = FToolTipService::Get();

    TEST_SECTION("A point outside the rows belongs to no row");
    TEST_EXPECT_EQ(Table->FindRowAtPoint(IntVector2(20, 10)), 0);
    TEST_EXPECT_EQ(Table->FindRowAtPoint(IntVector2(20, 50)), 2);
    TEST_EXPECT_EQ(Table->FindRowAtPoint(IntVector2(20, 80)), FPropertyTable::InvalidRowIndex);
    TEST_EXPECT_EQ(Table->FindRowAtPoint(IntVector2(-20, 10)), FPropertyTable::InvalidRowIndex);

    TEST_SECTION("Resting on a row with a tip asks for one on the table's behalf");
    Table->OnMouseMove(MakeMoveEvent(IntVector2(20, 10)));
    TEST_EXPECT(ToolTips.IsPending());
    TEST_EXPECT_EQ(ToolTips.GetOwner(), StaticCastSharedPtr<FVisualElement>(Table));

    TEST_SECTION("Moving within the same row leaves the request that is already waiting alone");
    Table->OnMouseMove(MakeMoveEvent(IntVector2(60, 12)));
    TEST_EXPECT(ToolTips.IsPending());

    TEST_SECTION("A row that explains itself takes the previous tip down and asks for nothing");
    Table->OnMouseMove(MakeMoveEvent(IntVector2(20, 30)));
    TEST_EXPECT(!ToolTips.IsPending());
    TEST_EXPECT(ToolTips.GetOwner() == nullptr);

    TEST_SECTION("Crossing onto another row with a tip asks again, so the text follows the cursor down the table");
    Table->OnMouseMove(MakeMoveEvent(IntVector2(20, 50)));
    TEST_EXPECT(ToolTips.IsPending());

    TEST_SECTION("Leaving the table takes the tip with it");
    Table->OnMouseLeft(MakeMoveEvent(IntVector2(400, 400)));
    TEST_EXPECT(!ToolTips.IsPending());

    TEST_SECTION("Refilling the table drops a tip belonging to a row that is about to go");
    Table->OnMouseMove(MakeMoveEvent(IntVector2(20, 10)));
    TEST_EXPECT(ToolTips.IsPending());

    Table->ClearRows();
    TEST_EXPECT(!ToolTips.IsPending());

    TEST_END();
}

bool PropertyTableLayout_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FPropertyTable::FDesc Desc;
    Desc.Font                = Font;
    Desc.RowHeight           = 20;
    Desc.LabelColumnFraction = 0.5f;
    Desc.IndentPerLevel      = 12;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(Desc);

    TSharedPtr<FVisualElement> LocationEditor = CreateEditor("0, 0, 0", Font);
    TSharedPtr<FVisualElement> RotationEditor = CreateEditor("0, 0, 90", Font);
    TSharedPtr<FVisualElement> ScaleEditor    = CreateEditor("1, 1, 1", Font);

    Table->AddHeaderRow("Transform");
    Table->AddRow("Location", LocationEditor);
    Table->AddRow("Rotation", RotationEditor);

    FPropertyRow& ScaleRow = Table->AddRow("Scale", ScaleEditor);
    ScaleRow.IndentLevel   = 2;

    const FRectangle Bounds(IntVector2(10, 20), 200, 80);
    LayoutElement(Table, Bounds);

    TEST_SECTION("Every row sits one row height below the one before it, inside the bounds it was given");
    TEST_EXPECT_EQ(Table->GetRowRectangle(0, Bounds).Position, Bounds.Position);
    TEST_EXPECT_EQ(Table->GetRowRectangle(0, Bounds).Width, Bounds.Width);
    TEST_EXPECT_EQ(Table->GetRowRectangle(0, Bounds).Height, 20);

    for (int32 Index = 1; Index < Table->GetNumRows(); ++Index)
    {
        TEST_EXPECT_EQ(Table->GetRowRectangle(Index, Bounds).Position.Y, Table->GetRowRectangle(Index - 1, Bounds).GetBottom());
    }

    TEST_EXPECT_EQ(Table->GetRowRectangle(3, Bounds).GetBottom(), Bounds.GetBottom());

    TEST_SECTION("An index naming no row is an empty rectangle rather than a guess");
    TEST_EXPECT(Table->GetRowRectangle(-1, Bounds).IsEmpty());
    TEST_EXPECT(Table->GetRowRectangle(Table->GetNumRows(), Bounds).IsEmpty());

    TEST_SECTION("An editor is arranged into the right column, on the row that holds it, inset by the cell padding");
    const FRectangle LocationRowBounds = Table->GetRowRectangle(1, Bounds);
    const FMargin&   CellPadding       = FUIStyle::GetDefault().PropertyTable.CellPadding;

    TEST_EXPECT_EQ(LocationEditor->GetContentRectangle().Position.Y, LocationRowBounds.Position.Y + CellPadding.Top);
    TEST_EXPECT_EQ(LocationEditor->GetContentRectangle().Height, LocationRowBounds.Height - CellPadding.GetTotalVertical());
    TEST_EXPECT(LocationEditor->GetContentRectangle().Position.X >= Table->GetDividerRectangle(Bounds).GetRight());
    TEST_EXPECT(LocationEditor->GetContentRectangle().GetRight() <= Bounds.GetRight());

    TEST_SECTION("The editors are the table's children, in the order the rows hold them");
    TArray<TSharedPtr<FVisualElement>> Children;
    Table->GetChildren(Children);

    TEST_EXPECT_EQ(Children.Size(), 3);
    TEST_EXPECT(Children[0] == LocationEditor);
    TEST_EXPECT(Children[1] == RotationEditor);
    TEST_EXPECT(Children[2] == ScaleEditor);

    TEST_SECTION("A point in the right column finds the editor under it");
    FElementPath Path;
    Table->FindChildrenContainingPoint(RotationEditor->GetContentRectangle().GetCenter(), Path);

    TEST_EXPECT(Path.GetElements().Size() >= 2);
    TEST_EXPECT(Path.GetElements().Last() == RotationEditor);

    TEST_SECTION("Indenting a label moves it right and leaves the editor column where it was");
    TEST_EXPECT_EQ(ScaleEditor->GetContentRectangle().Position.X, RotationEditor->GetContentRectangle().Position.X);
    TEST_EXPECT_EQ(ScaleEditor->GetContentRectangle().Width, RotationEditor->GetContentRectangle().Width);

    FDrawCommandList Commands;
    DrawElement(Table, Commands);

    const int32 RotationLabelIndex = Commands.FindTextCommand("Rotation");
    const int32 ScaleLabelIndex    = Commands.FindTextCommand("Scale");
    TEST_EXPECT(RotationLabelIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(ScaleLabelIndex != FDrawCommandList::InvalidIndex);

    if (RotationLabelIndex != FDrawCommandList::InvalidIndex && ScaleLabelIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT_EQ(Commands[ScaleLabelIndex].Bounds.Position.X - Commands[RotationLabelIndex].Bounds.Position.X, 2 * Desc.IndentPerLevel);
    }

    TEST_END();
}
