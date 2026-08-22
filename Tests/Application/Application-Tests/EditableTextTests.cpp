#include "EditableTextTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Misc/Paths.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Input/Keys.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TrueTypeFontFace.h>
#include <Application/Elements/EditableText.h>

#if PLATFORM_MACOS
/** @brief The modifier a shortcut is spelled with here, which is the one FModifierKeyState calls the command. */
static constexpr EModifierFlag GCommandModifier = EModifierFlag::Super;

/** @brief The other platform's spelling of it, which is not a chord here. */
static constexpr EModifierFlag GForeignCommandModifier = EModifierFlag::Ctrl;
#else
static constexpr EModifierFlag GCommandModifier = EModifierFlag::Ctrl;

static constexpr EModifierFlag GForeignCommandModifier = EModifierFlag::Super;
#endif

static FKeyEvent CreateKeyDownEvent(FKey Key)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(), false, true);
}

static FKeyEvent CreateKeyDownEvent(FKey Key, EModifierFlag Modifiers)
{
    return FKeyEvent(EInputEventType::KeyDown, Key, FModifierKeyState(Modifiers), false, true);
}

static FKeyEvent CreateCharEvent(CHAR Character)
{
    return FKeyEvent(EInputEventType::KeyChar, Keys::Unknown, FModifierKeyState(), static_cast<uint32>(Character), false, true);
}

static FKeyEvent CreateCharEvent(CHAR Character, EModifierFlag Modifiers)
{
    return FKeyEvent(EInputEventType::KeyChar, Keys::Unknown, FModifierKeyState(Modifiers), static_cast<uint32>(Character), false, true);
}

static FCursorEvent CreateMouseButtonEvent(const IntVector2& ClientPosition, bool bIsDown)
{
    const EInputEventType EventType = bIsDown ? EInputEventType::MouseButtonDown : EInputEventType::MouseButtonUp;
    return FCursorEvent(EventType, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(), bIsDown);
}

static FCursorEvent CreateMouseMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static TSharedPtr<FEditableText> CreateEditableText(const CHAR* Text, const TSharedPtr<IFontFace>& Font)
{
    FEditableText::FDesc Desc;
    Desc.Text     = Text;
    Desc.HintText = "Hint";
    Desc.Font     = Font;
    Desc.Padding  = FMargin(0);

    Desc.TextCursorBlinkPeriod = 0.0f;

    return FEditableText::Create(Desc);
}

bool EditableTextEditing_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TEST_SECTION("A new element starts with the text cursor past the initial text");
    TSharedPtr<FEditableText> Editable = CreateEditableText("Ab", Font);
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

    TSharedPtr<FEditableText> Watched = CreateEditableText("", Font);
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

    TSharedPtr<FEditableText> Editable = CreateEditableText("Hello", Font);

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
    TSharedPtr<FEditableText> Typed = CreateEditableText("", Font);

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
    TSharedPtr<FEditableText> Empty = CreateEditableText("", Font);

    const IntVector2 DesiredSize = Empty->PrepareDesiredSize();
    TEST_EXPECT_EQ(DesiredSize.X, 4 * 8);
    TEST_EXPECT_EQ(DesiredSize.Y, 16);

    TEST_END();
}

bool EditableTextKeyInterceptor_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableText> Editable = CreateEditableText("Hello", Font);
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

