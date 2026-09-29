#pragma once
#include "Core/Math/Color.h"
#include "Application/Layout/LayoutTypes.h"

class FDrawCommandList;

struct APPLICATION_API FDisclosureGlyph
{
    /**
     * @brief Draws the triangle filling a box.
     *
     * @param OutCommandList The list to record into.
     * @param LayerId        The layer to draw on.
     * @param Bounds         The box the triangle fills.
     * @param bIsExpanded    True to point down, false to point right.
     * @param Tint           The fill colour.
     */
    static void Draw(FDrawCommandList& OutCommandList, int32 LayerId, const FRectangle& Bounds, bool bIsExpanded, const FFloatColor& Tint);
};
