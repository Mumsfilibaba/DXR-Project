#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Text/IFontFace.h"

/** @brief Called when the item is chosen, after the menus have closed. */
DECLARE_DELEGATE(FOnMenuItemActivated);

class APPLICATION_API FMenuItem final : public FInteractiveElement
{
public:
    struct FDesc
    {
        FDesc()
            : Label()
            , ShortcutText()
            , Icon()
            , Font(nullptr)
            , CheckState(ECheckBoxState::Unchecked)
            , bIsCheckable(false)
            , SubMenu(nullptr)
            , OnActivated()
        {
        }

        /**
         * @brief Sets the text the row is named by.
         *
         * @param InLabel The label to show.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetLabel(const String& InLabel)
        {
            Label = InLabel;
            return *this;
        }

        /**
         * @brief Sets the face the row is measured and drawn with.
         *
         * @param InFont The face to use.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetFont(const TSharedPtr<IFontFace>& InFont)
        {
            Font = InFont;
            return *this;
        }

        /** @brief The text the row is named by. */
        String Label;

        /** @brief Right-aligned accelerator hint, drawn upper case and not bound. */
        String ShortcutText;

        /** @brief Drawn in the gutter, and only while no check mark is taking it. */
        FUIBrush Icon;

        /** @brief The face the row is measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief The state the check mark starts in, which only a checkable row draws. */
        ECheckBoxState CheckState;

        /** @brief True to draw the check mark in the gutter once the state leaves Unchecked. */
        bool bIsCheckable : 1;

        /** @brief Non-null draws the arrow and opens on hover rather than activating. */
        TSharedPtr<FVisualElement> SubMenu;

        /** @brief Fired when the item is chosen, after the menus have closed. */
        FOnMenuItemActivated OnActivated;
    };

public:

    /** @brief The square reserved on the left for a check mark or an icon, in pixels. */
    static constexpr int32 GutterWidth = 20;

    /** @brief The height every row takes unless its face is taller than that, in pixels. */
    static constexpr int32 RowHeight = 26;

    /** @brief The space held between the label and whatever is right-aligned beside it, in pixels. */
    static constexpr int32 ShortcutGap = 24;

    /** @brief The square reserved on the right for the submenu arrow, in pixels. */
    static constexpr int32 ArrowWidth = 14;

public:
    static TSharedPtr<FMenuItem> Create(const FDesc& Desc);

public:
    FMenuItem();
    virtual ~FMenuItem();

    /**
     * @brief Initializes the item with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Sets the check state, which only shows on an item that was made checkable.
     *
     * @param InCheckState The state to show.
     */
    void SetCheckState(ECheckBoxState InCheckState);

    /** @return The state the check mark shows, which is only drawn on an item that was made checkable. */
    NODISCARD FORCEINLINE ECheckBoxState GetCheckState() const
    {
        return CheckState;
    }

    /**
     * @brief Marks the row as the one keyboard navigation has landed on, which draws like a hover.
     *
     * @param bInIsHighlighted True to draw the row highlighted.
     */
    void SetHighlighted(bool bInIsHighlighted);

    /** @return True while keyboard navigation has landed on the row, which draws it highlighted. */
    NODISCARD FORCEINLINE bool IsHighlighted() const
    {
        return bIsHighlighted;
    }

    /** @brief Chooses the row as a click would, opening its submenu or firing its delegate. */
    void Activate();

    /**
     * @brief Sets what choosing the row does, replacing whatever it was created with.
     *
     * @param InOnActivated The delegate to run.
     */
    void SetOnActivated(const FOnMenuItemActivated& InOnActivated);

    /** @return The text the row is named by. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

    /** @return True when the item was given a submenu, so it opens one rather than activating. */
    NODISCARD FORCEINLINE bool HasSubMenu() const
    {
        return SubMenu != nullptr;
    }

    /** @return The submenu the item opens, or null when the item activates instead. */
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& GetSubMenu() const
    {
        return SubMenu;
    }

    /** @brief Opens the submenu at once, skipping the hover delay, which is what a click does. */
    void OpenSubMenu();

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    String                     Label;
    String                     ShortcutText;
    FUIBrush                   Icon;
    TSharedPtr<IFontFace>      Font;
    ECheckBoxState             CheckState;
    bool                       bIsCheckable;
    bool                       bIsHighlighted;
    TSharedPtr<FVisualElement> SubMenu;
    FOnMenuItemActivated       OnActivatedDelegate;
};

class APPLICATION_API FMenuSeparator final : public FVisualElement
{
public:

    /** @brief The inset the rule is held back from either edge by, in pixels. */
    static constexpr int32 InsetX = 20;

public:
    static TSharedPtr<FMenuSeparator> Create();

public:
    FMenuSeparator();
    virtual ~FMenuSeparator();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
};

class APPLICATION_API FMenuSectionHeader final : public FVisualElement
{
public:

    /** @brief The inset the caption starts at and the rule beside it ends at, in pixels. */
    static constexpr int32 InsetX = 20;

    /** @brief The space held between the caption and the rule beside it, in pixels. */
    static constexpr int32 LabelGap = 12;

public:

    /**
     * @brief Creates a header naming the group of rows below it.
     *
     * @param Label The caption, which is drawn upper case.
     * @param Font  The face the caption is measured and drawn with.
     * @return The new header.
     */
    NODISCARD static TSharedPtr<FMenuSectionHeader> Create(const String& Label, const TSharedPtr<IFontFace>& Font);

public:
    FMenuSectionHeader();
    virtual ~FMenuSectionHeader();

    /**
     * @brief Initializes the header with the caption it names its group by.
     *
     * @param InLabel The caption, which is folded to upper case here rather than on every draw.
     * @param InFont  The face the caption is measured and drawn with.
     */
    void Initialize(const String& InLabel, const TSharedPtr<IFontFace>& InFont);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /** @return The caption as it is drawn, which is upper case whatever the header was given. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

private:
    String                Label;
    TSharedPtr<IFontFace> Font;
};
