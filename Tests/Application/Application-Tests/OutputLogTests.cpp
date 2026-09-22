#include "OutputLogTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Misc/OutputDeviceLogger.h>
#include <Application/Console/ConsoleLogBuffer.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/LogView.h>
#include <Application/Elements/RichTextBlock.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Input/Keys.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Tick(Bounds);
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition, bool bIsDown)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(), bIsDown);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static void DragSelection(const TSharedPtr<FRichTextBlock>& Block, const IntVector2& From, const IntVector2& To)
{
    Block->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, From, true));
    Block->OnMouseMove(MakeMoveEvent(To));
    Block->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, To, false));
}

static TSharedPtr<FRichTextBlock> MakeBlock(const TSharedPtr<IFontFace>& Font, const TArray<String>& Texts, bool bAutoWrapText = false)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    FRichTextBlock::FDesc Desc;
    Desc.bAutoWrapText = bAutoWrapText;

    for (int32 Index = 0; Index < Texts.Size(); ++Index)
    {
        Desc.Runs.Add(FTextRun(Texts[Index], Font.Get(), Index % 2 == 0 ? Style.Colors.Text : Style.Colors.Accent));
    }

    return FRichTextBlock::Create(Desc);
}

static int32 CountBoxes(const FDrawCommandList& CommandList)
{
    int32 NumBoxes = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        NumBoxes += Command.Type == EDrawCommandType::Box ? 1 : 0;
    }

    return NumBoxes;
}

static int32 CountTexts(const FDrawCommandList& CommandList)
{
    int32 NumTexts = 0;
    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        NumTexts += Command.Type == EDrawCommandType::Text ? 1 : 0;
    }

    return NumTexts;
}

static void DrawElement(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList)
{
    const FDrawGeometry Geometry(Element->GetContentRectangle(), 1.0f);
    Element->OnDraw(Geometry, OutCommandList, 0);
}

bool RichTextLayout_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    TSharedPtr<FRichTextBlock> Block = MakeBlock(Font, { "[Info] ", "engine started" });
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_SECTION("The runs concatenate into one string the indices refer to");
    TEST_EXPECT_EQ(Block->GetText(), String("[Info] engine started"));
    TEST_EXPECT_EQ(Block->GetLayout().GetCharacterCount(), 21);

    TEST_SECTION("Without wrapping it is one line as wide as the text");
    TEST_EXPECT_EQ(Block->GetLayout().GetLines().Size(), 1);
    TEST_EXPECT_EQ(Block->GetLayout().GetSize().X, 21 * 8);

    TEST_SECTION("A newline in a run starts a line without any wrapping");
    TSharedPtr<FRichTextBlock> TwoLines = MakeBlock(Font, { "first\n", "second\n" });
    LayoutElement(TwoLines, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(TwoLines->GetLayout().GetLines().Size(), 3);

    TEST_SECTION("A wrapping block breaks at the width it was arranged to");
    TSharedPtr<FRichTextBlock> Wrapped = MakeBlock(Font, { "aaaa bbbb cccc dddd" }, true);
    LayoutElement(Wrapped, FRectangle(IntVector2(0, 0), 80, 200));

    TEST_EXPECT(Wrapped->GetLayout().GetLines().Size() > 1);
    TEST_EXPECT(Wrapped->GetLayout().GetSize().X <= 80);

    TEST_SECTION("Re-arranging to a wider box re-wraps rather than keeping the old breaks");
    LayoutElement(Wrapped, FRectangle(IntVector2(0, 0), 400, 200));
    TEST_EXPECT_EQ(Wrapped->GetLayout().GetLines().Size(), 1);

    TEST_SECTION("A margin insets the text without changing what it measures");
    FRichTextBlock::FDesc InsetDesc;
    InsetDesc.Runs.Add(FTextRun("hello", Font.Get(), FUIStyle::GetDefault().Colors.Text));
    InsetDesc.Margin = FMargin(10, 6);

    TSharedPtr<FRichTextBlock> Inset = FRichTextBlock::Create(InsetDesc);
    LayoutElement(Inset, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Inset->ComputeDesiredSize().X, 5 * 8 + 20);
    TEST_EXPECT_EQ(Inset->ComputeDesiredSize().Y, 16 + 12);

    TEST_SECTION("Replacing the runs replaces the text");
    Block->SetRuns({ FTextRun("replaced", Font.Get(), FUIStyle::GetDefault().Colors.Text) });
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Block->GetText(), String("replaced"));

    TEST_SECTION("Clearing leaves nothing behind");
    Block->ClearRuns();
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Block->GetLayout().GetCharacterCount(), 0);
    TEST_EXPECT(!Block->HasSelection());

    TEST_END();
}

