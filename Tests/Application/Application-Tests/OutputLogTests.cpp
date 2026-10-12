#include "OutputLogTests.h"
#include "StubPlatformApplication.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Misc/OutputDeviceManager.h>
#include <Application/Console/ConsoleLogBuffer.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/LogView.h>
#include <Application/Elements/RichTextBlock.h>
#include <Core/Containers/StringView.h>
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
    Element->Arrange(Bounds);
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const FIntVector2& ClientPosition, bool bIsDown)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, FIntVector2(0, 0), FModifierKeyState(), bIsDown);
}

static FCursorEvent MakeMoveEvent(const FIntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, FIntVector2(0, 0), FModifierKeyState());
}

static void DragSelection(const TSharedPtr<FRichTextBlock>& Block, const FIntVector2& From, const FIntVector2& To)
{
    Block->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, From, true));
    Block->OnMouseMove(MakeMoveEvent(To));
    Block->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, To, false));
}

static TSharedPtr<FRichTextBlock> MakeBlock(const TSharedPtr<IFontFace>& Font, const TArray<FString>& Texts, bool bAutoWrapText = false)
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

    TSharedPtr<FRichTextBlock> Block = MakeBlock(Font, { TEXT("[Info] "), TEXT("engine started") });
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_SECTION(TEXT("The runs concatenate into one string the indices refer to"));
    TEST_EXPECT_EQ(Block->GetText(), FString(TEXT("[Info] engine started")));
    TEST_EXPECT_EQ(Block->GetLayout().GetCharacterCount(), 21);

    TEST_SECTION(TEXT("Without wrapping it is one line as wide as the text"));
    TEST_EXPECT_EQ(Block->GetLayout().GetLines().Size(), 1);
    TEST_EXPECT_EQ(Block->GetLayout().GetSize().X, 21 * 8);

    TEST_SECTION(TEXT("A newline in a run starts a line without any wrapping"));
    TSharedPtr<FRichTextBlock> TwoLines = MakeBlock(Font, { TEXT("first\n"), TEXT("second\n") });
    LayoutElement(TwoLines, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(TwoLines->GetLayout().GetLines().Size(), 3);

    TEST_SECTION(TEXT("A wrapping block breaks at the width it was arranged to"));
    TSharedPtr<FRichTextBlock> Wrapped = MakeBlock(Font, { TEXT("aaaa bbbb cccc dddd") }, true);
    LayoutElement(Wrapped, FRectangle(FIntVector2(0, 0), 80, 200));

    TEST_EXPECT(Wrapped->GetLayout().GetLines().Size() > 1);
    TEST_EXPECT(Wrapped->GetLayout().GetSize().X <= 80);

    TEST_SECTION(TEXT("Re-arranging to a wider box re-wraps rather than keeping the old breaks"));
    LayoutElement(Wrapped, FRectangle(FIntVector2(0, 0), 400, 200));
    TEST_EXPECT_EQ(Wrapped->GetLayout().GetLines().Size(), 1);

    TEST_SECTION(TEXT("A margin insets the text without changing what it measures"));
    FRichTextBlock::FDesc InsetDesc;
    InsetDesc.Runs.Add(FTextRun(TEXT("hello"), Font.Get(), FUIStyle::GetDefault().Colors.Text));
    InsetDesc.Margin = FMargin(10, 6);

    TSharedPtr<FRichTextBlock> Inset = FRichTextBlock::Create(InsetDesc);
    LayoutElement(Inset, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Inset->ComputeDesiredSize().X, 5 * 8 + 20);
    TEST_EXPECT_EQ(Inset->ComputeDesiredSize().Y, 16 + 12);

    TEST_SECTION(TEXT("Replacing the runs replaces the text"));
    Block->SetRuns({ FTextRun(TEXT("replaced"), Font.Get(), FUIStyle::GetDefault().Colors.Text) });
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Block->GetText(), FString(TEXT("replaced")));

    TEST_SECTION(TEXT("Clearing leaves nothing behind"));
    Block->ClearRuns();
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Block->GetLayout().GetCharacterCount(), 0);
    TEST_EXPECT(!Block->HasSelection());

    TEST_END();
}

