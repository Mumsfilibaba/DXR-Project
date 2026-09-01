#pragma once
#include "Application/Elements/Window.h"

struct APPLICATION_API Popups
{
    /**
     * @brief Opens a popup window holding an element and lays it out.
     *
     * @param ParentWindow  The window the popup belongs to, which keeps activation.
     * @param Bounds        Where to put it, in screen coordinates.
     * @param Content       The element to show.
     * @param bAcceptsInput False makes the popup transparent to the cursor, which a tool tip needs.
     * @return The window, or null when there is no application to create it in.
     */
    NODISCARD static TSharedPtr<FWindow> Open(const TSharedPtr<FWindow>& ParentWindow, const FRectangle& Bounds,
        const TSharedPtr<FVisualElement>& Content, bool bAcceptsInput = true);

    /**
     * @brief Closes a popup window.
     *
     * @param PopupWindow The window to close, which may be null.
     */
    static void Close(const TSharedPtr<FWindow>& PopupWindow);

    /**
     * @brief The work area of the monitor a point sits on.
     *
     * @param Point The point to look up, in screen coordinates.
     * @return The work area, falling back to the primary monitor when the point is on none of them.
     */
    NODISCARD static FRectangle FindWorkArea(const IntVector2& Point);

    /**
     * @brief Slides a rectangle back onto the monitor it is anchored to, without resizing it.
     *
     * @param Bounds The rectangle to place, in screen coordinates.
     * @return The rectangle, moved as little as it takes to fit.
     */
    NODISCARD static FRectangle ClampToWorkArea(const FRectangle& Bounds);
};
