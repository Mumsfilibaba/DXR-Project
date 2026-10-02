#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Menus/MenuAnchor.h"
#include "Application/Text/IFontFace.h"

class FToolBar;

enum class EToolBarItemType : uint8
{
    /** @brief Fires once per click and holds no state. */
    Button,

    /** @brief Latches, and draws lit for as long as it is checked. */
    Toggle,

    /** @brief Opens a menu, and draws lit for as long as that menu is open. */
    DropDown,

    /** @brief A rule between groups, which is not interactive. */
    Separator,

    /** @brief An element the caller built, which the bar only places. */
    Custom,

    /** @brief Empty space that swallows whatever width the entries leave, which is what pushes a group off the left edge. */
    FlexibleSpace,
};

struct FToolBarItemDesc
{
    FToolBarItemDesc()
        : Icon()
        , Label()
        , ToolTipText()
        , MinWidth(0)
    {
    }

    /**
     * @brief Sets the icon drawn ahead of the label.
     *
     * @param InIcon The brush to draw.
     * @return This desc, so the setters can be chained.
     */
    FORCEINLINE FToolBarItemDesc& SetIcon(const FUIBrush& InIcon)
    {
        Icon = InIcon;
        return *this;
    }

    /**
     * @brief Sets the text drawn beside the icon.
     *
     * @param InLabel The text to show.
     * @return This desc, so the setters can be chained.
     */
    FORCEINLINE FToolBarItemDesc& SetLabel(const String& InLabel)
    {
        Label = InLabel;
        return *this;
    }

    /**
     * @brief Sets the tip shown once the cursor has rested on the entry.
     *
     * @param InToolTipText The text to show, which is empty for an entry with no tip.
     * @return This desc, so the setters can be chained.
     */
    FORCEINLINE FToolBarItemDesc& SetToolTipText(const String& InToolTipText)
    {
        ToolTipText = InToolTipText;
        return *this;
    }

    /**
     * @brief Sets a width the entry will not shrink below, which is what makes a row of them line up.
     *
     * @param InMinWidth The width in pixels, or zero to size the entry to its content.
     * @return This desc, so the setters can be chained.
     */
    FORCEINLINE FToolBarItemDesc& SetMinWidth(int32 InMinWidth)
    {
        MinWidth = InMinWidth;
        return *this;
    }

    /** @brief The icon drawn ahead of the label, which is invalid for a text-only entry. */
    FUIBrush Icon;

    /** @brief The text drawn beside the icon, which is empty for an icon-only entry. */
    String Label;

    /** @brief The tip shown once the cursor has rested, which is empty for an entry with no tip. */
    String ToolTipText;

    /** @brief A width the entry will not shrink below, or zero to size it to its content. */
    int32 MinWidth;
};

class APPLICATION_API FToolBarButton final : public FInteractiveElement
{
public:

    /**
     * @brief Creates an entry of the specified type.
     *
     * @param Item     What the entry shows.
     * @param InType   Which of the three kinds of entry it is.
     * @param InFont   The face the label is measured and drawn with.
     * @param InIconSize The edge length of the icon square, in pixels.
     * @return The new entry.
     */
    static TSharedPtr<FToolBarButton> Create(const FToolBarItemDesc& Item, EToolBarItemType InType, const TSharedPtr<IFontFace>& InFont, int32 InIconSize);

public:
    FToolBarButton();
    virtual ~FToolBarButton();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Tells a dropdown entry which bar and anchor it belongs to, which the bar does as it builds it.
     *
     * @param InOwnerBar The bar the entry sits in.
     * @param InAnchor   The anchor holding the entry, which owns the menu it opens.
     */
    void SetOwner(FToolBar* InOwnerBar, FMenuAnchor* InAnchor);

    /**
     * @brief Sets the latched state without firing the delegate, which is how a host pushes a model value in.
     *
     * @param InState The state to move to.
     */
    void SetCheckState(ECheckBoxState InState);

    /**
     * @brief Sets the text the entry shows, which is how a transport control switches between Play and Stop.
     *
     * @param InLabel The text to show, empty for an icon-only entry.
     */
    void SetLabel(const String& InLabel);

    /**
     * @brief Sets the icon drawn ahead of the label, empty for a text-only entry.
     *
     * @param InIcon The brush to draw.
     */
    void SetIcon(const FUIBrush& InIcon);

    /**
     * @brief Sets the tip shown once the cursor has rested on the entry.
     *
     * @param InToolTipText The text to show, which is empty for an entry with no tip.
     */
    void SetToolTipText(const String& InToolTipText);

    /**
     * @brief Sets what a click on the entry does.
     *
     * @param InOnClicked The delegate to fire.
     */
    void SetOnClicked(const FOnClicked& InOnClicked);

    /**
     * @brief Sets what a change of the latched state reports to.
     *
     * @param InOnStateChanged The delegate to fire.
     */
    void SetOnStateChanged(const FOnCheckStateChanged& InOnStateChanged);

