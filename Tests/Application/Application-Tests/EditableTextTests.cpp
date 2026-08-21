#include "EditableTextTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Draw/DrawCommandList.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Elements/EditableTextElement.h>

static FKeyEvent CreateKeyDownEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

static FKeyEvent CreateCharEvent(CHAR Character)
{
    return FKeyEvent(EInputEventType::KeyChar, Keys::Unknown, FModifierKeyState(), static_cast<uint32>(Character), false, true);
}

static TSharedPtr<FEditableTextElement> CreateEditableText(const CHAR* Text, const TSharedPtr<IFontFace>& Font)
{
    FEditableTextElement::FInitializer Initializer;
    Initializer.Text     = Text;
    Initializer.HintText = "Hint";
    Initializer.Font     = Font;
    Initializer.Padding  = FMargin(0);
    return FEditableTextElement::Create(Initializer);
}

bool EditableTextEditing_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TEST_SECTION("A new element starts with the text cursor past the initial text");
    TSharedPtr<FEditableTextElement> Editable = CreateEditableText("Ab", Font);
    TEST_EXPECT(Editable->GetText().Equals("Ab"));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 2);

    TEST_SECTION("Inserting at the end appends");
    Editable->InsertCharacter('c');
    TEST_EXPECT(Editable->GetText().Equals("Abc"));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 3);

    TEST_SECTION("Inserting in the middle splits the text");
    Editable->SetTextCursorPosition(1);
    Editable->InsertText(StringView("XY"));
    TEST_EXPECT(Editable->GetText().Equals("AXYbc"));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 3);

    TEST_SECTION("Backspace removes the character before the text cursor");
    TEST_EXPECT(Editable->DeleteBackward());
    TEST_EXPECT(Editable->GetText().Equals("AXbc"));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 2);

    TEST_SECTION("Backspace at the start does nothing");
    Editable->MoveTextCursorToStart();
    TEST_EXPECT(!Editable->DeleteBackward());
    TEST_EXPECT(Editable->GetText().Equals("AXbc"));

    TEST_SECTION("Delete removes the character after the text cursor and leaves it put");
    TEST_EXPECT(Editable->DeleteForward());
    TEST_EXPECT(Editable->GetText().Equals("Xbc"));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    TEST_SECTION("Delete at the end does nothing");
    Editable->MoveTextCursorToEnd();
    TEST_EXPECT(!Editable->DeleteForward());
    TEST_EXPECT(Editable->GetText().Equals("Xbc"));

    TEST_SECTION("Replacing a range leaves the text cursor after the replacement");
    Editable->ReplaceRange(0, 1, StringView("Long"));
    TEST_EXPECT(Editable->GetText().Equals("Longbc"));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 4);

    TEST_SECTION("Clearing empties the text and resets the text cursor");
    Editable->ClearText();
    TEST_EXPECT(Editable->GetText().IsEmpty());
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    TEST_SECTION("A change fires the delegate, and a silent set does not");
    int32 NumChanges = 0;

    TSharedPtr<FEditableTextElement> Watched = CreateEditableText("", Font);
    Watched->GetOnTextChanged().BindLambda([&NumChanges](const String&)
    {
        NumChanges++;
    });

    Watched->InsertCharacter('a');
    TEST_EXPECT_EQ(NumChanges, 1);

    Watched->SetTextSilently("Silent");
    TEST_EXPECT_EQ(NumChanges, 1);
    TEST_EXPECT(Watched->GetText().Equals("Silent"));

    TEST_END();
}

bool EditableTextCursor_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableTextElement> Editable = CreateEditableText("Hello", Font);

    TEST_SECTION("The text cursor clamps into the text");
    Editable->SetTextCursorPosition(-10);
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    Editable->SetTextCursorPosition(500);
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 5);

    TEST_SECTION("Left and right stop at the ends");
    Editable->MoveTextCursorRight();
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 5);

    Editable->MoveTextCursorLeft();
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 4);

    Editable->MoveTextCursorToStart();
    Editable->MoveTextCursorLeft();
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    TEST_SECTION("Home and End arrive as key events");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::End));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 5);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Home));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 1);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    TEST_SECTION("Printable characters are accepted and control characters are not");
    TSharedPtr<FEditableTextElement> Typed = CreateEditableText("", Font);

    TEST_EXPECT(Typed->OnKeyChar(CreateCharEvent('A')).IsEventHandled());
    TEST_EXPECT(Typed->GetText().Equals("A"));

    TEST_EXPECT(!Typed->OnKeyChar(CreateCharEvent('\t')).IsEventHandled());
    TEST_EXPECT(!Typed->OnKeyChar(CreateCharEvent('\n')).IsEventHandled());
    TEST_EXPECT(!Typed->OnKeyChar(CreateCharEvent(static_cast<CHAR>(0x7F))).IsEventHandled());
    TEST_EXPECT(Typed->GetText().Equals("A"));

    TEST_SECTION("Backspace and delete arrive as key events");
    Typed->OnKeyDown(CreateKeyDownEvent(Keys::Backspace));
    TEST_EXPECT(Typed->GetText().IsEmpty());

    TEST_SECTION("The desired size never collapses below the hint");
    TSharedPtr<FEditableTextElement> Empty = CreateEditableText("", Font);

    const IntVector2 DesiredSize = Empty->PrepareDesiredSize();
    TEST_EXPECT_EQ(DesiredSize.X, 4 * 8);
    TEST_EXPECT_EQ(DesiredSize.Y, 16);

    TEST_END();
}

