#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Docking/DockNode.h"

class FDockingArea;
class FDrawCommandList;
class FVisualElement;

struct FDockDragPanel
{
    /** @brief The panel being dragged. */
    String PanelId;

    /** @brief The text its tab shows, which is also the caption of any window built for it. */
    String Label;

    /** @brief Its content, which no area holds while the drag does. */
    TSharedPtr<FVisualElement> Content;

    /** @brief The area it was torn out of, which is null once that area has gone. */
    FDockingArea* SourceArea = nullptr;
};

/** @brief Called at tear-out, which is what puts the panel on screen as something to drag. */
DECLARE_DELEGATE(FOnDockDragBegan, const FDockDragPanel& /*Panel*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called when a drag was let go where no area is, which is what floats a panel into a window of its own. */
DECLARE_DELEGATE(FOnDockDropOutside, const FDockDragPanel& /*Panel*/, const IntVector2& /*ScreenPosition*/);

/** @brief Called once a drag is over however it ended, which is what takes down whatever tear-out put up. */
DECLARE_DELEGATE(FOnDockDragEnded);

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
     * @brief Starts a drag, taking the panel out of its area as it does, so what follows the cursor is the
     * panel itself rather than a stand-in for it. The rebuild the source area is left owing is deferred, so
     * the tab still dispatching the event that got here outlives the call.
     *
     * @param PanelId        The panel being dragged.
     * @param SourceArea     The area it came from, which a cancel hands it back to.
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
     * @brief Ends the drag, docking the panel into the last valid target and telling whoever listens when
     * there is none. The drag carries the panel's registration with it, so the target is handed a panel it
     * does not have to already know about.
     */
    void EndDrag();

    /**
     * @brief Ends the drag and puts the panel back where it was torn out of, which is only reachable from a
     * teardown now that Escape commits the drop like a release does. A source area that has since gone
     * leaves nowhere to put it back.
     */
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

    /**
     * @brief Sets what is told that a tear-out has happened, which is what gives the drag something visible
     * to follow the cursor. Nothing else puts the panel on screen while it is in flight.
     *
     * @param InOnDragBegan The delegate to call.
     */
    void SetOnDragBegan(const FOnDockDragBegan& InOnDragBegan);

    /**
     * @brief Sets what is told about a drop that landed on no area at all. The panel is already out of its
     * area by then, so a drop into nowhere with nobody listening is the one path that loses it.
     *
     * @param InOnDropOutside The delegate to call.
     */
    void SetOnDropOutside(const FOnDockDropOutside& InOnDropOutside);

    /**
     * @brief Sets what is told that a drag is over, whichever exit it took. Called after the drop has been
     * committed, so a listener taking down what it put up at tear-out can still be read from until then.
     *
     * @param InOnDragEnded The delegate to call.
     */
    void SetOnDragEnded(const FOnDockDragEnded& InOnDragEnded);

    /** @return The panel being dragged, or an empty string when no drag is in flight. */
    NODISCARD FORCEINLINE const String& GetDraggedPanelId() const
    {
        return DraggedPanelId;
    }

    /** @return The text the ghost shows, which falls back to the panel id when the area had no label for it. */
    NODISCARD FORCEINLINE const String& GetDraggedPanelLabel() const
    {
        return DraggedPanelLabel;
    }

    /** @return The area the drag started in, or null once it has ended. */
    NODISCARD FORCEINLINE FDockingArea* GetSourceArea() const
    {
        return SourceArea;
    }

    /** @return The panel in flight, whose id is empty when no drag is in flight. */
    NODISCARD FDockDragPanel GetDraggedPanel() const;

    /** @return True while a panel torn out of an area is in flight. */
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

    String                     DraggedPanelId;
    String                     DraggedPanelLabel;
    TSharedPtr<FVisualElement> DraggedPanelContent;
    FDockingArea*              SourceArea;
    FDockingArea*              TargetArea;
    String                     TargetPanelId;
    EDockDirection             TargetDirection;
    IntVector2                 CursorPosition;
    TArray<FDockingArea*>      RegisteredAreas;
    FOnDockDragBegan           OnDragBeganDelegate;
    FOnDockDropOutside         OnDropOutsideDelegate;
    FOnDockDragEnded           OnDragEndedDelegate;

    static TUniquePtr<FDockDragState> DockDragState;
};
