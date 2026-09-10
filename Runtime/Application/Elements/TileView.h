#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

struct FTileItem
{
    /** @brief The text drawn under the icon, elided when it is wider than the tile. */
    String Label;

    /** @brief The image drawn in the upper part of the tile, which is unset for an item without one. */
    FUIBrush Icon;

    /** @brief What the item stands for, which the host reads back once the selection changes. */
    void* UserData = nullptr;
};

/** @brief Called whenever the set of selected tiles changes, carrying the whole selection. */
DECLARE_DELEGATE(FOnTileSelectionChanged, const TArray<int32>& /*SelectedIndices*/);

/** @brief Called when a tile is double clicked, which is what opens the thing it stands for. */
DECLARE_DELEGATE(FOnTileActivated, int32 /*Index*/);

/** @brief Called once the cursor has moved far enough with a tile pressed to mean a drag rather than a click. */
DECLARE_DELEGATE(FOnTileDragDetected, int32 /*Index*/, const FCursorEvent& /*CursorEvent*/);

class APPLICATION_API FTileView final : public FVisualElement
{
public:

    /** @brief How far one wheel step scrolls, in pixels. */
    static constexpr int32 DefaultScrollAmountPerWheelStep = 40;

    /** @brief The index reported while the cursor is over no tile. */
    static constexpr int32 InvalidTileIndex = -1;

    /** @brief How far the cursor has to travel with the left button held on a tile before it means a drag, in pixels. */
    static constexpr int32 DragThreshold = 4;

public:
    struct FDesc
    {
        /** @brief The face the labels are drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief How large one tile is, in pixels, icon and label together. */
        IntVector2 TileSize = IntVector2(80, 92);

        /** @brief The gap left between two tiles, in pixels, on both axes. */
        int32 TileSpacing = 8;

        /** @brief The side of the icon square drawn in the upper part of a tile, in pixels. */
        int32 IconSize = 48;

        /** @brief How far the label band is inset from the tile's sides, in pixels, which is what the label elides to. */
        int32 LabelInset = 4;

        /** @brief How far a tile's corners are rounded, in pixels, or zero to leave them square. */
        float CornerRadius = 0.0f;

        /** @brief The fill of a tile that is neither hovered nor selected, transparent to leave the card unfilled. */
        FFloatColor IdleFill = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        /** @brief The fill of the tile under the cursor. */
        FFloatColor HoveredFill = FUIStyle::GetDefault().Colors.ControlHovered;

        /** @brief The fill of a selected tile, which wins over the hovered fill. */
        FFloatColor SelectedFill = FUIStyle::GetDefault().Colors.TextSelectionBackground;

        /** @brief True to let a chord or a shift click select more than one tile at a time. */
        bool bAllowMultiSelect = true;

        /** @brief Fired whenever the set of selected tiles changes. */
        FOnTileSelectionChanged OnSelectionChanged;

        /** @brief Fired when a tile is double clicked. */
        FOnTileActivated OnItemActivated;

        /** @brief Fired once a press on a tile has turned into a drag. */
        FOnTileDragDetected OnDragDetected;
    };

public:
    static TSharedPtr<FTileView> Create(const FDesc& Desc);

public:
    FTileView();
    virtual ~FTileView();

    /**
     * @brief Initializes the view with the specified parameters.
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
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseDoubleClick(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual bool SupportsKeyboardFocus() const override;

    /**
     * @brief Replaces every item, dropping the selection with them.
     *
     * @param InItems The items to show, laid out in the order they are given.
     */
    void SetItems(const TArray<FTileItem>& InItems);

    /** @return The items shown, in layout order. */
    NODISCARD FORCEINLINE const TArray<FTileItem>& GetItems() const
    {
        return Items;
    }

    /**
     * @brief Sets the run of a label to draw highlighted, which the host keeps in step with whatever
     * it filtered the tiles by.
     *
     * @param InFilter The text to match, compared without regard to case. An empty filter highlights nothing.
     */
    void SetFilterText(const String& InFilter);

    /** @return The run of a label drawn highlighted, which is empty when nothing is highlighted. */
    NODISCARD FORCEINLINE const String& GetFilterText() const
    {
        return FilterText;
    }

    /** @brief Drops the selection, firing the selection delegate when there was one to drop. */
    void ClearSelection();

    /**
     * @brief Selects one tile on its own, which is how a host lights the item it has just created.
     *
     * @param Index The tile to select, ignored when it is not one of the items shown.
     */
    void SetSelection(int32 Index);

