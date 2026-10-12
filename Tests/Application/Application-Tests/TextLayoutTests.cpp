#include "TextLayoutTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/TextLayout.h>

/** @brief How wide one character is in the face every test here uses. */
constexpr int32 GAdvance = 8;

/** @brief How tall one line is in that face. */
constexpr int32 GLineHeight = 16;

static TSharedPtr<FFixedWidthFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(GAdvance, GLineHeight);
}

/** @brief The text of every laid-out run concatenated, which drops what wrapping did not carry over. */
static FString ConcatenateRuns(const FTextLayout& Layout)
{
    FString Result;
    for (const FTextRun& Run : Layout.GetRuns())
    {
        Result.Append(Run.Text);
    }

    return Result;
}

/** @brief The text of the runs on one line. */
static FString LineText(const FTextLayout& Layout, int32 LineIndex)
{
    const FTextLine& Line = Layout.GetLines()[LineIndex];

    FString Result;
    for (int32 Index = 0; Index < Line.RunCount; ++Index)
    {
        Result.Append(Layout.GetRuns()[Line.FirstRunIndex + Index].Text);
    }

    return Result;
}

bool TextLayoutBasics_Test()
{
    TEST_BEGIN();

    TSharedPtr<FFixedWidthFontFace> Font = CreateFont();

    FTextLayout Layout;

    TEST_SECTION(TEXT("A layout starts with nothing in it"));
    TEST_EXPECT(Layout.GetLines().IsEmpty());
    TEST_EXPECT(Layout.GetRuns().IsEmpty());
    TEST_EXPECT_EQ(Layout.GetCharacterCount(), 0);
    TEST_EXPECT(Layout.GetSize() == FIntVector2(0, 0));

    TEST_SECTION(TEXT("Laying out no text still leaves one line, so a caret has somewhere to sit"));
    Layout.WrapToWidth(0);
    TEST_EXPECT_EQ(Layout.GetLines().Size(), 1);
    TEST_EXPECT_EQ(Layout.GetSize().X, 0);

    TEST_SECTION(TEXT("One run is one line as wide as its characters"));
    Layout.Clear();
    Layout.AppendRun(FTextRun(FString(TEXT("Hello")), Font.Get(), FFloatColor::White));
    Layout.WrapToWidth(0);

    TEST_EXPECT_EQ(Layout.GetLines().Size(), 1);
    TEST_EXPECT_EQ(Layout.GetRuns().Size(), 1);
    TEST_EXPECT(Layout.GetSize() == FIntVector2(5 * GAdvance, GLineHeight));
    TEST_EXPECT_EQ(Layout.GetLines()[0].Width, 5 * GAdvance);
    TEST_EXPECT_EQ(Layout.GetLines()[0].Height, GLineHeight);
    TEST_EXPECT_EQ(Layout.GetLines()[0].Baseline, Font->GetAscent());

    TEST_SECTION(TEXT("Two runs sit side by side on the same line, which is what a styled sentence needs"));
    Layout.AppendRun(FTextRun(FString(TEXT(" World")), Font.Get(), FFloatColor::Red));
    Layout.WrapToWidth(0);

    TEST_EXPECT_EQ(Layout.GetLines().Size(), 1);
    TEST_EXPECT_EQ(Layout.GetRuns().Size(), 2);
    TEST_EXPECT_EQ(Layout.GetLines()[0].RunCount, 2);
    TEST_EXPECT(Layout.GetSize() == FIntVector2(11 * GAdvance, GLineHeight));

    TEST_SECTION(TEXT("Each run keeps its own color rather than the last one winning"));
    TEST_EXPECT(Layout.GetRuns()[0].Tint == FFloatColor::White);
    TEST_EXPECT(Layout.GetRuns()[1].Tint == FFloatColor::Red);

    TEST_SECTION(TEXT("The concatenated text is what the character indices refer to"));
    TEST_EXPECT(Layout.GetText() == TEXT("Hello World"));
    TEST_EXPECT_EQ(Layout.GetCharacterCount(), 11);

    TEST_SECTION(TEXT("Appending after a layout drops it, so a stale line count is never read back"));
    Layout.AppendRun(FTextRun(FString(TEXT("!")), Font.Get(), FFloatColor::White));
    TEST_EXPECT(Layout.GetLines().IsEmpty());
    TEST_EXPECT(Layout.GetSize() == FIntVector2(0, 0));

    TEST_SECTION(TEXT("A run with no face measures to nothing but still counts as text"));
    FTextLayout FacelessLayout;
    FacelessLayout.AppendRun(FTextRun(FString(TEXT("Hello")), nullptr, FFloatColor::White));
    FacelessLayout.WrapToWidth(0);

    TEST_EXPECT_EQ(FacelessLayout.GetSize().X, 0);
    TEST_EXPECT_EQ(FacelessLayout.GetCharacterCount(), 5);

    TEST_SECTION(TEXT("Clear puts it back to empty"));
    Layout.Clear();
    TEST_EXPECT(Layout.GetLines().IsEmpty());
    TEST_EXPECT(Layout.GetRuns().IsEmpty());
    TEST_EXPECT_EQ(Layout.GetCharacterCount(), 0);
    TEST_EXPECT(Layout.GetText().IsEmpty());

    TEST_END();
}

