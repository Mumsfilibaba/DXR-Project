#pragma once
#include "Core/Containers/String.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Menus/MenuAnchor.h"
#include "Application/Text/IFontFace.h"

class FMenuBar;

class APPLICATION_API FMenuBarButton final : public FInteractiveElement
{
public:
    static TSharedPtr<FMenuBarButton> Create(const String& InLabel, const TSharedPtr<IFontFace>& InFont);

public:
    FMenuBarButton();
    virtual ~FMenuBarButton();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Tells the button which bar and anchor it belongs to, which the bar does as it builds it.
     *
     * @param InOwnerBar The bar the button sits in.
     * @param InAnchor   The anchor holding the button, which owns the menu it opens.
     */
    void SetOwner(FMenuBar* InOwnerBar, FMenuAnchor* InAnchor);

    /**
     * @brief Replaces the look of the entry, which also resets its padding to the one the style carries.
     *
     * @param InStyle The look to draw with.
     */
    void SetStyle(const FUIMenuBarStyle& InStyle);

    /** @return The text the button is named by. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    String                Label;
    TSharedPtr<IFontFace> Font;
    FUIMenuBarStyle       Style;
    FMenuBar*             OwnerBar;
    FMenuAnchor*          Anchor;
};

class APPLICATION_API FMenuBar final : public FCompoundElement
{
public:
    static TSharedPtr<FMenuBar> Create();

public:
    FMenuBar();
    virtual ~FMenuBar();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;

    /** @brief Puts the row of buttons in place, which cannot happen before the bar has a shared reference. */
    void Initialize();

    /**
     * @brief Appends a titled drop-down to the right of the last one.
     *
     * @param Label       The title the button shows.
     * @param Font        The face the title is drawn with.
     * @param MenuContent The menu the button opens.
     * @return The anchor, so a caller can open or close the menu itself.
     */
    TSharedPtr<FMenuAnchor> AddMenu(const String& Label, const TSharedPtr<IFontFace>& Font, const TSharedPtr<FVisualElement>& MenuContent);

    /** @brief Closes whichever drop-down is open. */
    void CloseActiveMenu();

    /** @return True when any anchor in the bar has its drop-down open. */
    NODISCARD bool IsAnyMenuOpen() const;

    /** @return The anchors the buttons are held by, in the order they were added. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FMenuAnchor>>& GetAnchors() const
    {
        return Anchors;
    }

    /**
     * @brief Switches to a hovered button's menu, which only happens while a sibling is already open.
     *
     * @param HoveredAnchor The anchor the cursor moved onto.
     */
    void OnButtonHovered(FMenuAnchor* HoveredAnchor);

    /**
     * @brief Replaces the look of the strip and of the entries already on it, which every entry added
     * afterwards takes too.
     *
     * @param InStyle The look to draw with.
     */
    void SetStyle(const FUIMenuBarStyle& InStyle);

    /** @return The look the strip and its entries draw themselves with. */
    NODISCARD FORCEINLINE const FUIMenuBarStyle& GetStyle() const
    {
        return Style;
    }

private:
    TSharedPtr<FHorizontalBox>         Panel;
    TArray<TSharedPtr<FMenuAnchor>>    Anchors;
    TArray<TSharedPtr<FMenuBarButton>> Buttons;
    FUIMenuBarStyle                    Style;
};
