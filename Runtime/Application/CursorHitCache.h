#pragma once
#include "Application/ElementPath.h"

class FWindow;

class APPLICATION_API FCursorHitCache
{
public:
    FCursorHitCache();
    ~FCursorHitCache();

    /**
     * @brief Gets the path under a point in a window, hit testing only when the cached path is out of date.
     *
     * @param InWindow         The window under the cursor, which may be null for a point over no window.
     * @param InClientPosition The point, in the window's client space.
     * @return The path from the window down, which stays valid until the next call or Invalidate.
     */
    const FElementPath& Resolve(const TSharedPtr<FWindow>& InWindow, const IntVector2& InClientPosition);

    /**
     * @brief Forgets the cached path and lets go of the elements on it, so the next Resolve walks the tree again
     * and nothing is kept alive by the cache in the meantime.
     */
    void Invalidate();

private:
    FElementPath      Path;
    TWeakPtr<FWindow> Window;
    IntVector2        ClientPosition;
    uint64            Generation;
    bool              bIsValid;
};
