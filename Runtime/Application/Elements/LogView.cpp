#include "Application/Console/ConsoleLogBuffer.h"
#include "Application/Elements/LogView.h"
#include "Application/Elements/RichTextBlock.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Misc/OutputDeviceManager.h"

constexpr uint8 LOG_VIEW_ALL_SEVERITIES = 0x7;

TSharedPtr<FLogView> FLogView::Create(const FDesc& Desc)
{
    TSharedPtr<FLogView> NewView = MakeSharedPtr<FLogView>();
    NewView->Initialize(Desc);
    return NewView;
}

FLogView::FLogView()
    : FCompoundElement()
    , IOutputDevice()
    , ScrollBox(nullptr)
    , TextBlock(nullptr)
    , Font(nullptr)
    , PendingLinesCS()
    , PendingLines()
    , Lines()
    , SearchText()
    , VisibleSeverities(LOG_VIEW_ALL_SEVERITIES)
    , MaxLineCount(DefaultMaxLineCount)
    , bFilterToMatches(false)
    , bAutoScroll(true)
    , bShowSeverityPrefix(true)
    , bLayoutIsStale(true)
    , bIsRegisteredWithLogger(false)
{
}

FLogView::~FLogView()
{
    UnregisterFromLogger();
}

void FLogView::Initialize(const FDesc& Desc)
{
    Font                = Desc.Font;
    MaxLineCount        = Math::Max(1, Desc.MaxLineCount);
    bAutoScroll         = Desc.bAutoScroll;
    bShowSeverityPrefix = Desc.bShowSeverityPrefix;

    SetMinimumSeverity(Desc.MinimumSeverity);

    FRichTextBlock::FDesc TextDesc;
    TextDesc.Margin        = FMargin(12, 8);
    TextDesc.bIsSelectable = true;
    TextDesc.bAutoWrapText = true;

    TextBlock = FRichTextBlock::Create(TextDesc);

    ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(TextBlock);

    SetContent(ScrollBox);
}

IntVector2 FLogView::PrepareDesiredSize()
{
    DrainPendingLines();

    if (bLayoutIsStale)
    {
        RebuildLayout();
    }

    return FCompoundElement::PrepareDesiredSize();
}