bool RichTextCulling_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    String LongText;
    for (int32 Index = 0; Index < 100; ++Index)
    {
        LongText += String::Printf("line %d\n", Index);
    }

    TSharedPtr<FRichTextBlock> Block = MakeBlock(Font, { LongText });
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 100 * 16));

    TEST_SECTION("With nothing clipping it every line is drawn");
    FDrawCommandList Unclipped;
    DrawElement(Block, Unclipped);

    TEST_EXPECT_EQ(CountTexts(Unclipped), 100);

    TEST_SECTION("Behind a clip only the lines it leaves open are, so scrolling costs the view not the document");
    FDrawCommandList Clipped;
    Clipped.PushClip(0, FRectangle(IntVector2(0, 0), 400, 3 * 16));
    DrawElement(Block, Clipped);
    Clipped.PopClip(1);

    TEST_EXPECT_EQ(CountTexts(Clipped), 3);

    TEST_SECTION("A clip part way down draws the band it covers rather than the lines above it");
    FDrawCommandList Scrolled;
    Scrolled.PushClip(0, FRectangle(IntVector2(0, 50 * 16), 400, 4 * 16));
    DrawElement(Block, Scrolled);
    Scrolled.PopClip(1);

    TEST_EXPECT_EQ(CountTexts(Scrolled), 4);

    TEST_END();
}