bool TextLayoutNewlines_Test()
{
    TEST_BEGIN();

    TSharedPtr<FFixedWidthFontFace> Font = CreateFont();

    TEST_SECTION(TEXT("A newline breaks the line even with no wrap width"));
    FTextLayout Layout;
    Layout.AppendRun(FTextRun(FString(TEXT("A\nB")), Font.Get(), FFloatColor::White));
    Layout.WrapToWidth(0);

    TEST_EXPECT_EQ(Layout.GetLines().Size(), 2);
    TEST_EXPECT(Layout.GetSize() == FIntVector2(GAdvance, 2 * GLineHeight));
    TEST_EXPECT(LineText(Layout, 0) == TEXT("A"));
    TEST_EXPECT(LineText(Layout, 1) == TEXT("B"));

    TEST_SECTION(TEXT("The newline itself is never handed to a draw command"));
    TEST_EXPECT(ConcatenateRuns(Layout) == TEXT("AB"));

    TEST_SECTION(TEXT("It is still counted, so the indices line up with the string the caller holds"));
    TEST_EXPECT_EQ(Layout.GetCharacterCount(), 3);
    TEST_EXPECT(Layout.GetText() == TEXT("A\nB"));

    TEST_SECTION(TEXT("A trailing newline opens a line that takes a full row of its own"));
    FTextLayout TrailingLayout;
    TrailingLayout.AppendRun(FTextRun(FString(TEXT("A\n")), Font.Get(), FFloatColor::White));
    TrailingLayout.WrapToWidth(0);

    TEST_EXPECT_EQ(TrailingLayout.GetLines().Size(), 2);
    TEST_EXPECT_EQ(TrailingLayout.GetLines()[1].RunCount, 0);
    TEST_EXPECT_EQ(TrailingLayout.GetLines()[1].Height, GLineHeight);
    TEST_EXPECT_EQ(TrailingLayout.GetSize().Y, 2 * GLineHeight);

    TEST_SECTION(TEXT("A blank line between two paragraphs survives as a blank line"));
    FTextLayout BlankLayout;
    BlankLayout.AppendRun(FTextRun(FString(TEXT("A\n\nB")), Font.Get(), FFloatColor::White));
    BlankLayout.WrapToWidth(0);

    TEST_EXPECT_EQ(BlankLayout.GetLines().Size(), 3);
    TEST_EXPECT_EQ(BlankLayout.GetLines()[1].RunCount, 0);
    TEST_EXPECT_EQ(BlankLayout.GetLines()[1].Width, 0);
    TEST_EXPECT_EQ(BlankLayout.GetSize().Y, 3 * GLineHeight);

    TEST_SECTION(TEXT("A newline in the middle of a run breaks it without needing two runs"));
    FTextLayout SplitLayout;
    SplitLayout.AppendRun(FTextRun(FString(TEXT("one\ntwo")), Font.Get(), FFloatColor::White));
    SplitLayout.WrapToWidth(0);

    TEST_EXPECT_EQ(SplitLayout.GetLines().Size(), 2);
    TEST_EXPECT_EQ(SplitLayout.GetRuns().Size(), 2);
    TEST_EXPECT(LineText(SplitLayout, 1) == TEXT("two"));

    TEST_SECTION(TEXT("A line break between two runs lands where the runs meet"));
    FTextLayout PairLayout;
    PairLayout.AppendRun(FTextRun(FString(TEXT("one\n")), Font.Get(), FFloatColor::White));
    PairLayout.AppendRun(FTextRun(FString(TEXT("two")), Font.Get(), FFloatColor::Red));
    PairLayout.WrapToWidth(0);

    TEST_EXPECT_EQ(PairLayout.GetLines().Size(), 2);
    TEST_EXPECT(LineText(PairLayout, 0) == TEXT("one"));
    TEST_EXPECT(LineText(PairLayout, 1) == TEXT("two"));
    TEST_EXPECT(PairLayout.GetRuns()[1].Tint == FFloatColor::Red);

    TEST_END();
}