    /** @return The selected indices, which is empty when nothing is selected. */
    NODISCARD FORCEINLINE const TArray<int32>& GetSelection() const
    {
        return SelectedIndices;
    }

    /**
     * @brief Whether a tile is one of the selected ones.
     *
     * @param Index The tile to check.
     * @return True while the tile is selected.
     */
    NODISCARD bool IsSelected(int32 Index) const;

    /**
     * @brief Sets how large one tile is, which is what a zoom slider on a content browser drives.
     *
     * @param InTileSize The new size in pixels, clamped to at least one pixel on each axis.
     */
    void SetTileSize(const IntVector2& InTileSize);

    /** @return How large one tile is, in pixels, icon and label together. */
    NODISCARD FORCEINLINE const IntVector2& GetTileSize() const
    {
        return TileSize;
    }

    /**
     * @brief Scrolls to an absolute offset, clamped into the scrollable range.
     *
     * @param InScrollOffset The offset from the top of the grid, in pixels.
     */
    void SetScrollOffset(int32 InScrollOffset);

    /** @return How far the grid is scrolled, as an offset from its top in pixels. */
    NODISCARD FORCEINLINE int32 GetScrollOffset() const
    {
        return ScrollOffset;
    }

    /**
     * @brief Gets the far end of the scrollable range, measured on the last arrange.
     *
     * @return How far the grid overhangs the view, in pixels, or zero when it fits.
     */
    NODISCARD int32 GetMaxScrollOffset() const;

    /**
     * @brief Scrolls the least it can to bring a tile fully into view, which is what an item selected from
     * outside the grid or one just added at its end needs.
     *
     * @param Index The tile to bring into view, ignored when it is not a tile.
     */
    void ScrollToTile(int32 Index);

    /** @return The tile under the cursor, or InvalidTileIndex when the cursor is over none. */
    NODISCARD FORCEINLINE int32 GetHoveredTile() const
    {
        return HoveredIndex;
    }

    /**
     * @brief Where a tile sits, which a rename field placed over its label and a drop highlight both need.
     *
     * @param Index The tile to measure.
     * @return The tile's bounds in client coordinates, scrolling included, or an empty rectangle for a bad index.
     */
    NODISCARD FRectangle GetTileBounds(int32 Index) const;

    /**
     * @brief Where a tile's label band sits, which is where a rename field goes.
     *
     * @param Index The tile to measure.
     * @return The band's bounds in client coordinates, or an empty rectangle for a bad index.
     */
    NODISCARD FRectangle GetTileLabelBounds(int32 Index) const;

    /**
     * @brief The tile at a point, which a drop handler needs to resolve what was dropped on.
     *
     * @param ClientPosition The point to test, in client coordinates.
     * @return The tile under the point, or InvalidTileIndex when the point is over none.
     */
    NODISCARD int32 FindTileAt(const IntVector2& ClientPosition) const;

private:
    NODISCARD int32 ResolveNumColumns(int32 AvailableWidth) const;
    NODISCARD int32 ComputeContentHeight(int32 AvailableWidth) const;
    NODISCARD FRectangle ComputeTileBounds(int32 Index, const FRectangle& Bounds) const;
    NODISCARD FRectangle ComputeIconBounds(const FRectangle& Tile) const;
    NODISCARD FRectangle ComputeLabelBounds(const FRectangle& Tile) const;

    void SelectTile(int32 Index, bool bToggle, bool bExtend);

    TArray<FTileItem>       Items;
    TArray<int32>           SelectedIndices;
    TSharedPtr<IFontFace>   Font;
    String                  FilterText;
    IntVector2              TileSize;
    IntVector2              PressPosition;
    int32                   TileSpacing;
    int32                   IconSize;
    int32                   LabelInset;
    float                   CornerRadius;
    FFloatColor             IdleFill;
    FFloatColor             HoveredFill;
    FFloatColor             SelectedFill;
    int32                   ScrollOffset;
    int32                   ContentHeight;
    int32                   ViewHeight;
    int32                   HoveredIndex;
    int32                   AnchorIndex;
    int32                   PressedIndex;
    bool                    bAllowMultiSelect;
    FOnTileSelectionChanged OnSelectionChangedDelegate;
    FOnTileActivated        OnItemActivatedDelegate;
    FOnTileDragDetected     OnDragDetectedDelegate;
};