bool RichTextSelection_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font = CreateFont();

    String LastSelection;
    int32  NumSelectionCallbacks = 0;

    FRichTextBlock::FDesc Desc;
    Desc.Runs.Add(FTextRun("[Error] ", Font.Get(), FUIStyle::GetDefault().Colors.Accent));
    Desc.Runs.Add(FTextRun("disk full", Font.Get(), FUIStyle::GetDefault().Colors.Text));
    Desc.OnSelectionChanged = FOnSelectionChanged::CreateLambda([&](const String& SelectedText)
    {
        LastSelection = SelectedText;
        NumSelectionCallbacks++;
    });

    TSharedPtr<FRichTextBlock> Block = FRichTextBlock::Create(Desc);
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_SECTION("Nothing is selected to begin with");
    TEST_EXPECT(!Block->HasSelection());
    TEST_EXPECT(Block->GetSelectedText().IsEmpty());

    TEST_SECTION("A drag selects the characters it crossed, whatever run they came from");
    DragSelection(Block, IntVector2(2 * 8 + 1, 8), IntVector2(11 * 8 + 1, 8));

    TEST_EXPECT(Block->HasSelection());
    TEST_EXPECT_EQ(Block->GetSelectionStart(), 2);
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 11);
    TEST_EXPECT_EQ(Block->GetSelectedText(), String("rror] dis"));

    TEST_SECTION("Which is what the selection callback was told");
    TEST_EXPECT_EQ(LastSelection, String("rror] dis"));

    TEST_SECTION("Dragging back the other way selects the same span");
    DragSelection(Block, IntVector2(11 * 8 + 1, 8), IntVector2(2 * 8 + 1, 8));

    TEST_EXPECT_EQ(Block->GetSelectionStart(), 2);
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 11);

    TEST_SECTION("A press with no drag places a caret rather than selecting anything");
    Block->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(4 * 8 + 1, 8), true));
    Block->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, IntVector2(4 * 8 + 1, 8), false));

    TEST_EXPECT(!Block->HasSelection());

    TEST_SECTION("Select all takes the whole text");
    Block->SelectAll();

    TEST_EXPECT_EQ(Block->GetSelectedText(), String("[Error] disk full"));
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 17);

    TEST_SECTION("A selection is drawn behind the text, one box per line it covers");
    FDrawCommandList Unselected;
    Block->ClearSelection();
    DrawElement(Block, Unselected);

    FDrawCommandList Selected;
    Block->SelectAll();
    DrawElement(Block, Selected);

    TEST_EXPECT_EQ(CountBoxes(Selected) - CountBoxes(Unselected), 1);

    TEST_SECTION("A selection spanning wrapped lines is one box per line");
    TSharedPtr<FRichTextBlock> Wrapped = MakeBlock(Font, { "aaaa bbbb cccc" }, true);
    LayoutElement(Wrapped, FRectangle(IntVector2(0, 0), 40, 200));

    TEST_EXPECT_EQ(Wrapped->GetLayout().GetLines().Size(), 3);

    FDrawCommandList WrappedEmpty;
    DrawElement(Wrapped, WrappedEmpty);

    FDrawCommandList WrappedSelected;
    Wrapped->SelectAll();
    DrawElement(Wrapped, WrappedSelected);

    TEST_EXPECT_EQ(CountBoxes(WrappedSelected) - CountBoxes(WrappedEmpty), 3);

    TEST_SECTION("Setting a span out of range clamps rather than reading past the text");
    Block->SetSelection(-10, 500);

    TEST_EXPECT_EQ(Block->GetSelectionStart(), 0);
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 17);

    TEST_SECTION("A block that is not selectable ignores a drag");
    FRichTextBlock::FDesc FixedDesc;
    FixedDesc.Runs.Add(FTextRun("read only", Font.Get(), FUIStyle::GetDefault().Colors.Text));
    FixedDesc.bIsSelectable = false;

    TSharedPtr<FRichTextBlock> Fixed = FRichTextBlock::Create(FixedDesc);
    LayoutElement(Fixed, FRectangle(IntVector2(0, 0), 400, 60));

    DragSelection(Fixed, IntVector2(1, 8), IntVector2(60, 8));
    TEST_EXPECT(!Fixed->HasSelection());

    TEST_SECTION("Text that can be dragged through puts the text cursor under the pointer, so the log reads as selectable");
    TSharedPtr<FRichTextBlock> Hovered = MakeBlock(Font, { "hover me" });
    LayoutElement(Hovered, FRectangle(IntVector2(0, 0), 400, 60));

    ECursor Cursor = ECursor::Arrow;
    TEST_EXPECT(!Hovered->GetCursor(Cursor));

    Hovered->OnMouseEntered(MakeMoveEvent(IntVector2(4 * 8 + 1, 8)));

    TEST_EXPECT(Hovered->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::TextInput);

    TEST_SECTION("Leaving it hands the shape back");
    Hovered->OnMouseLeft(MakeMoveEvent(IntVector2(900, 900)));

    TEST_EXPECT(!Hovered->GetCursor(Cursor));

    TEST_SECTION("Text that cannot be selected keeps the arrow, which is what a tool tip wants");
    Fixed->OnMouseEntered(MakeMoveEvent(IntVector2(1, 8)));

    TEST_EXPECT(!Fixed->GetCursor(Cursor));

    TEST_SECTION("Dragging past the top of a host scroll box moves the view and grows the selection");
    FRichTextBlock::FDesc TallDesc;
    for (int32 Line = 0; Line < 20; ++Line)
    {
        TallDesc.Runs.Add(FTextRun("line of log text\n", Font.Get(), FUIStyle::GetDefault().Colors.Text));
    }

    TSharedPtr<FRichTextBlock> TallBlock = FRichTextBlock::Create(TallDesc);
    TSharedPtr<FScrollBox>     Host      = FScrollBox::Create();
    Host->SetContent(TallBlock);
    LayoutElement(Host, FRectangle(IntVector2(0, 0), 200, 48));
    Host->SetScrollOffset(FScrollBox::DefaultScrollAmountPerWheelStep * 2);

    const int32 OffsetBefore = Host->GetScrollOffset();
    TEST_EXPECT(OffsetBefore > 0);

    TallBlock->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(8, 24), true));
    TallBlock->OnMouseMove(MakeMoveEvent(IntVector2(8, -20)));

    TEST_EXPECT(Host->GetScrollOffset() < OffsetBefore);
    TEST_EXPECT(TallBlock->HasSelection());

    TEST_END();
}