bool EditableTextKeyInterceptor_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableTextElement> Editable = CreateEditableText("Hello", Font);
    Editable->SetTextCursorPosition(5);

    int32 NumIntercepted = 0;
    bool  bClaimKeys     = true;

    Editable->GetOnKeyDownInterceptor().BindLambda([&NumIntercepted, &bClaimKeys](const FKeyEvent&) -> EKeyInterceptResult
    {
        NumIntercepted++;
        return bClaimKeys ? EKeyInterceptResult::Handled : EKeyInterceptResult::NotHandled;
    });

    TEST_SECTION("A claimed key never reaches the default behavior");
    TEST_EXPECT(Editable->OnKeyDown(CreateKeyDownEvent(Keys::Backspace)).IsEventHandled());
    TEST_EXPECT_EQ(NumIntercepted, 1);
    TEST_EXPECT(Editable->GetText().Equals("Hello"));

    TEST_SECTION("An unclaimed key falls through to the default behavior");
    bClaimKeys = false;

    TEST_EXPECT(Editable->OnKeyDown(CreateKeyDownEvent(Keys::Backspace)).IsEventHandled());
    TEST_EXPECT_EQ(NumIntercepted, 2);
    TEST_EXPECT(Editable->GetText().Equals("Hell"));

    TEST_SECTION("Enter reaches the commit delegate when it is not claimed");
    int32 NumCommits = 0;
    Editable->GetOnTextCommitted().BindLambda([&NumCommits](const String&)
    {
        NumCommits++;
    });

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Enter));
    TEST_EXPECT_EQ(NumCommits, 1);

    TEST_SECTION("A claimed Enter never commits");
    bClaimKeys = true;
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Enter));
    TEST_EXPECT_EQ(NumCommits, 1);

    TEST_END();
}

bool EditableTextDraw_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableTextElement> Editable = CreateEditableText("Hello", Font);
    Editable->PrepareDesiredSize();
    Editable->Tick(FRectangle(IntVector2(0, 0), 200, 16));

    TEST_SECTION("An unfocused element draws its text without a cursor");
    FDrawCommandList CommandList;
    Editable->OnDraw(FDrawGeometry(Editable->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.FindTextCommand("Hello"), 0);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Line), 0);

    TEST_SECTION("A focused element draws the cursor at the width of the text in front of it");
    Editable->OnFocusGained();
    TEST_EXPECT(Editable->HasKeyboardFocus());

    Editable->SetTextCursorPosition(3);

    CommandList.Reset();
    Editable->OnDraw(FDrawGeometry(Editable->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Line), 1);
    TEST_EXPECT_EQ(CommandList[1].Bounds.Position.X, 3 * 8);
    TEST_EXPECT_EQ(CommandList[1].Bounds.Width, 1);
    TEST_EXPECT_EQ(CommandList[1].Bounds.Height, 16);

    TEST_SECTION("Losing focus hides the text cursor again");
    Editable->OnFocusLost();
    TEST_EXPECT(!Editable->HasKeyboardFocus());

    CommandList.Reset();
    Editable->OnDraw(FDrawGeometry(Editable->GetContentRectangle(), 1.0f), CommandList, 0);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Line), 0);

    TEST_SECTION("An empty element draws the hint instead of the text");
    TSharedPtr<FEditableTextElement> Empty = CreateEditableText("", Font);
    Empty->PrepareDesiredSize();
    Empty->Tick(FRectangle(IntVector2(0, 0), 200, 16));

    CommandList.Reset();
    Empty->OnDraw(FDrawGeometry(Empty->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.FindTextCommand("Hint"), 0);

    TEST_END();
}
