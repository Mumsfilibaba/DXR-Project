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
    virtual void SetOuterCornerRadius(float InCornerRadius) override;

    /**
     * @brief Appends a row to the column.
     *
     * @param Item The item to add.
     */
    void AddItem(const TSharedPtr<FMenuItem>& Item);

    /** @brief Appends a rule between two groups of rows. */
    void AddSeparator();

    /**
     * @brief Appends a caption naming the group of rows below it, drawn upper case with a rule beside it.
     *
     * @param Label The caption.
     * @param Font  The face the caption is measured and drawn with.
     */
    void AddSection(const String& Label, const TSharedPtr<IFontFace>& Font);

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
     * @brief Sets the width every row is stretched to, which a combo box uses to match its button and is
     * the only way to come out narrower than the style's floor, which a menu that never calls this takes.
     *
     * @param InMinDesiredWidth The width in pixels, or zero to size to the widest row.
     */
    void SetMinDesiredWidth(int32 InMinDesiredWidth);

    /**
     * @brief Replaces the look of the menu, of every entry already in it and of every entry appended
     * afterwards, which a combo box uses so its list reads as part of the field rather than as one of the
     * application menus. A caller dressing one row differently has to do that after the AddItem that hands
     * it this style.
     *
     * @param InStyle The look to draw with.
     */
    void SetStyle(const FUIMenuStyle& InStyle);

    /** @return The look the menu and its entries draw themselves with. */
    NODISCARD FORCEINLINE const FUIMenuStyle& GetStyle() const
    {
        return Style;
    }

private:
    TSharedPtr<FVerticalBox>               Panel;
    TArray<TSharedPtr<FMenuItem>>          Items;
    TArray<TSharedPtr<FMenuSeparator>>     Separators;
    TArray<TSharedPtr<FMenuSectionHeader>> Sections;
    FUIMenuStyle                           Style;
    int32                                  HighlightedIndex;
    int32                                  MinDesiredWidth;
};