bool RichTextSearch_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    TSharedPtr<FRichTextBlock> Block = MakeBlock(Font, { "the cat sat on ", "the mat" });
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_SECTION("No search means no matches");
    TEST_EXPECT(Block->GetSearchMatches().IsEmpty());

    TEST_SECTION("Every occurrence is found, including the one in the second run");
    Block->SetSearchText("the");

    TEST_EXPECT_EQ(Block->GetSearchMatches().Size(), 2);
    TEST_EXPECT_EQ(Block->GetSearchMatches()[0], 0);
    TEST_EXPECT_EQ(Block->GetSearchMatches()[1], 15);

    TEST_SECTION("A match crossing the boundary between two runs is still one match");
    Block->SetSearchText("on the");

    TEST_EXPECT_EQ(Block->GetSearchMatches().Size(), 1);
    TEST_EXPECT_EQ(Block->GetSearchMatches()[0], 12);

    TEST_SECTION("Matches do not overlap");
    TSharedPtr<FRichTextBlock> Repeated = MakeBlock(Font, { "aaaa" });
    LayoutElement(Repeated, FRectangle(IntVector2(0, 0), 400, 60));

    Repeated->SetSearchText("aa");
    TEST_EXPECT_EQ(Repeated->GetSearchMatches().Size(), 2);

    TEST_SECTION("A search with no hits finds none, and the text is untouched");
    Block->SetSearchText("dog");

    TEST_EXPECT(Block->GetSearchMatches().IsEmpty());
    TEST_EXPECT_EQ(Block->GetText(), String("the cat sat on the mat"));

    TEST_SECTION("Clearing the search clears the matches");
    Block->SetSearchText("the");
    Block->SetSearchText("");

    TEST_EXPECT(Block->GetSearchMatches().IsEmpty());

    TEST_SECTION("Each hit is highlighted with its own box");
    FDrawCommandList Unhighlighted;
    DrawElement(Block, Unhighlighted);

    FDrawCommandList Highlighted;
    Block->SetSearchText("the");
    DrawElement(Block, Highlighted);

    TEST_EXPECT_EQ(CountBoxes(Highlighted) - CountBoxes(Unhighlighted), 2);

    TEST_SECTION("A match is filled in the search color rather than the one a selection takes");
    Block->SelectAll();

    FDrawCommandList Selected;
    DrawElement(Block, Selected);

    const FUIStyle& Style = FUIStyle::GetDefault();

    int32 SearchLayer    = -1;
    int32 SelectionLayer = -1;
    for (const FDrawCommand& Command : Selected.GetCommands())
    {
        if (Command.Type != EDrawCommandType::Box)
        {
            continue;
        }

        if (Command.HasTint(Style.Colors.SearchTextHighlight))
        {
            SearchLayer = Command.LayerId;
        }
        else if (Command.HasTint(Style.Colors.TextSelectionBackground))
        {
            SelectionLayer = Command.LayerId;
        }
    }

    TEST_EXPECT(SearchLayer >= 0);
    TEST_EXPECT(SelectionLayer >= 0);

    TEST_EXPECT(SearchLayer > SelectionLayer);

    Block->ClearSelection();

    TEST_SECTION("Replacing the text re-runs the search against it");
    Block->SetRuns({ FTextRun("the the the", Font.Get(), FUIStyle::GetDefault().Colors.Text) });
    LayoutElement(Block, FRectangle(IntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Block->GetSearchMatches().Size(), 3);

    TEST_END();
}

