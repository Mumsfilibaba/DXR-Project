#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
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

class APPLICATION_API FTileView final : public FVisualElement
{
public:

    /** @brief How far one wheel step scrolls, in pixels. */
    static constexpr int32 DefaultScrollAmountPerWheelStep = 40;

    /** @brief The index reported while the cursor is over no tile. */
    static constexpr int32 InvalidTileIndex = -1;

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

        /** @brief True to let a chord or a shift click select more than one tile at a time. */
        bool bAllowMultiSelect = true;

        /** @brief Fired whenever the set of selected tiles changes. */
        FOnTileSelectionChanged OnSelectionChanged;

        /** @brief Fired when a tile is double clicked. */
        FOnTileActivated OnItemActivated;
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

    /** @brief Drops the selection, firing the selection delegate when there was one to drop. */
    void ClearSelection();

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

    /** @return The tile under the cursor, or InvalidTileIndex when the cursor is over none. */
    NODISCARD FORCEINLINE int32 GetHoveredTile() const
    {
        return HoveredIndex;
    }

private:
    NODISCARD int32 ResolveNumColumns(int32 AvailableWidth) const;
    NODISCARD int32 ComputeContentHeight(int32 AvailableWidth) const;
    NODISCARD FRectangle ComputeTileBounds(int32 Index, const FRectangle& Bounds) const;
    NODISCARD int32 FindTileAt(const IntVector2& ClientPosition) const;
    NODISCARD String ElideLabel(const String& InLabel, int32 MaxWidth) const;

    void SelectTile(int32 Index, bool bToggle, bool bExtend);

    TArray<FTileItem>       Items;
    TArray<int32>           SelectedIndices;
    TSharedPtr<IFontFace>   Font;
    IntVector2              TileSize;
    int32                   TileSpacing;
    int32                   IconSize;
    int32                   ScrollOffset;
    int32                   ContentHeight;
    int32                   ViewHeight;
    int32                   HoveredIndex;
    int32                   AnchorIndex;
    bool                    bAllowMultiSelect;
    FOnTileSelectionChanged OnSelectionChangedDelegate;
    FOnTileActivated        OnItemActivatedDelegate;
};
