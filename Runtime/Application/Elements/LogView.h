#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Misc/IOutputDevice.h"
#include "Core/Platform/CriticalSection.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Text/IFontFace.h"

class FRichTextBlock;
class FScrollBox;

struct FLogLine
{
    FLogLine()
        : Message()
        , Severity(ELogSeverity::Info)
    {
    }

    FLogLine(const String& InMessage, ELogSeverity InSeverity)
        : Message(InMessage)
        , Severity(InSeverity)
    {
    }

    String       Message;
    ELogSeverity Severity;
};

class APPLICATION_API FLogView final : public FCompoundElement, public IOutputDevice
{
public:

    /** @brief Matches the console, which is the other thing in the engine holding log lines. */
    static constexpr int32 DefaultMaxLineCount = 1000;

public:
    struct FDesc
    {
        TSharedPtr<IFontFace> Font = nullptr;
        int32                 MaxLineCount = DefaultMaxLineCount;
        ELogSeverity          MinimumSeverity = ELogSeverity::Info;
        bool                  bAutoScroll : 1 = true;
        bool                  bShowSeverityPrefix : 1 = true;
    };

public:
    static TSharedPtr<FLogView> Create(const FDesc& Desc);

public:
    FLogView();
    virtual ~FLogView();

    /**
     * @brief Initializes the view with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 PrepareDesiredSize() override;

    // IOutputDevice Interface
    virtual void Log(const String& Message) override;
    virtual void Log(ELogSeverity Severity, const String& Message) override;
    virtual void Flush() override;

    /** @brief Starts receiving everything the engine logs. */
    void RegisterWithLogger();

    /** @brief Stops receiving what the engine logs. Safe to call when never registered. */
    void UnregisterFromLogger();

    /**
     * @brief Shows or hides one severity, leaving the other two as they were. The three verbosities filter
     * independently, so hiding warnings does not touch the errors above them.
     *
     * @param Severity  The severity to show or hide.
     * @param bIsVisible True to show its lines.
     */
    void SetSeverityVisible(ELogSeverity Severity, bool bIsVisible);

    /**
     * @brief Gets whether one severity's lines are shown.
     *
     * @param Severity The severity to test.
     * @return True while its lines are shown.
     */
    NODISCARD bool IsSeverityVisible(ELogSeverity Severity) const;

    /**
     * @brief Hides every line below a severity, which is the threshold form of the per-severity mask.
     *
     * @param InMinimumSeverity The least severity still shown.
     */
    void SetMinimumSeverity(ELogSeverity InMinimumSeverity);

    /**
     * @brief Highlights matching substrings and, when filtering, hides lines with no match.
     *
     * @param InSearchText      The text to look for. An empty string clears the search.
     * @param bInFilterToMatches True to hide non-matching lines rather than only highlighting.
     */
    void SetSearchText(const String& InSearchText, bool bInFilterToMatches);

    /** @return The string the lines are searched for, which is empty when there is no search. */
    NODISCARD FORCEINLINE const String& GetSearchText() const
    {
        return SearchText;
    }

    /** @return True when non-matching lines are hidden rather than only left unhighlighted. */
    NODISCARD FORCEINLINE bool IsFilteringToMatches() const
    {
        return bFilterToMatches;
    }

    /**
     * @brief Follows the tail as lines arrive, until the reader scrolls away from it.
     *
     * @param bInAutoScroll True to follow the tail.
     */
    void SetAutoScroll(bool bInAutoScroll);

    /** @return True while autoscroll is on, so the view follows the tail as lines arrive. */
    NODISCARD FORCEINLINE bool IsAutoScrollEnabled() const
    {
        return bAutoScroll;
    }

    /**
     * @brief Gets whether the last line is showing, which is what re-arms autoscroll after a manual scroll.
     *
     * @return True when the view sits at the bottom.
     */
    NODISCARD bool IsScrolledToBottom() const;

    /** @brief Scrolls to the last line. */
    void ScrollToBottom();

    /** @brief Drops every line. */
    void Clear();

    /** @brief Selects the whole of the visible text. */
    void SelectAll();

    /** @brief Puts the selection, or the whole visible text when there is none, on the clipboard. */
    void CopyToClipboard() const;

    /**
     * @brief Sets the line cap, trimming immediately when it shrinks.
     *
     * @param InMaxLineCount The new cap, which is clamped to at least one.
     */
    void SetMaxLineCount(int32 InMaxLineCount);

    /** @return The line cap, which is how many lines are kept before the oldest are dropped. */
    NODISCARD FORCEINLINE int32 GetMaxLineCount() const
    {
        return MaxLineCount;
    }

    /** @return Every line held, oldest first, whether or not the filter shows it. */
    NODISCARD TArray<FLogLine> GetLines() const;

    /** @return The messages the filters currently admit, in the order they were logged. */
    NODISCARD TArray<String> GetVisibleMessages() const;

    /** @return How many lines the filters currently admit. */
    NODISCARD int32 GetNumVisibleLines() const;

    /** @return How many lines are held, whether or not the filter shows them. */
    NODISCARD int32 GetNumLines() const;

    /** @return The block the lines are drawn by, which is what a selection or a search reads back from. */
    NODISCARD FORCEINLINE const TSharedPtr<FRichTextBlock>& GetTextBlock() const
    {
        return TextBlock;
    }

    /** @return The scroll box the text sits in, which owns the viewport it is scrolled inside. */
    NODISCARD FORCEINLINE const TSharedPtr<FScrollBox>& GetScrollBox() const
    {
        return ScrollBox;
    }

private:
    NODISCARD static const CHAR* GetSeverityPrefix(ELogSeverity Severity);
    NODISCARD static uint8 GetSeverityBit(ELogSeverity Severity);

    void DrainPendingLines();
    void RebuildLayout();
    void TrimToMaxLineCount();

    NODISCARD bool IsLineVisible(const FLogLine& Line) const;

    TSharedPtr<FScrollBox>     ScrollBox;
    TSharedPtr<FRichTextBlock> TextBlock;
    TSharedPtr<IFontFace>      Font;
    mutable FCriticalSection   PendingLinesCS;
    TArray<FLogLine>           PendingLines;
    TArray<FLogLine>           Lines;
    String                     SearchText;
    uint8                      VisibleSeverities;
    int32                      MaxLineCount;
    bool                       bFilterToMatches        : 1;
    bool                       bAutoScroll             : 1;
    bool                       bShowSeverityPrefix     : 1;
    bool                       bLayoutIsStale          : 1;
    bool                       bIsRegisteredWithLogger : 1;
};