bool TextLayoutWrapping_Test()
{
    TEST_BEGIN();

    TSharedPtr<FFixedWidthFontFace> Font = CreateFont();

    TEST_SECTION(TEXT("Text that fits stays on one line"));
    FTextLayout Layout;
    Layout.AppendRun(FTextRun(FString(TEXT("aaa bbb ccc")), Font.Get(), FFloatColor::White));
    Layout.WrapToWidth(1000);

    TEST_EXPECT_EQ(Layout.GetLines().Size(), 1);
    TEST_EXPECT_EQ(Layout.GetWrapWidth(), 1000);

    TEST_SECTION(TEXT("Text that does not is broken at a word boundary rather than mid-word"));
    Layout.WrapToWidth(10 * GAdvance);

    TEST_EXPECT_EQ(Layout.GetLines().Size(), 2);
    TEST_EXPECT(LineText(Layout, 0) == TEXT("aaa bbb "));
    TEST_EXPECT(LineText(Layout, 1) == TEXT("ccc"));

    TEST_SECTION(TEXT("No line is wider than the width it was wrapped to"));
    for (const FTextLine& Line : Layout.GetLines())
    {
        TEST_EXPECT(Line.Width <= 10 * GAdvance);
    }

    TEST_SECTION(TEXT("Wrapping loses no characters"));
    TEST_EXPECT(ConcatenateRuns(Layout) == TEXT("aaa bbb ccc"));

    TEST_SECTION(TEXT("The size grows down rather than out"));
    TEST_EXPECT(Layout.GetSize().X <= 10 * GAdvance);
    TEST_EXPECT_EQ(Layout.GetSize().Y, 2 * GLineHeight);

    TEST_SECTION(TEXT("Widening it again puts the text back on one line"));
    Layout.WrapToWidth(1000);
    TEST_EXPECT_EQ(Layout.GetLines().Size(), 1);
    TEST_EXPECT(ConcatenateRuns(Layout) == TEXT("aaa bbb ccc"));

    TEST_SECTION(TEXT("A word wider than the whole line is cut rather than looping forever"));
    FTextLayout LongWordLayout;
    LongWordLayout.AppendRun(FTextRun(FString(TEXT("aaaaaaaaaaaa")), Font.Get(), FFloatColor::White));
    LongWordLayout.WrapToWidth(5 * GAdvance);

    TEST_EXPECT_EQ(LongWordLayout.GetLines().Size(), 3);
    TEST_EXPECT(ConcatenateRuns(LongWordLayout) == TEXT("aaaaaaaaaaaa"));

    for (const FTextLine& Line : LongWordLayout.GetLines())
    {
        TEST_EXPECT(Line.Width <= 5 * GAdvance);
    }

    TEST_SECTION(TEXT("A word that does not fit in what is left of a line moves down whole"));
    FTextLayout PushDownLayout;
    PushDownLayout.AppendRun(FTextRun(FString(TEXT("ab ")), Font.Get(), FFloatColor::White));
    PushDownLayout.AppendRun(FTextRun(FString(TEXT("cdefgh")), Font.Get(), FFloatColor::Red));
    PushDownLayout.WrapToWidth(8 * GAdvance);

    TEST_EXPECT_EQ(PushDownLayout.GetLines().Size(), 2);
    TEST_EXPECT(LineText(PushDownLayout, 0) == TEXT("ab "));
    TEST_EXPECT(LineText(PushDownLayout, 1) == TEXT("cdefgh"));

    TEST_SECTION(TEXT("Wrapping crosses a run boundary, so one paragraph of mixed styles reflows as one"));
    FTextLayout MixedLayout;
    MixedLayout.AppendRun(FTextRun(FString(TEXT("aaa ")), Font.Get(), FFloatColor::White));
    MixedLayout.AppendRun(FTextRun(FString(TEXT("bbb ccc")), Font.Get(), FFloatColor::Red));
    MixedLayout.WrapToWidth(8 * GAdvance);

    TEST_EXPECT(MixedLayout.GetLines().Size() >= 2);
    TEST_EXPECT(ConcatenateRuns(MixedLayout) == TEXT("aaa bbb ccc"));

    TEST_SECTION(TEXT("Wrapping and newlines compose, so a wrapped paragraph still ends where it says it does"));
    FTextLayout ParagraphLayout;
    ParagraphLayout.AppendRun(FTextRun(FString(TEXT("aaa bbb\nccc")), Font.Get(), FFloatColor::White));
    ParagraphLayout.WrapToWidth(4 * GAdvance);

    TEST_EXPECT_EQ(ParagraphLayout.GetLines().Size(), 3);
    TEST_EXPECT(LineText(ParagraphLayout, 2) == TEXT("ccc"));

    TEST_END();
}

