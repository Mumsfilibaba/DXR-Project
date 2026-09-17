#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/Window.h"
#include "Application/Menus/MenuTypes.h"

typedef TSharedPtr<struct FMenuLayer> FMenuHandle;

struct FMenuLayer
{
    FMenuLayer()
        : HostWindow(nullptr)
        , MenuWindow(nullptr)
        , Content(nullptr)
        , ScreenBounds()
        , bIsInline(true)
    {
    }

    /** @brief The window the menu is drawn in, which owns the popup instead when there is one. */
    TSharedPtr<FWindow> HostWindow;

    /** @brief The popup the menu was given because it did not fit its host, and null while it is inline. */
    TSharedPtr<FWindow> MenuWindow;

    /** @brief The menu itself. */
    TSharedPtr<FVisualElement> Content;

    /** @brief Where the menu ended up, in screen coordinates. */
    FRectangle ScreenBounds;

    /** @brief Whether the menu is drawn inside its host rather than in a popup window. */
    bool bIsInline;
};

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
     * @brief Opens a menu, closing any sibling already open at that depth. The depth comes from the anchor:
     * an anchor that sits inside an open menu makes the new one its child and closes everything deeper, and
     * an anchor anywhere else closes the lot first. The menu is drawn inside its host window whenever it
     * fits there, which rounds its corners without needing a transparent surface, and only falls back to a
     * popup window when it does not.
     *
     * @param AnchorElement The element the menu is opened from, which decides both depth and host window.
     * @param AnchorBounds  The rectangle to place against, in screen coordinates.
     * @param Placement     Where to put the menu relative to the anchor.
     * @param MenuContent   The menu to show.
     * @return A handle to the open menu, or null when it could not be opened.
     */
    FMenuHandle PushMenu(
        const TSharedPtr<FVisualElement>& AnchorElement,
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
     * @brief Whether a menu is still open.
     *
     * @param Menu The menu to look for.
     * @return True when it is still open.
     */
    NODISCARD bool IsMenuOpen(const FMenuHandle& Menu) const;

    /**
     * @brief The depth a menu sits at, counting the outermost as one.
     *
     * @param Menu The menu to look for.
     * @return Its depth, or zero when it is not open.
     */
    NODISCARD int32 GetMenuDepth(const FMenuHandle& Menu) const;

    /**
     * @brief The depth of the deepest open menu an element sits inside, which is what a row dismisses to
     * before it opens a submenu of its own.
     *
     * @param Element The element to look up.
     * @return Its owning menu's depth, or zero when it is in no menu.
     */
    NODISCARD int32 GetOwningMenuDepth(const TSharedPtr<FVisualElement>& Element) const;

    /** @return The open menus, outermost first. */
    NODISCARD FORCEINLINE const TArray<FMenuHandle>& GetOpenMenus() const
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
     * is shown without taking activation, so keys arrive at the window that opened it, and a host offers
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

    NODISCARD FRectangle ResolveBounds(const FRectangle& AnchorBounds, const IntVector2& MenuSize, EMenuPlacement Placement,
        int32 ContentTopInset, const FRectangle& ClampArea) const;
    NODISCARD int32 FindMenuIndex(const FMenuHandle& Menu) const;

    // The deepest open menu the anchor sits inside, which is the menu the new one becomes a child of
    NODISCARD int32 FindParentIndex(const TSharedPtr<FVisualElement>& AnchorElement) const;

    void CloseLayer(const FMenuHandle& Menu);

    TArray<FMenuHandle> OpenMenus;
    FPendingSubMenu     PendingSubMenu;

    static TUniquePtr<FMenuStack> MenuStack;
};