bool RichTextCulling_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    FString LongText;
    for (int32 Index = 0; Index < 100; ++Index)
    {
        LongText += FString::Printf(TEXT("line %d\n"), Index);
    }

    TSharedPtr<FRichTextBlock> Block = MakeBlock(Font, { LongText });
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 100 * 16));

    TEST_SECTION(TEXT("With nothing clipping it every line is drawn"));
    FDrawCommandList Unclipped;
    DrawElement(Block, Unclipped);

    TEST_EXPECT_EQ(CountTexts(Unclipped), 100);

    TEST_SECTION(TEXT("Behind a clip only the lines it leaves open are, so scrolling costs the view not the document"));
    FDrawCommandList Clipped;
    Clipped.PushClip(0, FRectangle(FIntVector2(0, 0), 400, 3 * 16));
    DrawElement(Block, Clipped);
    Clipped.PopClip(1);

    TEST_EXPECT_EQ(CountTexts(Clipped), 3);

    TEST_SECTION(TEXT("A clip part way down draws the band it covers rather than the lines above it"));
    FDrawCommandList Scrolled;
    Scrolled.PushClip(0, FRectangle(FIntVector2(0, 50 * 16), 400, 4 * 16));
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

    FString LastSelection;
    int32   NumSelectionCallbacks = 0;

    FRichTextBlock::FDesc Desc;
    Desc.Runs.Add(FTextRun(TEXT("[Error] "), Font.Get(), FUIStyle::GetDefault().Colors.Accent));
    Desc.Runs.Add(FTextRun(TEXT("disk full"), Font.Get(), FUIStyle::GetDefault().Colors.Text));
    Desc.OnSelectionChanged = FOnSelectionChanged::CreateLambda([&](const FString& SelectedText)
    {
        LastSelection = SelectedText;
        NumSelectionCallbacks++;
    });

    TSharedPtr<FRichTextBlock> Block = FRichTextBlock::Create(Desc);
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_SECTION(TEXT("Nothing is selected to begin with"));
    TEST_EXPECT(!Block->HasSelection());
    TEST_EXPECT(Block->GetSelectedText().IsEmpty());

    TEST_SECTION(TEXT("A drag selects the characters it crossed, whatever run they came from"));
    DragSelection(Block, FIntVector2(2 * 8 + 1, 8), FIntVector2(11 * 8 + 1, 8));

    TEST_EXPECT(Block->HasSelection());
    TEST_EXPECT_EQ(Block->GetSelectionStart(), 2);
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 11);
    TEST_EXPECT_EQ(Block->GetSelectedText(), FString(TEXT("rror] dis")));

    TEST_SECTION(TEXT("Which is what the selection callback was told"));
    TEST_EXPECT_EQ(LastSelection, FString(TEXT("rror] dis")));

    TEST_SECTION(TEXT("Dragging back the other way selects the same span"));
    DragSelection(Block, FIntVector2(11 * 8 + 1, 8), FIntVector2(2 * 8 + 1, 8));

    TEST_EXPECT_EQ(Block->GetSelectionStart(), 2);
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 11);

    TEST_SECTION(TEXT("A press with no drag places a caret rather than selecting anything"));
    Block->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, FIntVector2(4 * 8 + 1, 8), true));
    Block->OnMouseButtonUp(MakeButtonEvent(EInputEventType::MouseButtonUp, FIntVector2(4 * 8 + 1, 8), false));

    TEST_EXPECT(!Block->HasSelection());

    TEST_SECTION(TEXT("Select all takes the whole text"));
    Block->SelectAll();

    TEST_EXPECT_EQ(Block->GetSelectedText(), FString(TEXT("[Error] disk full")));
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 17);

    TEST_SECTION(TEXT("A selection is drawn behind the text, one box per line it covers"));
    FDrawCommandList Unselected;
    Block->ClearSelection();
    DrawElement(Block, Unselected);

    FDrawCommandList Selected;
    Block->SelectAll();
    DrawElement(Block, Selected);

    TEST_EXPECT_EQ(CountBoxes(Selected) - CountBoxes(Unselected), 1);

    TEST_SECTION(TEXT("A selection spanning wrapped lines is one box per line"));
    TSharedPtr<FRichTextBlock> Wrapped = MakeBlock(Font, { TEXT("aaaa bbbb cccc") }, true);
    LayoutElement(Wrapped, FRectangle(FIntVector2(0, 0), 40, 200));

    TEST_EXPECT_EQ(Wrapped->GetLayout().GetLines().Size(), 3);

    FDrawCommandList WrappedEmpty;
    DrawElement(Wrapped, WrappedEmpty);

    FDrawCommandList WrappedSelected;
    Wrapped->SelectAll();
    DrawElement(Wrapped, WrappedSelected);

    TEST_EXPECT_EQ(CountBoxes(WrappedSelected) - CountBoxes(WrappedEmpty), 3);

    TEST_SECTION(TEXT("Setting a span out of range clamps rather than reading past the text"));
    Block->SetSelection(-10, 500);

    TEST_EXPECT_EQ(Block->GetSelectionStart(), 0);
    TEST_EXPECT_EQ(Block->GetSelectionEnd(), 17);

    TEST_SECTION(TEXT("A block that is not selectable ignores a drag"));
    FRichTextBlock::FDesc FixedDesc;
    FixedDesc.Runs.Add(FTextRun(TEXT("read only"), Font.Get(), FUIStyle::GetDefault().Colors.Text));
    FixedDesc.bIsSelectable = false;

    TSharedPtr<FRichTextBlock> Fixed = FRichTextBlock::Create(FixedDesc);
    LayoutElement(Fixed, FRectangle(FIntVector2(0, 0), 400, 60));

    DragSelection(Fixed, FIntVector2(1, 8), FIntVector2(60, 8));
    TEST_EXPECT(!Fixed->HasSelection());

    TEST_SECTION(TEXT("Text that can be dragged through puts the text cursor under the pointer, so the log reads as selectable"));
    TSharedPtr<FRichTextBlock> Hovered = MakeBlock(Font, { TEXT("hover me") });
    LayoutElement(Hovered, FRectangle(FIntVector2(0, 0), 400, 60));

    ECursor Cursor = ECursor::Arrow;
    TEST_EXPECT(!Hovered->GetCursor(Cursor));

    Hovered->OnMouseEntered(MakeMoveEvent(FIntVector2(4 * 8 + 1, 8)));

    TEST_EXPECT(Hovered->GetCursor(Cursor));
    TEST_EXPECT(Cursor == ECursor::TextInput);

    TEST_SECTION(TEXT("Leaving it hands the shape back"));
    Hovered->OnMouseLeft(MakeMoveEvent(FIntVector2(900, 900)));

    TEST_EXPECT(!Hovered->GetCursor(Cursor));

    TEST_SECTION(TEXT("Text that cannot be selected keeps the arrow, which is what a tool tip wants"));
    Fixed->OnMouseEntered(MakeMoveEvent(FIntVector2(1, 8)));

    TEST_EXPECT(!Fixed->GetCursor(Cursor));

    TEST_SECTION(TEXT("Dragging past the top of a host scroll box moves the view and grows the selection"));
    FRichTextBlock::FDesc TallDesc;
    for (int32 Line = 0; Line < 20; ++Line)
    {
        TallDesc.Runs.Add(FTextRun(TEXT("line of log text\n"), Font.Get(), FUIStyle::GetDefault().Colors.Text));
    }

    TSharedPtr<FRichTextBlock> TallBlock = FRichTextBlock::Create(TallDesc);
    TSharedPtr<FScrollBox>     Host      = FScrollBox::Create();
    Host->SetContent(TallBlock);
    LayoutElement(Host, FRectangle(FIntVector2(0, 0), 200, 48));
    Host->SetScrollOffset(FScrollBox::DefaultScrollAmountPerWheelStep * 2);

    const int32 OffsetBefore = Host->GetScrollOffset();
    TEST_EXPECT(OffsetBefore > 0);

    TallBlock->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, FIntVector2(8, 24), true));
    TallBlock->OnMouseMove(MakeMoveEvent(FIntVector2(8, -20)));

    TEST_EXPECT(Host->GetScrollOffset() < OffsetBefore);
    TEST_EXPECT(TallBlock->HasSelection());

    TEST_END();
}

