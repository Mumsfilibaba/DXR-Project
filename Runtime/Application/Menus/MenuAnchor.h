#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Menus/MenuTypes.h"

/** @brief Builds the menu the moment it is needed, for a menu whose contents depend on the state when it opens. */
DECLARE_RETURN_DELEGATE(FOnGetMenuContent, TSharedPtr<FVisualElement>);

/** @brief Called when the anchor opens or closes its menu. */
DECLARE_DELEGATE(FOnMenuOpenChanged, bool /*bIsOpen*/);

class APPLICATION_API FMenuAnchor final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : Content(nullptr)
            , MenuContent(nullptr)
            , Placement(EMenuPlacement::BelowLeftAligned)
            , AnchorInset()
            , OnGetMenuContent()
            , OnOpenChanged()
        {
        }

        /**
         * @brief Sets the element the menu is anchored to, which is usually the button that opens it.
         *
         * @param InContent The element to wrap.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetContent(const TSharedPtr<FVisualElement>& InContent)
        {
            Content = InContent;
            return *this;
        }

        /**
         * @brief Sets the menu to open, for a menu whose contents do not change.
         *
         * @param InMenuContent The menu to show.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetMenuContent(const TSharedPtr<FVisualElement>& InMenuContent)
        {
            MenuContent = InMenuContent;
            return *this;
        }

        /** @brief The element the menu is anchored to, which is usually the button that opens it. */
        TSharedPtr<FVisualElement> Content;

        /** @brief The menu to open, for a menu whose contents do not change. */
        TSharedPtr<FVisualElement> MenuContent;

        /** @brief Where the menu is placed relative to the anchor. */
        EMenuPlacement Placement;

        /** @brief How far the anchor's own fill is held inside its bounds, so the menu meets what is drawn. */
        FMargin AnchorInset;

        /** @brief Consulted before every open, and takes precedence over MenuContent. */
        FOnGetMenuContent OnGetMenuContent;

        /** @brief Fired when the anchor opens or closes its menu. */
        FOnMenuOpenChanged OnOpenChanged;
    };

public:
    static TSharedPtr<FMenuAnchor> Create(const FDesc& Desc);

public:
    FMenuAnchor();
    virtual ~FMenuAnchor();

    /**
     * @brief Initializes the anchor with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    /** @brief Opens the menu under the anchor, replacing whatever the stack had open at that depth. */
    void Open();

    /** @brief Closes the menu, and everything it opened, when this anchor is what opened it. */
    void Close();

    /** @brief Opens a closed menu and closes an open one, which is what a click on the anchor does. */
    void Toggle();

    /**
     * @brief Gets whether this anchor's menu is open, noticing first that the stack may have closed it.
     *
     * @return True while the menu is one of the open menus.
     */
    NODISCARD bool IsOpen() const;

    /**
     * @brief Sets where the menu is placed relative to the anchor.
     *
     * @param InPlacement The placement to use.
     */
    void SetPlacement(EMenuPlacement InPlacement);

    /**
     * @brief Replaces the menu opened next, which does not disturb one already open.
     *
     * @param InMenuContent The menu to show.
     */
    void SetMenuContent(const TSharedPtr<FVisualElement>& InMenuContent);

    /**
     * @brief Sets how far the anchor's own fill is held inside its bounds, so the menu meets what is drawn.
     *
     * @param InAnchorInset The inset to deflate the anchor's bounds by before the menu is placed.
     */
    void SetAnchorInset(const FMargin& InAnchorInset);

    /** @return How far the anchor's bounds are deflated before the menu is placed against them. */
    NODISCARD FORCEINLINE const FMargin& GetAnchorInset() const
    {
        return AnchorInset;
    }

    /**
     * @brief Gets the menu this anchor opened, noticing first that the stack may have closed it.
     *
     * @return The menu, or null while it is closed.
     */
    NODISCARD FMenuHandle GetMenu() const;

private:
    void SyncOpenState() const;

    TSharedPtr<FVisualElement> MenuContent;
    EMenuPlacement             Placement;
    FMargin                    AnchorInset;
    FOnGetMenuContent          OnGetMenuContentDelegate;
    FOnMenuOpenChanged         OnOpenChangedDelegate;
    mutable FMenuHandle        Menu;
};
