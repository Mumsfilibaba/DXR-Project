#pragma once
#include "Core/Containers/SharedPtr.h"

class FWindowElement;
class FDrawCommandList;

struct IApplicationRenderer
{
    /** @brief Virtual destructor for the IApplicationRenderer interface. */
    virtual ~IApplicationRenderer() = default;

    /**
     * @brief Opens the command list a window records into. The list belongs to the renderer, which keeps
     * it until the frame it was recorded for has been submitted, so nothing may hold on to it past the
     * matching EndWindow.
     *
     * @param InWindow The window about to be drawn.
     * @return The list to record into, or null when the renderer does not draw this window.
     */
    virtual FDrawCommandList* BeginWindow(const TSharedPtr<FWindowElement>& InWindow) = 0;

    /**
     * @brief Closes the list opened by the matching BeginWindow.
     *
     * @param InWindow The window that finished recording.
     */
    virtual void EndWindow(const TSharedPtr<FWindowElement>& InWindow) = 0;

    /**
     * @brief Releases whatever the renderer holds for a window that is going away.
     *
     * @param InWindow The window being destroyed.
     */
    virtual void OnWindowDestroyed(const TSharedPtr<FWindowElement>& InWindow) = 0;
};
