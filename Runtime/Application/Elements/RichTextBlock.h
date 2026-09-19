#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Text/TextLayout.h"

/** @brief Called when the selection changed, carrying the text now selected. */
DECLARE_DELEGATE(FOnSelectionChanged, const String& /*SelectedText*/);

class APPLICATION_API FRichTextBlock final : public FInteractiveElement
{
public:
    struct FDesc
    {
        TArray<FTextRun>    Runs;
        FMargin             Margin;
        int32               WrapWidth = 0;
        bool                bIsSelectable : 1 = true;
        bool                bAutoWrapText : 1 = true;
        FOnSelectionChanged OnSelectionChanged;
    };

public:
    static TSharedPtr<FRichTextBlock> Create(const FDesc& Desc);

public:
    FRichTextBlock();
    virtual ~FRichTextBlock();

    /**
     * @brief Initializes the block with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual bool GetCursor(ECursor& OutCursor) const override;

    /**
     * @brief Replaces the text, dropping the selection with it.
     *
     * @param InRuns The runs, in reading order.
     */
    void SetRuns(const TArray<FTextRun>& InRuns);

    /**
     * @brief Replaces the text and the search together, which finds the matches once rather than once for the
     * new text and again for the new search.
     *
     * @param InRuns       The runs, in reading order.
     * @param InSearchText The text to look for. An empty string clears the search.
     */
    void SetRunsAndSearchText(const TArray<FTextRun>& InRuns, const String& InSearchText);

    /**
     * @brief Appends one run to the end of the text.
     *
     * @param Run The run to append.
     */
    void AppendRun(const FTextRun& Run);

    /** @brief Drops every run and the selection. */
    void ClearRuns();

    /**
     * @brief Highlights every occurrence of a string, which is what a find box drives.
     *
     * @param InSearchText The text to look for. An empty string clears the search.
     */
    void SetSearchText(const String& InSearchText);

    /** @return The string the block highlights every occurrence of, or empty when there is no search. */
    NODISCARD FORCEINLINE const String& GetSearchText() const
    {
        return SearchText;
    }

    /** @return The start of each search match in the concatenated text, in order and never overlapping. */
    NODISCARD FORCEINLINE const TArray<int32>& GetSearchMatches() const
    {
        return SearchMatches;
    }

    /** @return True when the anchor and the cursor sit apart, rather than a caret resting somewhere. */
    NODISCARD bool HasSelection() const;

    /** @return The lower of the anchor and the cursor, as an index into the concatenated text. */
    NODISCARD int32 GetSelectionStart() const;

    /** @return The higher of the anchor and the cursor, which is one past the last selected character. */
    NODISCARD int32 GetSelectionEnd() const;

    /** @return The text inside the selection, or an empty string when nothing is selected. */
    NODISCARD String GetSelectedText() const;

    /** @brief Selects everything, which is what the context menu and the select-all chord both do. */
    void SelectAll();

    /** @brief Drops the selection. */
    void ClearSelection();

    /**
     * @brief Selects a span of the concatenated text, clamped to what there is.
     *
     * @param StartIndex Where the selection begins.
     * @param EndIndex   Where it ends.
     */
    void SetSelection(int32 StartIndex, int32 EndIndex);

    /** @brief Puts the selection on the clipboard, or the whole text when nothing is selected. */
    void CopyToClipboard() const;

    /** @return The whole text with the runs concatenated, which every selection and match index refers to. */
    NODISCARD String GetText() const;

    /**
     * @brief Gets the laid-out text, for a caller that needs its lines or its measurements.
     *
     * @return The layout, wrapped to the width of the last measure, arrange or draw.
     */
    NODISCARD FORCEINLINE const FTextLayout& GetLayout() const
    {
        return Layout;
    }

    /**
     * @brief The character under a point.
     *
     * @param ClientPosition The point, in the space the block was arranged in.
     * @return The index into the concatenated text.
     */
    NODISCARD int32 FindCharacterIndexAt(const IntVector2& ClientPosition) const;

protected:

    // FInteractiveElement Interface
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;

    virtual bool IsPressable() const override { return false; }

private:
    void RefreshLayout(int32 AvailableWidth) const;
    void RefreshSearchMatches();
    void GatherRangeRectangles(const TArray<FTextRange>& Ranges, TArray<FRectangle>& OutRectangles) const;

    NODISCARD FRectangle GetTextBounds() const;

    mutable FTextLayout Layout;
    FMargin             Margin;
    String              SearchText;
    TArray<int32>       SearchMatches;
    TArray<FTextRange>  SearchRanges;
    int32               WrapWidth;
    int32               SelectionAnchor;
    int32               SelectionCursor;
    bool                bIsSelectable;
    bool                bAutoWrapText;
    FOnSelectionChanged OnSelectionChangedDelegate;
};