    /**
     * @brief Lights a plain button from outside, which is how a set of them reads as a radio group without latching.
     *
     * @param bInIsHighlighted True to draw the entry lit.
     */
    void SetHighlighted(bool bInIsHighlighted);

    /**
     * @brief Sets how far each of the entry's corners is rounded, which is what fuses a group into one pill.
     *
     * @param InCornerRadius The four radii, in pixels.
     */
    void SetCornerRadius(const FCornerRadii& InCornerRadius);

    /**
     * @brief Sets a width the entry will not shrink below.
     *
     * @param InMinWidth The width in pixels, or zero to size the entry to its content.
     */
    void SetMinWidth(int32 InMinWidth);

    /** @return The four radii of the entry's fill, in pixels. */
    NODISCARD FORCEINLINE const FCornerRadii& GetCornerRadius() const
    {
        return CornerRadius;
    }

    /** @return The width the entry will not shrink below, or zero when it sizes to its content. */
    NODISCARD FORCEINLINE int32 GetMinWidth() const
    {
        return MinWidth;
    }

    /** @return The latched state of the entry, which only a toggle moves as it is clicked. */
    NODISCARD FORCEINLINE ECheckBoxState GetCheckState() const
    {
        return CheckState;
    }

    /** @return True for the Checked state only, so Undetermined reads as unchecked. */
    NODISCARD FORCEINLINE bool IsChecked() const
    {
        return CheckState == ECheckBoxState::Checked;
    }

    /** @return The kind of entry it was created as, which decides how it draws and what a click does. */
    NODISCARD FORCEINLINE EToolBarItemType GetItemType() const
    {
        return ItemType;
    }

    /** @return The text the entry shows, which is empty for an icon-only entry. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

    /** @return The tip shown once the cursor has rested, which is empty for an entry with no tip. */
    NODISCARD FORCEINLINE const String& GetToolTipText() const
    {
        return ToolTipText;
    }

    /**
     * @return True for a dropdown whose menu is open, a toggle in any state but Unchecked, and a plain button
     * SetHighlighted has lit.
     */
    NODISCARD bool IsHighlighted() const;

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    NODISCARD int32 ComputeContentWidth() const;
    NODISCARD FRectangle GetIconBounds(const FRectangle& Bounds, int32 LeadingOffset) const;

    void DrawArrow(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId, const FFloatColor& Tint) const;

    FUIBrush              Icon;
    String                Label;
    String                ToolTipText;
    TSharedPtr<IFontFace> Font;
    EToolBarItemType      ItemType;
    ECheckBoxState        CheckState;
    FCornerRadii          CornerRadius;
    int32                 IconSize;
    int32                 MinWidth;
    bool                  bIsHighlighted;
    FOnClicked            OnClickedDelegate;
    FOnCheckStateChanged  OnStateChangedDelegate;
    FToolBar*             OwnerBar;
    FMenuAnchor*          Anchor;
};

struct FToolBarEntry
{
    FToolBarEntry()
        : Element(nullptr)
        , Button(nullptr)
        , Type(EToolBarItemType::Custom)
    {
    }

    /** @brief The element occupying the slot, which is the menu anchor for a dropdown. */
    TSharedPtr<FVisualElement> Element;

    /** @brief The entry itself, which is null for a rule and for a custom element. */
    TSharedPtr<FToolBarButton> Button;

    /** @brief Which of the kinds of entry this is. */
    EToolBarItemType Type;
};

class APPLICATION_API FToolBar final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : Font(nullptr)
            , Orientation(EOrientation::Horizontal)
            , IconSize(16)
            , Padding(FMargin(4, 2, 4, 2))
            , ItemSpacing(2)
            , bHasBackground(true)
        {
        }

        /**
         * @brief Sets the face every label is measured and drawn with.
         *
         * @param InFont The face to use.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetFont(const TSharedPtr<IFontFace>& InFont)
        {
            Font = InFont;
            return *this;
        }

        /**
         * @brief Sets whether the strip runs across or down.
         *
         * @param InOrientation The direction entries are stacked in.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetOrientation(EOrientation InOrientation)
        {
            Orientation = InOrientation;
            return *this;
        }

        /** @brief The face every entry's label is drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief Whether the strip runs across or down, which also decides where a dropdown opens. */
        EOrientation Orientation;

        /** @brief The edge length of an icon square, in pixels. */
        int32 IconSize;

        /** @brief The space between the strip's bounds and its entries. */
        FMargin Padding;

        /** @brief The gap between one entry and the next, in pixels. */
        int32 ItemSpacing;

        /** @brief True to fill the strip with the docked panel's surface, so it reads flush with the tab strip above it. */
        bool bHasBackground : 1;
    };

public:
    static TSharedPtr<FToolBar> Create(const FDesc& Desc);

