#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

struct APPLICATION_API FTreeItem : public TSharedFromThis<FTreeItem>
{
    /**
     * @brief Builds a childless node.
     *
     * @param InLabel    The text its row shows.
     * @param InUserData Whatever the host wants to find the node by again, which the node does not own.
     * @return The new node, with no parent until it is added to one.
     */
    NODISCARD static TSharedPtr<FTreeItem> Create(const String& InLabel, void* InUserData = nullptr);

    /**
     * @brief Appends a child and points it back at this node.
     *
     * @param Child The node to append, which may be null and is then ignored.
     */
    void AddChild(const TSharedPtr<FTreeItem>& Child);

    /** @brief Drops every child, releasing each subtree the host does not hold elsewhere. */
    void ClearChildren();

    /** @return True when the node holds at least one child, so its row carries a disclosure triangle. */
    NODISCARD bool HasChildren() const;

    /** @return How many parents stand above the node, which is zero for a root. */
    NODISCARD int32 GetDepth() const;

    /** @brief The text the node's row shows. */
    String Label;

    /** @brief Drawn ahead of the label whenever it has a texture. */
    FUIBrush Icon;

    /** @brief Whatever the host hung off the node, which the node does not own. */
    void* UserData = nullptr;

    /** @brief The children, in the order their rows appear. */
    TArray<TSharedPtr<FTreeItem>> Children;

    /** @brief True while the children have rows of their own below this one. */
    bool bIsExpanded = false;

    /** @brief The node this one hangs off, which is unset for a root. */
    TWeakPtr<FTreeItem> Parent;
};

/** @brief Called with the whole selection after it changed. */
DECLARE_DELEGATE(FOnTreeSelectionChanged, const TArray<TSharedPtr<FTreeItem>>& /*Selection*/);

/** @brief Called when a row was double clicked or Enter was pressed on it. */
DECLARE_DELEGATE(FOnTreeItemActivated, const TSharedPtr<FTreeItem>& /*Item*/);

/** @brief Called with the state an item's disclosure moved to. */
DECLARE_DELEGATE(FOnTreeItemExpansionChanged, const TSharedPtr<FTreeItem>& /*Item*/, bool /*bIsExpanded*/);

/** @brief Called once the cursor has moved far enough with the left button held on a row to mean a drag. */
DECLARE_DELEGATE(FOnTreeItemDragDetected, const TSharedPtr<FTreeItem>& /*Item*/, const FCursorEvent& /*CursorEvent*/);

class APPLICATION_API FTreeView final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The face every label is measured and drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The height of one row, in pixels. */
        int32 RowHeight = FUIStyle::GetDefault().Metrics.RowHeight;

        /** @brief How far one level of depth shifts a row's contents to the right, in pixels. */
        int32 IndentPerLevel = 16;

        /** @brief True to let a chord click or a shift click put more than one row in the selection. */
        bool bAllowMultiSelect = true;

        /** @brief Fired with the whole selection after it changed. */
        FOnTreeSelectionChanged OnSelectionChanged;

        /** @brief Fired when a row was double clicked or Enter was pressed on it. */
        FOnTreeItemActivated OnItemActivated;

        /** @brief Fired with the state an item's disclosure moved to. */
        FOnTreeItemExpansionChanged OnExpansionChanged;

        /** @brief Fired once the cursor has moved far enough with the left button held on a row to mean a drag. */
        FOnTreeItemDragDetected OnDragDetected;
    };

public:

    /** @brief How many rows one wheel step scrolls. */
    static constexpr int32 RowsPerWheelStep = 3;

    /** @brief How far the cursor has to travel with the left button held on a row before it means a drag, in pixels. */
    static constexpr int32 DragThreshold = 4;

public:
    static TSharedPtr<FTreeView> Create(const FDesc& Desc);