bool EditableTextSelection_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableText> Editable = CreateEditableText("Hello", Font);

    TEST_SECTION("Nothing is selected to begin with");
    TEST_EXPECT(!Editable->HasSelection());
    TEST_EXPECT(Editable->GetSelectedText().IsEmpty());

    TEST_SECTION("Shift with an arrow drags a range out from where the text cursor was");
    Editable->MoveTextCursorToStart();
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));

    TEST_EXPECT(Editable->HasSelection());
    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 0);
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 2);
    TEST_EXPECT(Editable->GetSelectedText().Equals("He"));

    TEST_SECTION("Shifting back the other way shrinks the range again");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Shift));
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 1);

    TEST_SECTION("Shift-End reaches the end of the text");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::End, EModifierFlag::Shift));
    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 0);
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 5);

    TEST_SECTION("An unshifted arrow drops the selection onto the edge it moved towards");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left));
    TEST_EXPECT(!Editable->HasSelection());
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    TEST_SECTION("Select all covers the text, under the modifier this platform starts a chord with");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::A, GCommandModifier));
    TEST_EXPECT(Editable->GetSelectedText().Equals("Hello"));

    TEST_SECTION("The other platform's spelling of it selects nothing here");
    Editable->ClearSelection();
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::A, GForeignCommandModifier));
    TEST_EXPECT(!Editable->HasSelection());

    TEST_SECTION("Typing over a selection replaces it");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::A, GCommandModifier));
    Editable->OnKeyChar(CreateCharEvent('X'));
    TEST_EXPECT(Editable->GetText().Equals("X"));
    TEST_EXPECT(!Editable->HasSelection());
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 1);

    TEST_SECTION("Backspace removes a selection instead of the character in front of it");
    TSharedPtr<FEditableText> Deleted = CreateEditableText("Hello", Font);
    Deleted->SetTextCursorPosition(1);
    Deleted->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));
    Deleted->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));
    Deleted->OnKeyDown(CreateKeyDownEvent(Keys::Backspace));

    TEST_EXPECT(Deleted->GetText().Equals("Hlo"));
    TEST_EXPECT_EQ(Deleted->GetTextCursorPosition(), 1);
    TEST_EXPECT(!Deleted->HasSelection());

    TEST_SECTION("Delete does the same");
    Deleted->SelectAll();
    Deleted->OnKeyDown(CreateKeyDownEvent(Keys::Delete));
    TEST_EXPECT(Deleted->GetText().IsEmpty());

    TEST_SECTION("Moving the text cursor without a modifier never leaves a range behind");
    TSharedPtr<FEditableText> Moved = CreateEditableText("Hello", Font);
    Moved->SelectAll();
    Moved->MoveTextCursorToStart();
    TEST_EXPECT(!Moved->HasSelection());

    TEST_SECTION("A selection is drawn as a fill behind the text, spanning the selected characters");
    TSharedPtr<FEditableText> Drawn = CreateEditableText("Hello", Font);
    Drawn->PrepareDesiredSize();
    Drawn->Tick(FRectangle(IntVector2(0, 0), 200, 16));

    FDrawCommandList CommandList;
    Drawn->OnDraw(FDrawGeometry(Drawn->GetContentRectangle(), 1.0f), CommandList, 0);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 0);

    Drawn->SetTextCursorPosition(1);
    Drawn->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));
    Drawn->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));

    CommandList.Reset();
    Drawn->OnDraw(FDrawGeometry(Drawn->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 1);
    TEST_EXPECT(CommandList[0].Type == EDrawCommandType::Box);
    TEST_EXPECT_EQ(CommandList[0].Bounds.Position.X, 1 * 8);
    TEST_EXPECT_EQ(CommandList[0].Bounds.Width, 2 * 8);
    TEST_EXPECT_EQ(CommandList[0].Bounds.Height, 16);

    TEST_SECTION("The fill is recorded before the glyphs, so it stays behind them");
    TEST_EXPECT_EQ(CommandList.FindTextCommand("Hello"), 1);

    TEST_SECTION("Losing focus drops the selection along with the text cursor");
    Drawn->OnFocusGained();
    Drawn->SelectAll();
    Drawn->OnFocusLost();
    TEST_EXPECT(!Drawn->HasSelection());

    TEST_END();
}

