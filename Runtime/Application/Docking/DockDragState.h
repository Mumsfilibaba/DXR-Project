#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Docking/DockNode.h"

class FDockingArea;

class APPLICATION_API FDockDragState
{
public:

    /**
     * @brief Gets the process-wide drag, creating it on first use.
     *
     * @return The service, which lives until Shutdown takes it down.
     */
    NODISCARD static FDockDragState& Get();

    /**
     * @brief Gets whether the service exists, so a teardown path can avoid resurrecting it.
     *
     * @return True once it has been created and before Shutdown takes it down.
     */
    NODISCARD static bool IsInitialized();

    /** @brief Ends any drag in flight and takes the service down. */
    static void Shutdown();

public:
    FDockDragState();
    ~FDockDragState();

    /**
     * @brief Starts a drag, which is what a torn-out tab does instead of docking straight away.
     *
     * @param PanelId        The panel being dragged.
     * @param SourceArea     The area it came from, so a drop back into it knows what to undock.
     * @param ScreenPosition Where the cursor was, in screen coordinates.
     */
    void BeginDrag(const String& PanelId, FDockingArea* SourceArea, const IntVector2& ScreenPosition);

    /**
     * @brief Moves the drag, re-resolving which area and which edge is under the cursor.
     *
     * @param ScreenPosition Where the cursor is now, in screen coordinates.
     */
    void UpdateDrag(const IntVector2& ScreenPosition);

    /**
     * @brief Ends the drag, docking into the last valid target and leaving the panel alone when there is
     * none. A drop into an area other than the one it came from carries the panel's registration across,
     * because an area can only dock an id it knows about.
     */
    void EndDrag();

    /** @brief Ends the drag without docking anything, which is what Escape does. */
    void CancelDrag();

    /**
     * @brief Registers an area as a possible drop target, which every area does when it is created.
     *
     * @param Area The area to register.
     */
    void RegisterArea(FDockingArea* Area);

    /**
     * @brief Forgets an area, which every area does when it is destroyed.
     *
     * @param Area The area to forget.
     */
    void UnregisterArea(FDockingArea* Area);

    /** @return The panel being dragged, or an empty string when no drag is in flight. */
    NODISCARD FORCEINLINE const String& GetDraggedPanelId() const
    {
        return DraggedPanelId;
    }

    /** @return True while a drag is in flight. */
    NODISCARD FORCEINLINE bool IsDragging() const
    {
        return !DraggedPanelId.IsEmpty();
    }

    /** @return Where the cursor last was, in screen coordinates. */
    NODISCARD FORCEINLINE const IntVector2& GetCursorPosition() const
    {
        return CursorPosition;
    }

    /** @return The area the drag would drop into, or null when the cursor is over none of them. */
    NODISCARD FORCEINLINE FDockingArea* GetTargetArea() const
    {
        return TargetArea;
    }

    /** @return The panel the drop would be placed against, which is empty when it would target the root. */
    NODISCARD FORCEINLINE const String& GetTargetPanelId() const
    {
        return TargetPanelId;
    }

    /** @return Which side of the target the drop would land on. */
    NODISCARD FORCEINLINE EDockDirection GetTargetDirection() const
    {
        return TargetDirection;
    }

    /** @return True when the cursor is somewhere a drop would actually land. */
    NODISCARD FORCEINLINE bool HasTarget() const
    {
        return TargetArea != nullptr;
    }

private:
    void ClearTarget();

    String                DraggedPanelId;
    FDockingArea*         SourceArea;
    FDockingArea*         TargetArea;
    String                TargetPanelId;
    EDockDirection        TargetDirection;
    IntVector2            CursorPosition;
    TArray<FDockingArea*> RegisteredAreas;

    static TUniquePtr<FDockDragState> DockDragState;
};