public:
    FToolBar();
    virtual ~FToolBar();

    /**
     * @brief Initializes the bar with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Appends an entry that fires once per click.
     *
     * @param Item      What the entry shows.
     * @param OnClicked What the click does.
     * @return The entry, so a caller can enable or disable it later.
     */
    TSharedPtr<FToolBarButton> AddButton(const FToolBarItemDesc& Item, const FOnClicked& OnClicked);

    /**
     * @brief Appends an entry that holds its state, and draws lit while it is checked.
     *
     * @param Item           What the entry shows.
     * @param InitialState   The state the entry starts in.
     * @param OnStateChanged What a change of the state reports to.
     * @return The entry, so a caller can read or push the state later.
     */
    TSharedPtr<FToolBarButton> AddToggle(const FToolBarItemDesc& Item, ECheckBoxState InitialState, const FOnCheckStateChanged& OnStateChanged);

    /**
     * @brief Appends an entry that opens a menu, and draws lit while that menu is open.
     *
     * @param Item        What the entry shows.
     * @param MenuContent The menu the entry opens.
     * @return The anchor, so a caller can open or close the menu itself.
     */
    TSharedPtr<FMenuAnchor> AddDropDown(const FToolBarItemDesc& Item, const TSharedPtr<FVisualElement>& MenuContent);

    /**
     * @brief Appends an entry that rebuilds its menu each time it opens.
     *
     * @param Item            What the entry shows.
     * @param OnGetMenuContent Builds the menu when the entry is opened.
     * @return The anchor, so a caller can open or close the menu itself.
     */
    TSharedPtr<FMenuAnchor> AddDropDown(const FToolBarItemDesc& Item, const FOnGetMenuContent& OnGetMenuContent);

    /**
     * @brief Opens a run of entries that are fused into one pill.
     *
     * Entries appended until EndGroup sit flush against each other, and EndGroup rounds only the two outer ends, so
     * the run reads as a single segmented control rather than as separate buttons. Groups do not nest.
     */
    void BeginGroup();

    /** @brief Closes the run BeginGroup opened and rounds its outer ends. */
    void EndGroup();

    /** @brief Appends a rule between groups, which runs across the strip. */
    void AddSeparator();

    /**
     * @brief Appends empty space that grows to take whatever the sized entries leave over.
     *
     * Two of these around a group centre it, and one before a group pushes that group to the far edge. The strip has
     * to be stretched rather than sized to its content for there to be anything left over to take.
     */
    void AddFlexibleSpace();

    /**
     * @brief Appends an element the caller built, for the combo box or search field a strip sometimes carries.
     *
     * @param Widget       The element to place.
     * @param FillCoefficient How much of the leftover width to take, or zero to size the element to its content.
     */
    void AddWidget(const TSharedPtr<FVisualElement>& Widget, float FillCoefficient = 0.0f);

    /** @brief Drops every entry, so the bar can be refilled. */
    void ClearItems();

    /** @brief Closes whichever dropdown is open. */
    void CloseActiveMenu();

    /**
     * @brief Gets whether one of the dropdowns is open, which is what makes hovering a sibling switch menus.
     *
     * @return True while any of the bar's anchors is open.
     */
    NODISCARD bool IsAnyMenuOpen() const;

    /**
     * @brief Switches to a hovered entry's menu, which only happens while a sibling is already open.
     *
     * @param HoveredAnchor The anchor the cursor moved onto.
     */
    void OnButtonHovered(FMenuAnchor* HoveredAnchor);

    /** @return The bar's entries, in the order they were added, rules and custom elements included. */
    NODISCARD FORCEINLINE const TArray<FToolBarEntry>& GetItems() const
    {
        return Items;
    }

    /** @return How many entries the bar holds, rules and custom elements included. */
    NODISCARD FORCEINLINE int32 GetNumItems() const
    {
        return Items.Size();
    }

    /**
     * @brief The entry at an index, or null when that index holds a rule or a custom element.
     *
     * @param Index The position in the bar.
     * @return The entry, or null.
     */
    NODISCARD TSharedPtr<FToolBarButton> GetButton(int32 Index) const;

    /**
     * @brief The first entry with the specified label, which is how a test or a host reaches one by name.
     *
     * @param InLabel The label to look for.
     * @return The entry, or null when the bar has none by that name.
     */
    NODISCARD TSharedPtr<FToolBarButton> FindButton(const String& InLabel) const;

    /** @return The anchors the dropdowns are wrapped in, in the order they were added. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FMenuAnchor>>& GetAnchors() const
    {
        return Anchors;
    }

private:
    FBoxSlot& AppendSlot(const TSharedPtr<FVisualElement>& Element, const TSharedPtr<FToolBarButton>& Button, EToolBarItemType Type);

    TSharedPtr<FBox>                Panel;
    TArray<FToolBarEntry>           Items;
    TArray<TSharedPtr<FMenuAnchor>> Anchors;
    TSharedPtr<IFontFace>           Font;
    EOrientation                    Orientation;
    int32                           IconSize;
    int32                           ItemSpacing;
    int32                           GroupStartIndex;
    bool                            bHasBackground;
};