bool EditableTextMouseSelection_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableText> Editable = CreateEditableText("Hello", Font);
    Editable->PrepareDesiredSize();
    Editable->Tick(FRectangle(IntVector2(20, 10), 200, 16));

    TEST_SECTION("A press puts the text cursor under the cursor position");
    TEST_EXPECT(Editable->OnMouseButtonDown(CreateMouseButtonEvent(IntVector2(20 + (3 * 8), 12), true)).IsEventHandled());
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 3);
    TEST_EXPECT(!Editable->HasSelection());

    TEST_SECTION("Dragging from there extends a selection");
    TEST_EXPECT(Editable->OnMouseMove(CreateMouseMoveEvent(IntVector2(20 + (5 * 8), 12))).IsEventHandled());
    TEST_EXPECT(Editable->HasSelection());
    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 3);
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 5);

    TEST_SECTION("Releasing ends the drag, so later moves leave the selection alone");
    TEST_EXPECT(Editable->OnMouseButtonUp(CreateMouseButtonEvent(IntVector2(20 + (5 * 8), 12), false)).IsEventHandled());
    TEST_EXPECT(!Editable->OnMouseMove(CreateMouseMoveEvent(IntVector2(20, 12))).IsEventHandled());
    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 3);
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 5);

    TEST_SECTION("A press outside the element is left for whatever is under it");
    TEST_EXPECT(!Editable->OnMouseButtonDown(CreateMouseButtonEvent(IntVector2(20, 400), true)).IsEventHandled());
    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 3);
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 5);

    TEST_SECTION("Shift-clicking extends from where the text cursor already was");
    Editable->SetTextCursorPosition(1);
    Editable->OnMouseButtonDown(FCursorEvent(EInputEventType::MouseButtonDown, Keys::MouseButtonLeft, IntVector2(20 + (4 * 8), 12), IntVector2(0, 0), FModifierKeyState(EModifierFlag::Shift), true));

    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 1);
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 4);

    TEST_END();
}

bool EditableTextWordNavigation_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);
    TSharedPtr<FEditableText> Editable = CreateEditableText("RHI.Type Metal", Font);

    TEST_SECTION("A word ends at a space rather than at a dot");
    TEST_EXPECT_EQ(Editable->FindWordBoundaryLeft(14), 9);
    TEST_EXPECT_EQ(Editable->FindWordBoundaryLeft(9), 0);
    TEST_EXPECT_EQ(Editable->FindWordBoundaryLeft(0), 0);

    TEST_EXPECT_EQ(Editable->FindWordBoundaryRight(0), 8);
    TEST_EXPECT_EQ(Editable->FindWordBoundaryRight(8), 14);
    TEST_EXPECT_EQ(Editable->FindWordBoundaryRight(14), 14);

    TEST_SECTION("Both spellings of the word jump reach the same boundary");
    Editable->MoveTextCursorToEnd();
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Alt));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 9);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Ctrl));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Ctrl));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 8);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Alt));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 14);

    TEST_SECTION("An unshifted word jump leaves no selection behind");
    TEST_EXPECT(!Editable->HasSelection());

    TEST_SECTION("Alt and Shift together select one word, and a repeat takes the next");
    Editable->MoveTextCursorToEnd();
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Alt | EModifierFlag::Shift));

    TEST_EXPECT(Editable->GetSelectedText().Equals("Metal"));

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Alt | EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("RHI.Type Metal"));

    TEST_SECTION("Selecting forward a word at a time does the same from the other end");
    Editable->MoveTextCursorToStart();
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Ctrl | EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("RHI.Type"));

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Ctrl | EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("RHI.Type Metal"));

    TEST_SECTION("An unshifted jump out of a selection starts from the near edge of it");
    Editable->MoveTextCursorToStart();
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Shift));
    TEST_EXPECT_EQ(Editable->GetSelectionStart(), 0);
    TEST_EXPECT_EQ(Editable->GetSelectionEnd(), 2);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Alt));
    TEST_EXPECT(!Editable->HasSelection());
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 8);

    TEST_SECTION("Cmd takes the caret to the end of the line instead");
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Super));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 0);

    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Super));
    TEST_EXPECT_EQ(Editable->GetTextCursorPosition(), 14);

    TEST_SECTION("Cmd and Shift select the whole row, the way Shift with Home and End does");
    Editable->SetTextCursorPosition(9);
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Super | EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("RHI.Type "));

    Editable->SetTextCursorPosition(9);
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Home, EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("RHI.Type "));

    Editable->SetTextCursorPosition(9);
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::Right, EModifierFlag::Super | EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("Metal"));

    Editable->SetTextCursorPosition(9);
    Editable->OnKeyDown(CreateKeyDownEvent(Keys::End, EModifierFlag::Shift));
    TEST_EXPECT(Editable->GetSelectedText().Equals("Metal"));

    TEST_SECTION("Backspace and delete take a whole word under the same modifiers");
    TSharedPtr<FEditableText> Deleted = CreateEditableText("RHI.Type Metal", Font);
    Deleted->OnKeyDown(CreateKeyDownEvent(Keys::Backspace, EModifierFlag::Alt));
    TEST_EXPECT(Deleted->GetText().Equals("RHI.Type "));

    Deleted->MoveTextCursorToStart();
    Deleted->OnKeyDown(CreateKeyDownEvent(Keys::Delete, EModifierFlag::Ctrl));
    TEST_EXPECT(Deleted->GetText().Equals(" "));

    TEST_SECTION("The arrows are still resolved when the chord is one nothing else claims");
    TSharedPtr<FEditableText> Unclaimed = CreateEditableText("Hello", Font);
    Unclaimed->MoveTextCursorToEnd();

    TEST_EXPECT(Unclaimed->OnKeyDown(CreateKeyDownEvent(Keys::Left, EModifierFlag::Ctrl)).IsEventHandled());
    TEST_EXPECT_EQ(Unclaimed->GetTextCursorPosition(), 0);

    TEST_SECTION("A chord the field has no use for is left to whatever else is listening");
    TEST_EXPECT(!Unclaimed->OnKeyDown(CreateKeyDownEvent(Keys::S, EModifierFlag::Super)).IsEventHandled());
    TEST_EXPECT(Unclaimed->GetText().Equals("Hello"));

    TEST_END();
}