bool LogViewLogging_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FLogView::FDesc Desc;
    Desc.Font         = Font;
    Desc.MaxLineCount = 4;

    TSharedPtr<FLogView> LogView = FLogView::Create(Desc);
    LayoutElement(LogView, FRectangle(IntVector2(0, 0), 400, 200));

    TEST_SECTION("A fresh view holds nothing");
    TEST_EXPECT_EQ(LogView->GetNumLines(), 0);

    TEST_SECTION("A logged line is held once the view has ticked, since logging can come from any thread");
    LogView->Log(ELogSeverity::Info, "engine started");

    TEST_EXPECT_EQ(LogView->GetNumLines(), 0);
    LayoutElement(LogView, FRectangle(IntVector2(0, 0), 400, 200));

    TEST_EXPECT_EQ(LogView->GetNumLines(), 1);
    TEST_EXPECT_EQ(LogView->GetLines()[0].Message, String("engine started"));
    TEST_EXPECT_EQ(LogView->GetLines()[0].Severity, ELogSeverity::Info);

    TEST_SECTION("Flush folds them in without waiting for a tick");
    LogView->Log(ELogSeverity::Warning, "shader cache cold");
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 2);
    TEST_EXPECT_EQ(LogView->GetLines()[1].Severity, ELogSeverity::Warning);

    TEST_SECTION("The message-only overload logs at info");
    LogView->Log("plain");
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetLines()[2].Severity, ELogSeverity::Info);

    TEST_SECTION("The text carries the severity prefix and its colour, and the break is a run of its own");
    const TArray<FTextRun>& Runs = LogView->GetTextBlock()->GetLayout().GetSourceRuns();

    TEST_EXPECT_EQ(Runs.Size(), 8);
    TEST_EXPECT_EQ(Runs[0].Text, String("[Info] "));
    TEST_EXPECT_EQ(Runs[1].Text, String("engine started"));
    TEST_EXPECT_EQ(Runs[2].Text, String("\n"));
    TEST_EXPECT_EQ(Runs[3].Text, String("[Warning] "));

    TEST_EXPECT_EQ(Runs[3].Tint, FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning));
    TEST_EXPECT_EQ(Runs[4].Tint, FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning));

    TEST_SECTION("Which still reads back as one line per message");
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetText(), String("[Info] engine started\n[Warning] shader cache cold\n[Info] plain"));

    TEST_SECTION("Past the cap the oldest lines are dropped");
    LogView->Log("fourth");
    LogView->Log("fifth");
    LogView->Log("sixth");
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 4);
    TEST_EXPECT_EQ(LogView->GetLines()[0].Message, String("plain"));
    TEST_EXPECT_EQ(LogView->GetLines()[3].Message, String("sixth"));

    TEST_SECTION("Shrinking the cap trims what is already held");
    LogView->SetMaxLineCount(2);

    TEST_EXPECT_EQ(LogView->GetNumLines(), 2);
    TEST_EXPECT_EQ(LogView->GetLines()[0].Message, String("fifth"));

    TEST_SECTION("Clearing empties both the lines and the text");
    LogView->Clear();
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 0);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetText(), String());

    TEST_SECTION("Registering with the logger routes what the engine logs into the view");
    LogView->SetMaxLineCount(16);
    LogView->RegisterWithLogger();

    LOG_INFO("routed through the logger");
    LogView->Flush();

    LogView->UnregisterFromLogger();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 1);
    TEST_EXPECT(LogView->GetLines()[0].Message.Contains("routed through the logger"));

    TEST_SECTION("And unregistering stops it");
    LOG_INFO("after unregistering");
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 1);

    TEST_END();
}