void FLogView::Log(const String& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FLogView::Log(ELogSeverity Severity, const String& Message)
{
    TScopedLock Lock(PendingLinesCS);
    PendingLines.Emplace(Message, Severity);
}

void FLogView::Flush()
{
    DrainPendingLines();

    if (bLayoutIsStale)
    {
        RebuildLayout();
    }
}

void FLogView::RegisterWithLogger()
{
    if (!bIsRegisteredWithLogger)
    {
        FOutputDeviceManager::Get()->RegisterOutputDevice(this);
        bIsRegisteredWithLogger = true;
    }
}

void FLogView::UnregisterFromLogger()
{
    if (bIsRegisteredWithLogger)
    {
        FOutputDeviceManager::Get()->UnregisterOutputDevice(this);
        bIsRegisteredWithLogger = false;
    }
}

void FLogView::SetSeverityVisible(ELogSeverity Severity, bool bIsVisible)
{
    const uint8 Bit  = GetSeverityBit(Severity);
    const uint8 Mask = bIsVisible ? (VisibleSeverities | Bit) : static_cast<uint8>(VisibleSeverities & ~Bit);

    if (VisibleSeverities == Mask)
    {
        return;
    }

    VisibleSeverities = Mask;
    bLayoutIsStale    = true;
}

bool FLogView::IsSeverityVisible(ELogSeverity Severity) const
{
    return (VisibleSeverities & GetSeverityBit(Severity)) != 0;
}

void FLogView::SetMinimumSeverity(ELogSeverity InMinimumSeverity)
{
    SetSeverityVisible(ELogSeverity::Info, ELogSeverity::Info >= InMinimumSeverity);
    SetSeverityVisible(ELogSeverity::Warning, ELogSeverity::Warning >= InMinimumSeverity);
    SetSeverityVisible(ELogSeverity::Error, ELogSeverity::Error >= InMinimumSeverity);
}

void FLogView::SetSearchText(const String& InSearchText, bool bInFilterToMatches)
{
    if (SearchText == InSearchText && bFilterToMatches == bInFilterToMatches)
    {
        return;
    }

    SearchText       = InSearchText;
    bFilterToMatches = bInFilterToMatches;
    bLayoutIsStale   = true;
}

void FLogView::SetAutoScroll(bool bInAutoScroll)
{
    bAutoScroll = bInAutoScroll;

    if (bAutoScroll)
    {
        ScrollToBottom();
    }
}

bool FLogView::IsScrolledToBottom() const
{
    return ScrollBox ? ScrollBox->IsScrolledToEnd() : true;
}

void FLogView::ScrollToBottom()
{
    if (!ScrollBox)
    {
        return;
    }

    ScrollBox->SetScrollOffset(ScrollBox->GetMaxScrollOffset());
    ScrollBox->ScrollToEnd();
}

void FLogView::Clear()
{
    {
        TScopedLock Lock(PendingLinesCS);
        PendingLines.Clear();
    }

    Lines.Clear();
    bLayoutIsStale = true;
}

void FLogView::SelectAll()
{
    if (TextBlock)
    {
        TextBlock->SelectAll();
    }
}

void FLogView::CopyToClipboard() const
{
    if (TextBlock)
    {
        TextBlock->CopyToClipboard();
    }
}

void FLogView::SetMaxLineCount(int32 InMaxLineCount)
{
    MaxLineCount = Math::Max(1, InMaxLineCount);

    const int32 NumLinesBefore = Lines.Size();
    TrimToMaxLineCount();

    if (Lines.Size() != NumLinesBefore)
    {
        bLayoutIsStale = true;
    }
}

TArray<FLogLine> FLogView::GetLines() const
{
    return Lines;
}

TArray<String> FLogView::GetVisibleMessages() const
{
    TArray<String> Messages;
    for (const FLogLine& Line : Lines)
    {
        if (IsLineVisible(Line))
        {
            Messages.Add(Line.Message);
        }
    }

    return Messages;
}

int32 FLogView::GetNumVisibleLines() const
{
    int32 NumVisibleLines = 0;
    for (const FLogLine& Line : Lines)
    {
        NumVisibleLines += IsLineVisible(Line) ? 1 : 0;
    }

    return NumVisibleLines;
}

int32 FLogView::GetNumLines() const
{
    return Lines.Size();
}

void FLogView::DrainPendingLines()
{
    TArray<FLogLine> Arrived;
    {
        TScopedLock Lock(PendingLinesCS);
        if (PendingLines.IsEmpty())
        {
            return;
        }

        Arrived = ::Move(PendingLines);
        PendingLines.Clear();
    }

    const bool bWasAtBottom = IsScrolledToBottom();
    for (const FLogLine& Line : Arrived)
    {
        Lines.Add(Line);
    }

    TrimToMaxLineCount();
    bLayoutIsStale = true;

    if (bAutoScroll && bWasAtBottom)
    {
        ScrollToBottom();
    }
}

void FLogView::RebuildLayout()
{
    bLayoutIsStale = false;

    if (!TextBlock)
    {
        return;
    }

    TArray<FTextRun> Runs;
    for (const FLogLine& Line : Lines)
    {
        if (!IsLineVisible(Line))
        {
            continue;
        }

        const FFloatColor Color = FConsoleLogBuffer::GetSeverityColor(Line.Severity);
        if (bShowSeverityPrefix)
        {
            Runs.Emplace(GetSeverityPrefix(Line.Severity), Font.Get(), Color);
        }

        Runs.Emplace(Line.Message, Font.Get(), Color);
        Runs.Emplace("\n", Font.Get(), Color);
    }

    TextBlock->SetRunsAndSearchText(Runs, SearchText);
}

bool FLogView::IsLineVisible(const FLogLine& Line) const
{
    if (!IsSeverityVisible(Line.Severity))
    {
        return false;
    }

    if (!bFilterToMatches || SearchText.IsEmpty())
    {
        return true;
    }

    return Line.Message.Contains(SearchText);
}

void FLogView::TrimToMaxLineCount()
{
    const int32 NumLinesToDrop = Lines.Size() - MaxLineCount;
    if (NumLinesToDrop > 0)
    {
        Lines.RemoveAt(0, NumLinesToDrop);
    }
}

const CHAR* FLogView::GetSeverityPrefix(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Warning:
            return "[Warning] ";

        case ELogSeverity::Error:
            return "[Error] ";

        default:
            return "[Info] ";
    }
}

uint8 FLogView::GetSeverityBit(ELogSeverity Severity)
{
    switch (Severity)
    {
        case ELogSeverity::Warning:
            return 0x2;

        case ELogSeverity::Error:
            return 0x4;

        default:
            return 0x1;
    }
}