bool EditableTextCommandChord_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableText> Editable = CreateEditableText("Hello", Font);

    TEST_SECTION("A character that arrives under a command chord is not typed");
    TEST_EXPECT(!Editable->OnKeyChar(CreateCharEvent('a', EModifierFlag::Super)).IsEventHandled());
    TEST_EXPECT(!Editable->OnKeyChar(CreateCharEvent('a', EModifierFlag::Ctrl)).IsEventHandled());
    TEST_EXPECT(Editable->GetText().Equals("Hello"));

    TEST_SECTION("AltGr reports as Ctrl and Alt together and does produce text");
    TEST_EXPECT(Editable->OnKeyChar(CreateCharEvent('@', EModifierFlag::Ctrl | EModifierFlag::Alt)).IsEventHandled());
    TEST_EXPECT(Editable->GetText().Equals("Hello@"));

    TEST_SECTION("Shift is not a chord, so a capital still arrives as text");
    TEST_EXPECT(Editable->OnKeyChar(CreateCharEvent('A', EModifierFlag::Shift)).IsEventHandled());
    TEST_EXPECT(Editable->GetText().Equals("Hello@A"));

    TEST_SECTION("Select all survives the character macOS sends after the chord");
    TSharedPtr<FEditableText> Selected = CreateEditableText("Hello", Font);

    Selected->OnKeyDown(CreateKeyDownEvent(Keys::A, GCommandModifier));
    Selected->OnKeyChar(CreateCharEvent('a', GCommandModifier));

    TEST_EXPECT(Selected->GetText().Equals("Hello"));
    TEST_EXPECT(Selected->GetSelectedText().Equals("Hello"));

    TEST_SECTION("And the control character Windows sends in its place");
    TSharedPtr<FEditableText> Control = CreateEditableText("Hello", Font);

    Control->OnKeyDown(CreateKeyDownEvent(Keys::A, GCommandModifier));
    Control->OnKeyChar(CreateCharEvent(static_cast<CHAR>(0x01), GCommandModifier));

    TEST_EXPECT(Control->GetText().Equals("Hello"));
    TEST_EXPECT(Control->GetSelectedText().Equals("Hello"));

    TEST_SECTION("AltGr composes a character even on the keys a chord is spelled with");
    TSharedPtr<FEditableText> Composed = CreateEditableText("Hello", Font);
    Composed->SelectAll();

    TEST_EXPECT(!Composed->OnKeyDown(CreateKeyDownEvent(Keys::X, EModifierFlag::Ctrl | EModifierFlag::Alt)).IsEventHandled());
    TEST_EXPECT(Composed->GetText().Equals("Hello"));

    TEST_EXPECT(Composed->OnKeyChar(CreateCharEvent('z', EModifierFlag::Ctrl | EModifierFlag::Alt)).IsEventHandled());
    TEST_EXPECT(Composed->GetText().Equals("z"));

    TEST_SECTION("While the chord itself still takes the text away");
    TSharedPtr<FEditableText> Cut = CreateEditableText("Hello", Font);
    Cut->SelectAll();

    TEST_EXPECT(Cut->OnKeyDown(CreateKeyDownEvent(Keys::X, GCommandModifier)).IsEventHandled());
    TEST_EXPECT(Cut->GetText().IsEmpty());

    TEST_END();
}