bool LogViewFiltering_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FLogView::FDesc Desc;
    Desc.Font = Font;

    TSharedPtr<FLogView> LogView = FLogView::Create(Desc);
    LayoutElement(LogView, FRectangle(IntVector2(0, 0), 400, 200));

    LogView->Log(ELogSeverity::Info, "loading level");
    LogView->Log(ELogSeverity::Warning, "texture missing");
    LogView->Log(ELogSeverity::Error, "device lost");
    LogView->Log(ELogSeverity::Info, "loading complete");
    LogView->Flush();

    TEST_SECTION("Everything shows at the lowest severity");
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 4);

    TEST_SECTION("Raising the floor hides what is below it without dropping it");
    LogView->SetMinimumSeverity(ELogSeverity::Warning);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetNumLines(), 4);
    TEST_EXPECT_EQ(LogView->GetVisibleMessages()[0], String("texture missing"));

    TEST_SECTION("Which is what the text is rebuilt from, once the frame that asked for it comes round");
    LogView->Flush();

    TEST_EXPECT(!LogView->GetTextBlock()->GetText().Contains("loading level"));
    TEST_EXPECT(LogView->GetTextBlock()->GetText().Contains("device lost"));

    TEST_SECTION("Errors only leaves one");
    LogView->SetMinimumSeverity(ELogSeverity::Error);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);

    TEST_SECTION("Lowering it again brings the rest back");
    LogView->SetMinimumSeverity(ELogSeverity::Info);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 4);

    TEST_SECTION("A search that only highlights leaves every line showing");
    LogView->SetSearchText("loading", false);
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 4);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 2);

    TEST_SECTION("A search that filters hides the lines with no match");
    LogView->SetSearchText("loading", true);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetVisibleMessages()[1], String("loading complete"));

    TEST_SECTION("The two filters compose rather than replacing one another");
    LogView->SetMinimumSeverity(ELogSeverity::Warning);
    LogView->SetSearchText("e", true);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);

    LogView->SetSearchText("device", true);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);

    TEST_SECTION("Clearing the search brings back what only it was hiding");
    LogView->SetSearchText("", true);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);

    TEST_SECTION("A line arriving while a filter is up obeys it");
    LogView->SetMinimumSeverity(ELogSeverity::Error);
    LogView->Log(ELogSeverity::Info, "ignored for now");
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 5);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);

    TEST_SECTION("Typing a word is one rebuild at the end of the frame rather than one for every letter");
    LogView->SetMinimumSeverity(ELogSeverity::Info);
    LogView->SetSearchText("", false);
    LogView->Flush();

    const String SettledText = LogView->GetTextBlock()->GetText();
    LogView->SetSearchText("l", true);
    LogView->SetSearchText("lo", true);
    LogView->SetSearchText("loa", true);

    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetText(), SettledText);

    TEST_SECTION("And what it settles on is what the last keystroke asked for");
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 2);
    TEST_EXPECT(!LogView->GetTextBlock()->GetText().Contains("device lost"));

    TEST_END();
}

bool LogViewAutoScroll_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;

    const TSharedPtr<IFontFace> Font = CreateFont();

    FLogView::FDesc Desc;
    Desc.Font        = Font;
    Desc.bAutoScroll = true;

    TSharedPtr<FLogView> LogView = FLogView::Create(Desc);

    const FRectangle Bounds(IntVector2(0, 0), 400, 40);
    LayoutElement(LogView, Bounds);

    TEST_SECTION("An empty view is at the bottom, because there is nowhere else to be");
    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_SECTION("Following the tail keeps the last line showing as more arrive");
    for (int32 Index = 0; Index < 20; ++Index)
    {
        LogView->Log(String::Printf("line %d", Index));
    }

    LayoutElement(LogView, Bounds);

    TEST_EXPECT(LogView->GetScrollBox()->GetMaxScrollOffset() > 0);
    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_SECTION("Scrolling away from the tail stops the following");
    LogView->GetScrollBox()->SetScrollOffset(0);
    TEST_EXPECT(!LogView->IsScrolledToBottom());

    LogView->Log("arrived while scrolled up");
    LayoutElement(LogView, Bounds);

    TEST_EXPECT_EQ(LogView->GetScrollBox()->GetScrollOffset(), 0);
    TEST_EXPECT(!LogView->IsScrolledToBottom());

    TEST_SECTION("Scrolling back to the tail re-arms it");
    LogView->ScrollToBottom();
    TEST_EXPECT(LogView->IsScrolledToBottom());

    LogView->Log("arrived after coming back");
    LayoutElement(LogView, Bounds);

    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_SECTION("Turning it off leaves the view where the reader put it");
    LogView->SetAutoScroll(false);
    LogView->GetScrollBox()->SetScrollOffset(0);

    LogView->Log("ignored by the scroll");
    LayoutElement(LogView, Bounds);

    TEST_EXPECT_EQ(LogView->GetScrollBox()->GetScrollOffset(), 0);

    TEST_SECTION("Turning it back on jumps to the tail straight away");
    LogView->SetAutoScroll(true);
    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_END();
}