bool RichTextSearch_Test()
{
    TEST_BEGIN();

    const TSharedPtr<IFontFace> Font = CreateFont();

    TSharedPtr<FRichTextBlock> Block = MakeBlock(Font, { TEXT("the cat sat on "), TEXT("the mat") });
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_SECTION(TEXT("No search means no matches"));
    TEST_EXPECT(Block->GetSearchMatches().IsEmpty());

    TEST_SECTION(TEXT("Every occurrence is found, including the one in the second run"));
    Block->SetSearchText(TEXT("the"));

    TEST_EXPECT_EQ(Block->GetSearchMatches().Size(), 2);
    TEST_EXPECT_EQ(Block->GetSearchMatches()[0], 0);
    TEST_EXPECT_EQ(Block->GetSearchMatches()[1], 15);

    TEST_SECTION(TEXT("A match crossing the boundary between two runs is still one match"));
    Block->SetSearchText(TEXT("on the"));

    TEST_EXPECT_EQ(Block->GetSearchMatches().Size(), 1);
    TEST_EXPECT_EQ(Block->GetSearchMatches()[0], 12);

    TEST_SECTION(TEXT("Matches do not overlap"));
    TSharedPtr<FRichTextBlock> Repeated = MakeBlock(Font, { TEXT("aaaa") });
    LayoutElement(Repeated, FRectangle(FIntVector2(0, 0), 400, 60));

    Repeated->SetSearchText(TEXT("aa"));
    TEST_EXPECT_EQ(Repeated->GetSearchMatches().Size(), 2);

    TEST_SECTION(TEXT("A search with no hits finds none, and the text is untouched"));
    Block->SetSearchText(TEXT("dog"));

    TEST_EXPECT(Block->GetSearchMatches().IsEmpty());
    TEST_EXPECT_EQ(Block->GetText(), FString(TEXT("the cat sat on the mat")));

    TEST_SECTION(TEXT("Clearing the search clears the matches"));
    Block->SetSearchText(TEXT("the"));
    Block->SetSearchText(TEXT(""));

    TEST_EXPECT(Block->GetSearchMatches().IsEmpty());

    TEST_SECTION(TEXT("Each hit is highlighted with its own box"));
    FDrawCommandList Unhighlighted;
    DrawElement(Block, Unhighlighted);

    FDrawCommandList Highlighted;
    Block->SetSearchText(TEXT("the"));
    DrawElement(Block, Highlighted);

    TEST_EXPECT_EQ(CountBoxes(Highlighted) - CountBoxes(Unhighlighted), 2);

    TEST_SECTION(TEXT("A match is filled in the search color rather than the one a selection takes"));
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

    TEST_SECTION(TEXT("Replacing the text re-runs the search against it"));
    Block->SetRuns({ FTextRun(TEXT("the the the"), Font.Get(), FUIStyle::GetDefault().Colors.Text) });
    LayoutElement(Block, FRectangle(FIntVector2(0, 0), 400, 60));

    TEST_EXPECT_EQ(Block->GetSearchMatches().Size(), 3);

    TEST_SECTION(TEXT("A case-sensitive search does not treat Create and create as the same"));
    TSharedPtr<FRichTextBlock> Cased = MakeBlock(Font, { TEXT("Create create CREATE") });
    LayoutElement(Cased, FRectangle(FIntVector2(0, 0), 400, 60));

    Cased->SetSearchText(TEXT("Create"));
    TEST_EXPECT_EQ(Cased->GetSearchMatches().Size(), 1);
    TEST_EXPECT_EQ(Cased->GetSearchMatches()[0], 0);

    TEST_SECTION(TEXT("Ignoring case finds every spelling of the same word"));
    Cased->SetSearchCaseType(EStringCaseType::NoCase);
    TEST_EXPECT_EQ(Cased->GetSearchMatches().Size(), 3);

    TEST_SECTION(TEXT("Turning case matching back on drops the extra hits"));
    Cased->SetSearchCaseType(EStringCaseType::CaseSensitive);
    TEST_EXPECT_EQ(Cased->GetSearchMatches().Size(), 1);

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
    LayoutElement(LogView, FRectangle(FIntVector2(0, 0), 400, 200));

    TEST_SECTION(TEXT("A fresh view holds nothing"));
    TEST_EXPECT_EQ(LogView->GetNumLines(), 0);

    TEST_SECTION(TEXT("A logged line is held once the view has ticked, since logging can come from any thread"));
    LogView->Log(ELogSeverity::Info, TEXT("engine started"));

    TEST_EXPECT_EQ(LogView->GetNumLines(), 0);
    LayoutElement(LogView, FRectangle(FIntVector2(0, 0), 400, 200));

    TEST_EXPECT_EQ(LogView->GetNumLines(), 1);
    TEST_EXPECT_EQ(LogView->GetLines()[0].Message, FString(TEXT("engine started")));
    TEST_EXPECT_EQ(LogView->GetLines()[0].Severity, ELogSeverity::Info);

    TEST_SECTION(TEXT("Flush folds them in without waiting for a tick"));
    LogView->Log(ELogSeverity::Warning, TEXT("shader cache cold"));
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 2);
    TEST_EXPECT_EQ(LogView->GetLines()[1].Severity, ELogSeverity::Warning);

    TEST_SECTION(TEXT("The message-only overload logs at info"));
    LogView->Log(TEXT("plain"));
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetLines()[2].Severity, ELogSeverity::Info);

    TEST_SECTION(TEXT("The text carries the severity prefix and its colour, and the break is a run of its own"));
    const TArray<FTextRun>& Runs = LogView->GetTextBlock()->GetLayout().GetSourceRuns();

    TEST_EXPECT_EQ(Runs.Size(), 8);
    TEST_EXPECT_EQ(Runs[0].Text, FString(TEXT("[Info] ")));
    TEST_EXPECT_EQ(Runs[1].Text, FString(TEXT("engine started")));
    TEST_EXPECT_EQ(Runs[2].Text, FString(TEXT("\n")));
    TEST_EXPECT_EQ(Runs[3].Text, FString(TEXT("[Warning] ")));

    TEST_EXPECT_EQ(Runs[3].Tint, FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning));
    TEST_EXPECT_EQ(Runs[4].Tint, FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning));

    TEST_SECTION(TEXT("Which still reads back as one line per message"));
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetText(), FString(TEXT("[Info] engine started\n[Warning] shader cache cold\n[Info] plain")));

    TEST_SECTION(TEXT("Past the cap the oldest lines are dropped"));
    LogView->Log(TEXT("fourth"));
    LogView->Log(TEXT("fifth"));
    LogView->Log(TEXT("sixth"));
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 4);
    TEST_EXPECT_EQ(LogView->GetLines()[0].Message, FString(TEXT("plain")));
    TEST_EXPECT_EQ(LogView->GetLines()[3].Message, FString(TEXT("sixth")));

    TEST_SECTION(TEXT("Shrinking the cap trims what is already held"));
    LogView->SetMaxLineCount(2);

    TEST_EXPECT_EQ(LogView->GetNumLines(), 2);
    TEST_EXPECT_EQ(LogView->GetLines()[0].Message, FString(TEXT("fifth")));

    TEST_SECTION(TEXT("Clearing empties both the lines and the text"));
    LogView->Clear();
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 0);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetText(), FString());

    TEST_SECTION(TEXT("Registering with the logger routes what the engine logs into the view"));
    LogView->SetMaxLineCount(16);
    LogView->RegisterWithLogger();

    LOG_INFO("routed through the logger");
    LogView->Flush();

    LogView->UnregisterFromLogger();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 1);
    TEST_EXPECT(LogView->GetLines()[0].Message.Contains(TEXT("routed through the logger")));

    TEST_SECTION(TEXT("And unregistering stops it"));
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
    LayoutElement(LogView, FRectangle(FIntVector2(0, 0), 400, 200));

    LogView->Log(ELogSeverity::Info, TEXT("loading level"));
    LogView->Log(ELogSeverity::Warning, TEXT("texture missing"));
    LogView->Log(ELogSeverity::Error, TEXT("device lost"));
    LogView->Log(ELogSeverity::Info, TEXT("loading complete"));
    LogView->Flush();

    TEST_SECTION(TEXT("Everything shows at the lowest severity"));
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 4);

    TEST_SECTION(TEXT("Raising the floor hides what is below it without dropping it"));
    LogView->SetMinimumSeverity(ELogSeverity::Warning);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetNumLines(), 4);
    TEST_EXPECT_EQ(LogView->GetVisibleMessages()[0], FString(TEXT("texture missing")));

    TEST_SECTION(TEXT("Which is what the text is rebuilt from, once the frame that asked for it comes round"));
    LogView->Flush();

    TEST_EXPECT(!LogView->GetTextBlock()->GetText().Contains(TEXT("loading level")));
    TEST_EXPECT(LogView->GetTextBlock()->GetText().Contains(TEXT("device lost")));

    TEST_SECTION(TEXT("Errors only leaves one"));
    LogView->SetMinimumSeverity(ELogSeverity::Error);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);

    TEST_SECTION(TEXT("Lowering it again brings the rest back"));
    LogView->SetMinimumSeverity(ELogSeverity::Info);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 4);

    TEST_SECTION(TEXT("A search that only highlights leaves every line showing"));
    LogView->SetSearchText(TEXT("loading"), false);
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 4);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 2);

    TEST_SECTION(TEXT("A search that filters hides the lines with no match"));
    LogView->SetSearchText(TEXT("loading"), true);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetVisibleMessages()[1], FString(TEXT("loading complete")));

    TEST_SECTION(TEXT("The two filters compose rather than replacing one another"));
    LogView->SetMinimumSeverity(ELogSeverity::Warning);
    LogView->SetSearchText(TEXT("e"), true);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);

    LogView->SetSearchText(TEXT("device"), true);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);

    TEST_SECTION(TEXT("Clearing the search brings back what only it was hiding"));
    LogView->SetSearchText(TEXT(""), true);

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);

    TEST_SECTION(TEXT("A line arriving while a filter is up obeys it"));
    LogView->SetMinimumSeverity(ELogSeverity::Error);
    LogView->Log(ELogSeverity::Info, TEXT("ignored for now"));
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumLines(), 5);
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);

    TEST_SECTION(TEXT("Typing a word is one rebuild at the end of the frame rather than one for every letter"));
    LogView->SetMinimumSeverity(ELogSeverity::Info);
    LogView->SetSearchText(TEXT(""), false);
    LogView->Flush();

    const FString SettledText = LogView->GetTextBlock()->GetText();
    LogView->SetSearchText(TEXT("l"), true);
    LogView->SetSearchText(TEXT("lo"), true);
    LogView->SetSearchText(TEXT("loa"), true);

    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetText(), SettledText);

    TEST_SECTION(TEXT("And what it settles on is what the last keystroke asked for"));
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 2);
    TEST_EXPECT(!LogView->GetTextBlock()->GetText().Contains(TEXT("device lost")));

    TEST_SECTION(TEXT("By default Create and create are the same search"));
    LogView->SetSearchText(TEXT(""), false);
    LogView->Log(ELogSeverity::Info, TEXT("Create actor"));
    LogView->Log(ELogSeverity::Info, TEXT("create material"));
    LogView->Flush();

    TEST_EXPECT(!LogView->IsSearchCaseSensitive());

    LogView->SetSearchText(TEXT("Create"), true);
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 2);

    TEST_SECTION(TEXT("Match Case hides the line that only matches ignoring case"));
    LogView->SetSearchCaseSensitive(true);
    LogView->Flush();

    TEST_EXPECT(LogView->IsSearchCaseSensitive());
    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 1);
    TEST_EXPECT_EQ(LogView->GetVisibleMessages()[0], FString(TEXT("Create actor")));
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 1);

    TEST_SECTION(TEXT("Turning Match Case off brings the other spelling back"));
    LogView->SetSearchCaseSensitive(false);
    LogView->Flush();

    TEST_EXPECT_EQ(LogView->GetNumVisibleLines(), 2);
    TEST_EXPECT_EQ(LogView->GetTextBlock()->GetSearchMatches().Size(), 2);

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

    const FRectangle Bounds(FIntVector2(0, 0), 400, 40);
    LayoutElement(LogView, Bounds);

    TEST_SECTION(TEXT("An empty view is at the bottom, because there is nowhere else to be"));
    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_SECTION(TEXT("Following the tail keeps the last line showing as more arrive"));
    for (int32 Index = 0; Index < 20; ++Index)
    {
        LogView->Log(FString::Printf(TEXT("line %d"), Index));
    }

    LayoutElement(LogView, Bounds);

    TEST_EXPECT(LogView->GetScrollBox()->GetMaxScrollOffset() > 0);
    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_SECTION(TEXT("Scrolling away from the tail stops the following"));
    LogView->GetScrollBox()->SetScrollOffset(0);
    TEST_EXPECT(!LogView->IsScrolledToBottom());

    LogView->Log(TEXT("arrived while scrolled up"));
    LayoutElement(LogView, Bounds);

    TEST_EXPECT_EQ(LogView->GetScrollBox()->GetScrollOffset(), 0);
    TEST_EXPECT(!LogView->IsScrolledToBottom());

    TEST_SECTION(TEXT("Scrolling back to the tail re-arms it"));
    LogView->ScrollToBottom();
    TEST_EXPECT(LogView->IsScrolledToBottom());

    LogView->Log(TEXT("arrived after coming back"));
    LayoutElement(LogView, Bounds);

    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_SECTION(TEXT("Turning it off leaves the view where the reader put it"));
    LogView->SetAutoScroll(false);
    LogView->GetScrollBox()->SetScrollOffset(0);

    LogView->Log(TEXT("ignored by the scroll"));
    LayoutElement(LogView, Bounds);

    TEST_EXPECT_EQ(LogView->GetScrollBox()->GetScrollOffset(), 0);

    TEST_SECTION(TEXT("Turning it back on jumps to the tail straight away"));
    LogView->SetAutoScroll(true);
    TEST_EXPECT(LogView->IsScrolledToBottom());

    TEST_END();
}
