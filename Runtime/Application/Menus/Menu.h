#pragma once
#include "Application/Elements/Box.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Menus/MenuItem.h"

class APPLICATION_API FMenu final : public FCompoundElement
{
public:
    static TSharedPtr<FMenu> Create();

public:
    FMenu();
    virtual ~FMenu();

    /** @brief Puts the column inside the border, which cannot happen before the menu has a shared reference. */
    void Initialize();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;

    /**
     * @brief Appends a row to the column.
     *
     * @param Item The item to add.
     */
    void AddItem(const TSharedPtr<FMenuItem>& Item);

    /** @brief Appends a rule between two groups of rows. */
    void AddSeparator();

    /**
     * @brief Appends anything that is not a row, such as a search field at the head of the menu.
     *
     * @param Element The element to add.
     */
    void AddCustomEntry(const TSharedPtr<FVisualElement>& Element);

    /** @brief Drops every entry, so the menu can be rebuilt. */
    void ClearEntries();

    /** @return The rows of the menu in the order they were added, separators and custom entries excluded. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FMenuItem>>& GetItems() const
    {
        return Items;
    }

    /**
     * @brief Moves the highlight, skipping disabled rows and wrapping at either end.
     *
     * @param Delta How many rows to move by, negative to move up.
     */
    void MoveHighlight(int32 Delta);

    /**
     * @brief Highlights one row by index, or clears the highlight when the index is out of range.
     *
     * @param Index The row to highlight.
     */
    void SetHighlightedIndex(int32 Index);

    /** @return The index into the items of the highlighted row, or -1 when no row is highlighted. */
    NODISCARD FORCEINLINE int32 GetHighlightedIndex() const
    {
        return HighlightedIndex;
    }

    /** @brief Chooses the highlighted row, which does nothing when there is none. */
    void ActivateHighlighted();

    /**
     * @brief Sets the width every row is stretched to, which a combo box uses to match its button.
     *
     * @param InMinDesiredWidth The width in pixels, or zero to size to the widest row.
     */
    void SetMinDesiredWidth(int32 InMinDesiredWidth);

private:
    TSharedPtr<FVerticalBox>      Panel;
    TArray<TSharedPtr<FMenuItem>> Items;
    int32                         HighlightedIndex;
    int32                         MinDesiredWidth;
};