bool TextLayoutHitTesting_Test()
{
    TEST_BEGIN();

    TSharedPtr<FFixedWidthFontFace> Font = CreateFont();

    FTextLayout Layout;
    Layout.AppendRun(FTextRun(FString(TEXT("Hello")), Font.Get(), FFloatColor::White));
    Layout.AppendRun(FTextRun(FString(TEXT(" World")), Font.Get(), FFloatColor::Red));
    Layout.WrapToWidth(0);

    TEST_SECTION(TEXT("A click at the start lands on the first character"));
    TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(FIntVector2(0, 0)), 0);

    TEST_SECTION(TEXT("A click inside a run lands on the character under it"));
    TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(FIntVector2(3 * GAdvance, 0)), 3);

    TEST_SECTION(TEXT("A click on the second run resolves through the run it landed in"));
    TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(FIntVector2(5 * GAdvance, 0)), 5);
    TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(FIntVector2(8 * GAdvance, 0)), 8);

    TEST_SECTION(TEXT("A click past the end of the text lands after the last character"));
    TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(FIntVector2(1000, 0)), Layout.GetCharacterCount());

    TEST_SECTION(TEXT("A click below the last line stays on it rather than running off the end"));
    TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(FIntVector2(0, 1000)), 0);

    TEST_SECTION(TEXT("Every character can be found again from the left edge of its own box"));
    for (int32 Index = 0; Index < Layout.GetCharacterCount(); ++Index)
    {
        const FRectangle Bounds = Layout.GetCharacterBounds(Index);
        TEST_EXPECT_EQ(Layout.FindCharacterIndexAt(Bounds.Position), Index);
    }

    TEST_SECTION(TEXT("A character box is one advance wide and one line tall"));
    const FRectangle FirstBounds = Layout.GetCharacterBounds(0);
    TEST_EXPECT_EQ(FirstBounds.Width, GAdvance);
    TEST_EXPECT_EQ(FirstBounds.Height, GLineHeight);
    TEST_EXPECT(FirstBounds.Position == FIntVector2(0, 0));

    TEST_SECTION(TEXT("The boxes march across the line in order"));
    TEST_EXPECT(Layout.GetCharacterBounds(1).Position == FIntVector2(GAdvance, 0));
    TEST_EXPECT(Layout.GetCharacterBounds(5).Position == FIntVector2(5 * GAdvance, 0));

    TEST_SECTION(TEXT("The position one past the end is a caret rather than a character, so it has no width"));
    const FRectangle EndBounds = Layout.GetCharacterBounds(Layout.GetCharacterCount());
    TEST_EXPECT_EQ(EndBounds.Width, 0);
    TEST_EXPECT_EQ(EndBounds.Position.X, 11 * GAdvance);

    TEST_SECTION(TEXT("Every character is on the one line there is"));
    TEST_EXPECT_EQ(Layout.FindLineIndexForCharacter(0), 0);
    TEST_EXPECT_EQ(Layout.FindLineIndexForCharacter(11), 0);

    FTextLayout MultiLineLayout;
    MultiLineLayout.AppendRun(FTextRun(FString(TEXT("Hello\nWorld")), Font.Get(), FFloatColor::White));
    MultiLineLayout.WrapToWidth(0);

    TEST_SECTION(TEXT("A click on the second line resolves against that line's own runs"));
    TEST_EXPECT_EQ(MultiLineLayout.FindCharacterIndexAt(FIntVector2(0, GLineHeight)), 6);
    TEST_EXPECT_EQ(MultiLineLayout.FindCharacterIndexAt(FIntVector2(2 * GAdvance, GLineHeight)), 8);

    TEST_SECTION(TEXT("A character on the second line is drawn one line down"));
    TEST_EXPECT(MultiLineLayout.GetCharacterBounds(6).Position == FIntVector2(0, GLineHeight));

    TEST_SECTION(TEXT("The line index follows the break"));
    TEST_EXPECT_EQ(MultiLineLayout.FindLineIndexForCharacter(0), 0);
    TEST_EXPECT_EQ(MultiLineLayout.FindLineIndexForCharacter(5), 0);
    TEST_EXPECT_EQ(MultiLineLayout.FindLineIndexForCharacter(6), 1);
    TEST_EXPECT_EQ(MultiLineLayout.FindLineIndexForCharacter(11), 1);

    TEST_SECTION(TEXT("Hit testing an empty layout is answerable rather than out of range"));
    FTextLayout EmptyLayout;
    TEST_EXPECT_EQ(EmptyLayout.FindCharacterIndexAt(FIntVector2(50, 50)), 0);
    TEST_EXPECT_EQ(EmptyLayout.FindLineIndexForCharacter(0), 0);

    TEST_END();
}

