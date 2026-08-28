#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/Window.h"
#include "Application/Menus/MenuTypes.h"

class APPLICATION_API FMenuStack
{
public:

    /** @brief How long the cursor rests on an item before its submenu opens, in seconds. */
    static constexpr float DefaultSubMenuDelay = 0.25f;

    /**
     * @brief Gets the process-wide stack, which is what every anchor and item opens through.
     *
     * @return The stack, created on the first call.
     */
    NODISCARD static FMenuStack& Get();

    /** @brief Closes everything and drops the process-wide stack, which a host calls before the application goes. */
    static void Shutdown();

    /**
     * @brief The rectangle an element occupies on the desktop, which is what a menu is anchored to.
     *
     * @param Element The element to measure.
     * @return Its rectangle in screen coordinates, or an empty one when it belongs to no window.
     */
    NODISCARD static FRectangle GetScreenBounds(const TSharedPtr<FVisualElement>& Element);

public:
    FMenuStack();
    ~FMenuStack();

    FMenuStack(const FMenuStack&) = delete;
    FMenuStack& operator=(const FMenuStack&) = delete;

    /**
     * @brief Opens a menu in its own popup window, closing any sibling already open at that depth. The
     * depth comes from the parent: opening from a window that is itself an open menu makes the new one its
     * child and closes everything deeper, and opening from anything else closes the lot first.
     *
     * @param ParentWindow  The window the popup belongs to, which keeps activation.
     * @param AnchorBounds  The rectangle to place against, in screen coordinates.
     * @param Placement     Where to put the menu relative to the anchor.
     * @param MenuContent   The menu to show.
     * @return The popup window, so a caller can track its lifetime.
     */
    TSharedPtr<FWindow> PushMenu(
        const TSharedPtr<FWindow>&        ParentWindow,
        const FRectangle&                 AnchorBounds,
        EMenuPlacement                    Placement,
        const TSharedPtr<FVisualElement>& MenuContent);

    /** @brief Closes the deepest open menu. */
    void DismissTop();

    /** @brief Closes every open menu. */
    void DismissAll();

    /**
     * @brief Closes every menu below a depth, which is how hovering a sibling replaces a submenu.
     *
     * @param Depth The number of menus to leave open.
     */
    void DismissToDepth(int32 Depth);

    /**
     * @brief Closes every menu when a click landed outside all of them.
     *
     * @param ScreenPosition Where the click landed, in screen coordinates.
     * @return True when the click was consumed by dismissing.
     */
    bool DismissOnClickOutside(const IntVector2& ScreenPosition);

    /**
     * @brief Gets whether any menu is open, which a host uses to route Escape and arrows here first.
     *
     * @return True while at least one menu is open.
     */
    NODISCARD bool IsOpen() const;

    /** @return How many menus are open, the outermost counting as one, and zero when nothing is open. */
    NODISCARD int32 GetDepth() const;

    /**
     * @brief Whether a window is one of the open menus.
     *
     * @param MenuWindow The window to look for.
     * @return True when it is still open.
     */
    NODISCARD bool IsMenuOpen(const TSharedPtr<FWindow>& MenuWindow) const;

    /**
     * @brief The depth a menu window sits at, counting the outermost as one.
     *
     * @param MenuWindow The window to look for.
     * @return Its depth, or zero when it is not a menu.
     */
    NODISCARD int32 GetMenuDepth(const TSharedPtr<FWindow>& MenuWindow) const;

    /** @return The windows the open menus are shown in, outermost first. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FWindow>>& GetOpenMenus() const
    {
        return OpenMenus;
    }

    /**
     * @brief Arranges for a submenu to open once the cursor has rested on its item. Replaces whatever was
     * scheduled before, so moving down a column of items only ever has one submenu pending, which is the
     * one under the cursor now.
     *
     * @param Item           The item the cursor is resting on.
     * @param AnchorBounds   The rectangle to place the submenu against, in screen coordinates.
     * @param SubMenuContent The submenu to show.
     * @param DelaySeconds   How long the cursor has to rest before it opens.
     */
    void ScheduleSubMenu(
        const TSharedPtr<FVisualElement>& Item,
        const FRectangle&                 AnchorBounds,
        const TSharedPtr<FVisualElement>& SubMenuContent,
        float                             DelaySeconds = DefaultSubMenuDelay);

    /**
     * @brief Drops a scheduled submenu, which does nothing when a different item is pending.
     *
     * @param Item The item that was scheduled.
     */
    void CancelScheduledSubMenu(const TSharedPtr<FVisualElement>& Item);

    /** @return True while an item has a submenu waiting out its hover delay. */
    NODISCARD bool HasScheduledSubMenu() const;

    /**
     * @brief Routes a key to the deepest open menu, which is how menus are navigated without focus. A menu
     * window is shown without activation, so keys arrive at the window that opened it, and a host offers
     * them here first and only handles them itself when no menu wanted them.
     *
     * @param KeyEvent The key that went down.
     * @return True when a menu took the key.
     */
    bool HandleKeyDown(const FKeyEvent& KeyEvent);

    /**
     * @brief Advances the hover delay, opening whatever has waited long enough.
     *
     * @param DeltaSeconds Time since the last call.
     */
    void Tick(float DeltaSeconds);

private:
    struct FPendingSubMenu
    {
        FPendingSubMenu()
            : Item(nullptr)
            , Content(nullptr)
            , AnchorBounds()
            , RemainingSeconds(0.0f)
        {
        }

        TSharedPtr<FVisualElement> Item;
        TSharedPtr<FVisualElement> Content;
        FRectangle                 AnchorBounds;
        float                      RemainingSeconds;
    };

    NODISCARD FRectangle ResolveBounds(const FRectangle& AnchorBounds, const IntVector2& MenuSize, EMenuPlacement Placement) const;
    NODISCARD int32 FindMenuIndex(const TSharedPtr<FWindow>& MenuWindow) const;

    TArray<TSharedPtr<FWindow>> OpenMenus;
    FPendingSubMenu             PendingSubMenu;

    static TUniquePtr<FMenuStack> MenuStack;
};