bool EditableTextCaretBlink_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    FEditableText::FDesc Desc;
    Desc.Text    = "Hello";
    Desc.Font    = Font;
    Desc.Padding = FMargin(0);

    TSharedPtr<FEditableText> Editable = FEditableText::Create(Desc);

    TEST_SECTION("The default period is the rate ImGui blinks a caret at");
    TEST_EXPECT(Editable->GetTextCursorBlinkPeriod() > 1.19 && Editable->GetTextCursorBlinkPeriod() < 1.21);

    TEST_SECTION("The cursor is on for the first two thirds of every period");
    TEST_EXPECT(Editable->IsTextCursorVisibleAt(0.0));
    TEST_EXPECT(Editable->IsTextCursorVisibleAt(0.5));
    TEST_EXPECT(!Editable->IsTextCursorVisibleAt(0.9));
    TEST_EXPECT(!Editable->IsTextCursorVisibleAt(1.1));

    TEST_SECTION("And the phase repeats rather than running out");
    TEST_EXPECT(Editable->IsTextCursorVisibleAt(1.3));
    TEST_EXPECT(Editable->IsTextCursorVisibleAt(1.7));
    TEST_EXPECT(!Editable->IsTextCursorVisibleAt(2.1));
    TEST_EXPECT(Editable->IsTextCursorVisibleAt(12.1));

    TEST_SECTION("A period of zero leaves the cursor solid");
    TSharedPtr<FEditableText> Solid = CreateEditableText("Hello", Font);

    TEST_EXPECT(Solid->IsTextCursorVisibleAt(0.0));
    TEST_EXPECT(Solid->IsTextCursorVisibleAt(0.9));
    TEST_EXPECT(Solid->IsTextCursorVisibleAt(1000.0));

    TEST_SECTION("A solid cursor draws for as long as the element has focus");
    Solid->PrepareDesiredSize();
    Solid->Tick(FRectangle(IntVector2(0, 0), 200, 16));
    Solid->OnFocusGained();

    FDrawCommandList CommandList;
    Solid->OnDraw(FDrawGeometry(Solid->GetContentRectangle(), 1.0f), CommandList, 0);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Line), 1);

    TEST_SECTION("The layer is claimed for as long as the element has focus, whatever the phase");

    // A period short enough that the draws below land all over it, so the layer is checked on and off
    FEditableText::FDesc BlinkingDesc;
    BlinkingDesc.Text                  = "Hello";
    BlinkingDesc.Font                  = Font;
    BlinkingDesc.Padding               = FMargin(0);
    BlinkingDesc.TextCursorBlinkPeriod = 0.000001f;

    TSharedPtr<FEditableText> Blinking = FEditableText::Create(BlinkingDesc);
    Blinking->PrepareDesiredSize();
    Blinking->Tick(FRectangle(IntVector2(0, 0), 200, 16));
    Blinking->OnFocusGained();

    for (int32 Index = 0; Index < 64; ++Index)
    {
        CommandList.Reset();
        TEST_EXPECT_EQ(Blinking->OnDraw(FDrawGeometry(Blinking->GetContentRectangle(), 1.0f), CommandList, 4), 5);
    }

    TEST_SECTION("An unfocused element hands the layer straight back");
    Blinking->OnFocusLost();

    CommandList.Reset();
    TEST_EXPECT_EQ(Blinking->OnDraw(FDrawGeometry(Blinking->GetContentRectangle(), 1.0f), CommandList, 4), 4);

    TEST_END();
}

