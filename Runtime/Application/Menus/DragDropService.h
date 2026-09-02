#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"

struct FDragDropPayload
{
    /** @return True when the payload names a type, which every drag in flight does. */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return !TypeId.IsEmpty();
    }

    /** @brief What is being dragged, which is how a target decides whether it wants the drop. */
    String TypeId;

    /** @brief The text drawn under the cursor while the drag is in flight. */
    String DisplayText;

    /** @brief The icon drawn beside the display text, which is unset for a payload without one. */
    FUIBrush Icon;

    /** @brief What the payload stands for, which the source and the target agree on through the type id. */
    void* UserData = nullptr;
};

/** @brief Called on the target under the cursor once the drag ends over it. */
DECLARE_DELEGATE(FOnDragDropped, const FDragDropPayload& /*Payload*/, const IntVector2& /*ScreenPosition*/);

/** @brief Asks a target whether it would take the payload, which is what picks one out of the targets under the cursor. */
DECLARE_RETURN_DELEGATE(FOnDragOver, bool, const FDragDropPayload& /*Payload*/);

class APPLICATION_API FDragDropService
{
public:

    /** @brief The index reported while no registered target under the cursor wants the payload. */
    static constexpr int32 InvalidTargetIndex = -1;

    /** @brief The gap between the cursor and the ghost drawn under it, in pixels. */
    static constexpr int32 DragVisualCursorOffset = 12;

    /** @brief The side of the icon square drawn in the ghost, in pixels. */
    static constexpr int32 DragVisualIconSize = 16;

    /** @brief The space between the ghost's edge and what it holds, in pixels. */
    static constexpr int32 DragVisualPadding = 6;

public:

    /**
     * @brief Gets the process-wide service, which is what every drag source and drop target goes through.
     *
     * @return The service, created on the first call.
     */
    NODISCARD static FDragDropService& Get();

    /** @brief Cancels any drag in flight and drops the process-wide service, which a host calls before the application goes. */
    static void Release();

public:
    FDragDropService();
    ~FDragDropService();

    FDragDropService(const FDragDropService&) = delete;
    FDragDropService& operator=(const FDragDropService&) = delete;

    /**
     * @brief Starts a drag, which a source does once the cursor has moved far enough to mean one.
     *
     * @param InPayload        What is being dragged, which is ignored when it names no type.
     * @param InScreenPosition Where the cursor was, in screen coordinates.
     */
    void BeginDrag(const FDragDropPayload& InPayload, const IntVector2& InScreenPosition);

    /**
     * @brief Moves the drag, re-resolving which target is under the cursor.
     *
     * @param InScreenPosition Where the cursor is now, in screen coordinates.
     */
    void UpdateDrag(const IntVector2& InScreenPosition);

    /**
     * @brief Ends the drag, dropping onto the target under the cursor and dropping nothing when there is
     * none. The payload is handed to the target before the drag is cleared, so a handler that starts
     * another drag is not undone by this one ending.
     *
     * @param InScreenPosition Where the cursor was released, in screen coordinates.
     */
    void EndDrag(const IntVector2& InScreenPosition);

    /** @brief Ends the drag without dropping anything, which is what Escape does. */
    void CancelDrag();

    /** @return True while a drag is in flight. */
    NODISCARD FORCEINLINE bool IsDragging() const
    {
        return Payload.IsValid();
    }

    /** @return What is being dragged, which names no type when no drag is in flight. */
    NODISCARD FORCEINLINE const FDragDropPayload& GetPayload() const
    {
        return Payload;
    }

    /** @return Where the cursor last was, in screen coordinates. */
    NODISCARD FORCEINLINE const IntVector2& GetScreenPosition() const
    {
        return ScreenPosition;
    }

    /** @return True when the cursor is over a target that would take the payload. */
    NODISCARD FORCEINLINE bool HasTarget() const
    {
        return TargetIndex != InvalidTargetIndex;
    }

    /**
     * @brief Registers an element as a possible drop target, replacing its earlier registration. Held
     * weakly, so an element that goes without unregistering is skipped rather than followed.
     *
     * @param Target    The element to register.
     * @param OnDropped Fired when a drag ends over the element.
     * @param OnOver    Asked whether the element would take a payload. An unbound delegate takes every one.
     */
    void RegisterTarget(const TWeakPtr<FVisualElement>& Target, const FOnDragDropped& OnDropped, const FOnDragOver& OnOver);

    /**
     * @brief Forgets an element, which every registered element does when it is destroyed.
     *
     * @param Target The element to forget.
     */
    void UnregisterTarget(const TWeakPtr<FVisualElement>& Target);

    /**
     * @brief Appends the ghost drawn under the cursor while a drag is in flight, and nothing when there is none.
     *
     * @param OutCommandList The list to append to.
     * @param LayerId        The layer the ghost draws on, its contents one above.
     * @param ClientOrigin   Where the drawing window's client area starts on screen, which is subtracted from the
     *                       drag's screen position to land the ghost in that window's own coordinates.
     */
    void DrawDragVisual(FDrawCommandList& OutCommandList, int32 LayerId, const IntVector2& ClientOrigin = IntVector2(0, 0)) const;

private:
    struct FTarget
    {
        FTarget()
            : Element()
            , OnDropped()
            , OnOver()
        {
        }

        TWeakPtr<FVisualElement> Element;
        FOnDragDropped           OnDropped;
        FOnDragOver              OnOver;
    };

    NODISCARD static FRectangle ResolveTargetBounds(const TSharedPtr<FVisualElement>& Element);

    NODISCARD int32 FindTargetAt(const IntVector2& InScreenPosition) const;

    FDragDropPayload Payload;
    IntVector2       ScreenPosition;
    TArray<FTarget>  Targets;
    int32            TargetIndex;

    static TUniquePtr<FDragDropService> DragDropService;
};