public:
    FTreeView();
    virtual ~FTreeView();

    /**
     * @brief Initializes the tree view with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual bool SupportsKeyboardFocus() const override;

    /**
     * @brief Replaces the nodes shown at depth zero, which is the whole model.
     *
     * @param InRoots The new roots, whose subtrees come with them.
     */
    void SetRootItems(const TArray<TSharedPtr<FTreeItem>>& InRoots);

    /** @return The nodes shown at depth zero, in the order their rows appear. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FTreeItem>>& GetRootItems() const
    {
        return RootItems;
    }

    /**
     * @brief Marks the flattened row list dirty, which is what a host calls after editing the model
     * behind the view's back.
     */
    void RequestRefresh();

    /**
     * @brief Gets the rows currently shown, which is the expanded tree flattened and filtered.
     *
     * @return The rows top to bottom, rebuilt first when a change marked them dirty.
     */
    NODISCARD const TArray<TSharedPtr<FTreeItem>>& GetVisibleRows() const;

    /**
     * @brief Replaces the selection without firing the delegate, which is how a host pushes a model
     * value in.
     *
     * @param InSelection The items to select, which need not all have rows.
     */
    void SetSelection(const TArray<TSharedPtr<FTreeItem>>& InSelection);

    /** @brief Empties the selection without firing the delegate. */
    void ClearSelection();

    /** @return The selected items, in the order they were added rather than in row order. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FTreeItem>>& GetSelection() const
    {
        return Selection;
    }

    /**
     * @brief Gets whether an item is in the selection.
     *
     * @param Item The item to look for.
     * @return True when the selection holds it, which a hidden item still can be.
     */
    NODISCARD bool IsSelected(const TSharedPtr<FTreeItem>& Item) const;

    /**
     * @brief Opens or closes an item and fires the expansion delegate when that moved it.
     *
     * @param Item      The item to open or close, which may be null and is then ignored.
     * @param bExpanded True to show the children below it.
     */
    void SetItemExpanded(const TSharedPtr<FTreeItem>& Item, bool bExpanded);

    /** @brief Opens every item in the model, without firing the expansion delegate for each one. */
    void ExpandAll();

    /** @brief Closes every item in the model, without firing the expansion delegate for each one. */
    void CollapseAll();

    /**
     * @brief Sets the text a row's label has to contain to survive. An item whose own label misses but
     * whose subtree holds a match is shown anyway and opened, so the match can be reached without
     * walking the path by hand.
     *
     * @param InFilter The text to match, compared without regard to case. An empty filter shows
     * everything.
     */
    void SetFilterText(const String& InFilter);

    /** @return The text a row's label has to contain to survive, which is empty when nothing is filtered. */
    NODISCARD FORCEINLINE const String& GetFilterText() const
    {
        return FilterText;
    }

    /**
     * @brief Scrolls the least amount that brings an item's row fully into view.
     *
     * @param Item The item to reveal, which does nothing when it has no row.
     */
    void ScrollToItem(const TSharedPtr<FTreeItem>& Item);

    /** @return How far the rows are scrolled, as an offset from the top of the first one in pixels. */
    NODISCARD FORCEINLINE int32 GetScrollOffset() const
    {
        return ScrollOffset;
    }

    /**
     * @brief Gets the item whose row covers a point.
     *
     * @param ClientPosition The position to test, in the client space the view was arranged in.
     * @return The item, or null when the point falls outside the view or below the last row.
     */
    NODISCARD TSharedPtr<FTreeItem> FindItemAt(const IntVector2& ClientPosition) const;

    /**
     * @brief Gets the rectangle an item's row covers, which is what a host overlays something on a row with.
     *
     * @param Item The item to measure.
     * @return Its row in client space, or an empty rectangle when the item has no row.
     */
    NODISCARD FRectangle GetItemRowBounds(const TSharedPtr<FTreeItem>& Item) const;

    /**
     * @brief Gets the part of an item's row its label occupies, which is the row less the indent, the
     * disclosure and the icon.
     *
     * @param Item The item to measure.
     * @return The label band in client space at full row height, or an empty rectangle when the item has no row.
     */
    NODISCARD FRectangle GetItemLabelBounds(const TSharedPtr<FTreeItem>& Item) const;

private:
    static constexpr int32 InvalidRowIndex = TArray<TSharedPtr<FTreeItem>>::InvalidIndex;

    static void SetSubtreeExpanded(const TArray<TSharedPtr<FTreeItem>>& Items, bool bExpanded);

    void RebuildVisibleRows() const;
    void AppendVisibleRows(const TArray<TSharedPtr<FTreeItem>>& Items) const;

    NODISCARD bool MatchesFilterText(const TSharedPtr<FTreeItem>& Item) const;
    NODISCARD bool PassesFilter(const TSharedPtr<FTreeItem>& Item) const;
    NODISCARD FRectangle ComputeRowBounds(const FRectangle& ViewBounds, int32 RowIndex) const;
    NODISCARD FRectangle ComputeDisclosureBounds(const FRectangle& RowBounds, int32 Depth) const;
    NODISCARD int32 ComputeRowExtent(const TSharedPtr<FTreeItem>& Item) const;
    NODISCARD int32 ComputeLabelStartX(const FRectangle& RowBounds, const TSharedPtr<FTreeItem>& Item) const;
    NODISCARD int32 FindRowAt(const IntVector2& ClientPosition) const;
    NODISCARD int32 FindRowIndex(const TSharedPtr<FTreeItem>& Item) const;
    NODISCARD int32 GetCurrentRowIndex() const;
    NODISCARD int32 GetMaxScrollOffset() const;

    void ApplySelectionFromClick(int32 RowIndex, const FModifierKeyState& Modifiers);
    void SelectSingleRow(int32 RowIndex);
    void MoveSelection(int32 Delta);
    void CollapseOrMoveToParent();
    void ExpandOrMoveToFirstChild();
    void ScrollRowIntoView(int32 RowIndex);

    TArray<TSharedPtr<FTreeItem>>         RootItems;
    TArray<TSharedPtr<FTreeItem>>         Selection;
    mutable TArray<TSharedPtr<FTreeItem>> VisibleRows;
    TSharedPtr<IFontFace>                 Font;
    String                                FilterText;
    IntVector2                            PressPosition;
    int32                                 RowHeight;
    int32                                 IndentPerLevel;
    int32                                 ScrollOffset;
    int32                                 ViewHeight;
    int32                                 AnchorRowIndex;
    int32                                 HoveredRowIndex;
    int32                                 PressedRowIndex;
    bool                                  bAllowMultiSelect;
    mutable bool                          bRowsDirty;
    FOnTreeSelectionChanged               OnSelectionChangedDelegate;
    FOnTreeItemActivated                  OnItemActivatedDelegate;
    FOnTreeItemExpansionChanged           OnExpansionChangedDelegate;
    FOnTreeItemDragDetected               OnDragDetectedDelegate;
};