bool EditableTextBandAlignment_Test()
{
    TEST_BEGIN();

    TSharedPtr<FTrueTypeFontFace> Font = FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", 16);
    TEST_EXPECT(Font != nullptr);

    if (!Font)
    {
        TEST_END();
    }

    TEST_SECTION("The band the glyphs occupy is shorter than the line the face asks for");
    TEST_EXPECT(Font->GetTextBandHeight() < Font->GetLineHeight());

    TSharedPtr<FEditableText> Editable = CreateEditableText("lg", Font);
    Editable->OnFocusGained();

    const int32 BoxHeight = Font->GetTextBandHeight() + 8;
    Editable->PrepareDesiredSize();
    Editable->Tick(FRectangle(IntVector2(0, 0), 200, BoxHeight));

    FDrawCommandList CommandList;
    Editable->OnDraw(FDrawGeometry(Editable->GetContentRectangle(), 1.0f), CommandList, 0);

    int32 TextIndex       = FDrawCommandList::InvalidIndex;
    int32 TextCursorIndex = FDrawCommandList::InvalidIndex;
    for (int32 Index = 0; Index < CommandList.Size(); ++Index)
    {
        if (CommandList[Index].Type == EDrawCommandType::Text)
        {
            TextIndex = Index;
        }
        else if (CommandList[Index].Type == EDrawCommandType::Line)
        {
            TextCursorIndex = Index;
        }
    }

    TEST_EXPECT(TextIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(TextCursorIndex != FDrawCommandList::InvalidIndex);

    if (TextIndex == FDrawCommandList::InvalidIndex || TextCursorIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    const FRectangle TextCursorBounds = CommandList[TextCursorIndex].Bounds;

    TEST_SECTION("The text cursor is the capitals with the descent added above and below them");
    TEST_EXPECT_EQ(TextCursorBounds.Height, Font->GetCapHeight() + (Font->GetDescent() * 2));

    TEST_SECTION("Which leaves it centred in the box, give or take the pixel a halved odd number drops");
    const int32 SpaceAbove = TextCursorBounds.Position.Y;
    const int32 SpaceBelow = BoxHeight - TextCursorBounds.GetBottom();
    TEST_EXPECT(SpaceBelow - SpaceAbove == 0 || SpaceBelow - SpaceAbove == 1);

    FDrawCommandList TextOnlyList;
    TextOnlyList.AddText(0, CommandList[TextIndex].Bounds, String("lg"), Font.Get(), FFloatColor::White);

    FUIDrawData DrawData;
    DrawData.BuildFromCommandList(TextOnlyList);

    TEST_EXPECT(!DrawData.GetVertices().IsEmpty());

    if (DrawData.GetVertices().IsEmpty())
    {
        TEST_END();
    }

    float InkTop    = static_cast<float>(BoxHeight);
    float InkBottom = 0.0f;
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        InkTop    = Math::Min(InkTop, Vertex.Position.Y);
        InkBottom = Math::Max(InkBottom, Vertex.Position.Y);
    }

    TEST_SECTION("An ascender and a descender both stay inside it, so the two cannot drift apart");
    TEST_EXPECT(InkTop >= static_cast<float>(TextCursorBounds.Position.Y));
    TEST_EXPECT(InkBottom <= static_cast<float>(TextCursorBounds.GetBottom()));

    TEST_SECTION("And a line of capitals ends up with the same room above it as below it");

    FDrawCommandList CapitalsList;
    CapitalsList.AddText(0, CommandList[TextIndex].Bounds, String("HI"), Font.Get(), FFloatColor::White);
    DrawData.BuildFromCommandList(CapitalsList);

    float CapitalTop    = static_cast<float>(BoxHeight);
    float CapitalBottom = 0.0f;
    for (const FUIVertex& Vertex : DrawData.GetVertices())
    {
        CapitalTop    = Math::Min(CapitalTop, Vertex.Position.Y);
        CapitalBottom = Math::Max(CapitalBottom, Vertex.Position.Y);
    }

    TEST_EXPECT(CapitalTop == static_cast<float>(BoxHeight) - CapitalBottom);

    TEST_END();
}

bool EditableTextDraw_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = MakeSharedPtr<FFixedWidthFontFace>(8, 16);

    TSharedPtr<FEditableText> Editable = CreateEditableText("Hello", Font);
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
    TSharedPtr<FEditableText> Empty = CreateEditableText("", Font);
    Empty->PrepareDesiredSize();
    Empty->Tick(FRectangle(IntVector2(0, 0), 200, 16));

    CommandList.Reset();
    Empty->OnDraw(FDrawGeometry(Empty->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.FindTextCommand("Hint"), 0);

    TEST_END();
}