bool TextLayoutDraw_Test()
{
    TEST_BEGIN();

    TSharedPtr<FFixedWidthFontFace> Font = CreateFont();

    TEST_SECTION(TEXT("An empty layout draws nothing"));
    FTextLayout EmptyLayout;
    EmptyLayout.WrapToWidth(0);

    FDrawCommandList EmptyList;
    EmptyLayout.Draw(FDrawGeometry(FRectangle(FIntVector2(0, 0), 100, 20), 1.0f), EmptyList, 0);
    TEST_EXPECT_EQ(EmptyList.CountCommandsOfType(EDrawCommandType::Text), 0);

    FTextLayout Layout;
    Layout.AppendRun(FTextRun(FString(TEXT("Hello")), Font.Get(), FFloatColor::White));

    FTextRun Highlighted(FString(TEXT(" World")), Font.Get(), FFloatColor::Red);
    Highlighted.BackgroundTint = FFloatColor(0.0f, 0.0f, 1.0f, 1.0f);
    Layout.AppendRun(Highlighted);

    Layout.WrapToWidth(0);

    TEST_SECTION(TEXT("Each run becomes one text command"));
    FDrawCommandList CommandList;
    const int32 HighestLayer = Layout.Draw(FDrawGeometry(FRectangle(FIntVector2(10, 20), 200, 40), 1.0f), CommandList, 3);

    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Text), 2);

    TEST_SECTION(TEXT("Only the run with a background gets a box behind it"));
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 1);

    TEST_SECTION(TEXT("The background sits under the text rather than over it"));
    const int32 TextIndex = CommandList.FindTextCommand(FStringView(TEXT("Hello")));
    TEST_EXPECT(TextIndex != FDrawCommandList::InvalidIndex);

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box)
        {
            TEST_EXPECT_EQ(Command.LayerId, 3);
        }
        else if (Command.Type == EDrawCommandType::Text)
        {
            TEST_EXPECT_EQ(Command.LayerId, 4);
        }
    }

    TEST_EXPECT_EQ(HighestLayer, 4);

    TEST_SECTION(TEXT("The runs are placed left to right from the top-left of the geometry"));
    const int32 SecondIndex = CommandList.FindTextCommand(FStringView(TEXT(" World")));
    TEST_EXPECT(SecondIndex != FDrawCommandList::InvalidIndex);

    if (TextIndex != FDrawCommandList::InvalidIndex && SecondIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(CommandList[TextIndex].Bounds.Position == FIntVector2(10, 20));
        TEST_EXPECT(CommandList[SecondIndex].Bounds.Position == FIntVector2(10 + (5 * GAdvance), 20));
        TEST_EXPECT_EQ(CommandList[SecondIndex].Bounds.Width, 6 * GAdvance);
    }

    TEST_SECTION(TEXT("Each run keeps its own face and color on the way to the command"));
    if (SecondIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(CommandList[SecondIndex].Font == Font.Get());
        TEST_EXPECT(CommandList[SecondIndex].HasTint(FFloatColor::Red));
    }

    TEST_SECTION(TEXT("A second line is drawn one line height below the first"));
    FTextLayout MultiLineLayout;
    MultiLineLayout.AppendRun(FTextRun(FString(TEXT("one\ntwo")), Font.Get(), FFloatColor::White));
    MultiLineLayout.WrapToWidth(0);

    FDrawCommandList MultiLineList;
    MultiLineLayout.Draw(FDrawGeometry(FRectangle(FIntVector2(0, 0), 200, 40), 1.0f), MultiLineList, 0);

    const int32 FirstLineIndex  = MultiLineList.FindTextCommand(FStringView(TEXT("one")));
    const int32 SecondLineIndex = MultiLineList.FindTextCommand(FStringView(TEXT("two")));

    TEST_EXPECT(FirstLineIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(SecondLineIndex != FDrawCommandList::InvalidIndex);

    if (FirstLineIndex != FDrawCommandList::InvalidIndex && SecondLineIndex != FDrawCommandList::InvalidIndex)
    {
        TEST_EXPECT(MultiLineList[FirstLineIndex].Bounds.Position == FIntVector2(0, 0));
        TEST_EXPECT(MultiLineList[SecondLineIndex].Bounds.Position == FIntVector2(0, GLineHeight));
    }

    TEST_END();
}
