#pragma once
#include "Core/Templates/Utility/EnumOperators.h"
#include "Application/Layout/LayoutTypes.h"

enum class EPopupSide : uint8
{
    /** @brief Under the anchor, left edges flush. A point anchor opens the popup down and to the right of it. */
    Below,

    /** @brief Beside the anchor, top edges flush. */
    Right,
};

enum class EPopupFlip : uint8
{
    None = 0,

    /** @brief Across the side it opens on: above instead of below, or left instead of right. */
    Main = FLAG(0),

    /** @brief Along that side: right edges flush instead of left, or bottom edges flush instead of top. */
    Cross = FLAG(1),

    Both = Main | Cross,
};

ENUM_CLASS_OPERATORS(EPopupFlip);

struct APPLICATION_API FPopupPlacement
{
    /**
     * @brief Places a popup against its anchor. Each allowed flip is taken only where the popup would overflow the
     * area and the other side has room, and whatever still overflows is clamped in.
     *
     * @param Anchor      What the popup opens against, which may be a point.
     * @param Size        The popup's size.
     * @param Area        What it has to stay inside.
     * @param Side        Which side of the anchor it opens on.
     * @param Flips       Which ways it may flip.
     * @param Gap         The space kept between the anchor and the popup, across the side it opens on.
     * @param CrossOffset How far the popup is shifted along that side, as a submenu lifts its first row level.
     * @return The popup's bounds.
     */
    NODISCARD static FRectangle Resolve(
        const FRectangle& Anchor,
        const IntVector2& Size,
        const FRectangle& Area,
        EPopupSide        Side,
        EPopupFlip        Flips,
        int32             Gap         = 0,
        int32             CrossOffset = 0);

    /**
     * @brief Moves a rectangle the least distance that puts it inside an area, as far as it fits.
     *
     * @return The moved rectangle, unchanged in size. One too big for the area keeps its top-left inside it.
     */
    NODISCARD static FRectangle ClampIntoArea(const FRectangle& Bounds, const FRectangle& Area);
};
