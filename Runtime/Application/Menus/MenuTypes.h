#pragma once
#include "Application/Layout/LayoutTypes.h"

enum class EMenuPlacement : uint8
{
    /** @brief Under the anchor, left edges flush. A menu bar drop-down. */
    BelowLeftAligned,

    /** @brief Beside the anchor, its first row level with it. A submenu. */
    RightOfTopAligned,

    /** @brief At a screen point. A context menu, or a tooltip following the cursor. */
    AtCursor,
};
