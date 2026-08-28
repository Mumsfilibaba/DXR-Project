#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Docking/DockNode.h"
#include "Application/Elements/VisualElement.h"

/** @brief Called when a handle drag settles, carrying the shares every child ended up with. */
DECLARE_DELEGATE(FOnSplitterFractionsChanged, const TArray<float>& /*Fractions*/);

class APPLICATION_API FSplitter final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief Horizontal to sit the children side by side, Vertical to stack them. */
        EDockSplitOrientation Orientation = EDockSplitOrientation::Horizontal;

        /** @brief How wide the draggable handle between two children is, in pixels. Clamped to at least one. */
        int32 HandleThickness = FDockMetrics::SplitterThickness;

        /** @brief One share per child, normalized on use. Left empty to share the space evenly. */
        TArray<float> Fractions;

        /** @brief One per child, the least each can be squeezed to. Takes precedence over the size AddChild is given. */
        TArray<IntVector2> MinimumSizes;

        /** @brief Fired when a handle drag settles, carrying the shares every child ended up with. */
        FOnSplitterFractionsChanged OnFractionsChanged;
    };

public:
    static TSharedPtr<FSplitter> Create(const FDesc& Desc);

public:
    FSplitter();
    virtual ~FSplitter();

    /**
     * @brief Initializes the splitter with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual bool GetCursor(ECursor& OutCursor) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Appends a child at the far end, sharing the space evenly unless the description seeded shares.
     *
     * @param InChild       The element to add.
     * @param InMinimumSize The least the child can be squeezed to, ignored where the description named one.
     */
    void AddChild(const TSharedPtr<FVisualElement>& InChild, const IntVector2& InMinimumSize);

    /** @brief Drops every child, so the splitter can be refilled. */
    void ClearChildren();

    /**
     * @brief Replaces the shares, which is how a restored layout takes effect.
     *
     * @param InFractions One per child. A set that does not match the children is ignored.
     */
    void SetFractions(const TArray<float>& InFractions);

    /** @return One share per child, in the order they were added, summing to one. */
    NODISCARD FORCEINLINE const TArray<float>& GetFractions() const
    {
        return Fractions;
    }

    /**
     * @brief The handle between two children.
     *
     * @param HandleIndex The handle, which sits after the child of the same index.
     * @return Its rectangle, which is empty when there is no such handle.
     */
    NODISCARD FRectangle GetHandleRectangle(int32 HandleIndex) const;

    /**
     * @brief Which handle a point is on.
     *
     * @param ClientPosition The point to test, in the space the splitter was arranged in.
     * @return The handle index, or -1 when the point is on none of them.
     */
    NODISCARD int32 GetHandleIndexAt(const IntVector2& ClientPosition) const;

    /** @return The index of the handle being dragged, or -1 when no drag is in flight. */
    NODISCARD FORCEINLINE int32 GetActiveHandleIndex() const
    {
        return ActiveHandleIndex;
    }

    /** @return Horizontal when the children sit side by side, Vertical when they stack. */
    NODISCARD FORCEINLINE EDockSplitOrientation GetOrientation() const
    {
        return Orientation;
    }

private:
    bool TryNormalizeFractions(const TArray<float>& InFractions);

    void DragHandle(int32 HandleIndex, int32 DeltaPixels);

    int32 GetAvailableLength(const FRectangle& Bounds) const;
    int32 GetChildMinimumLength(int32 ChildIndex) const;

    EDockSplitOrientation               Orientation;
    int32                               HandleThickness;
    int32                               ActiveHandleIndex;
    int32                               HoveredHandleIndex;
    IntVector2                          DragOrigin;
    TArray<float>                       DragStartFractions;
    TArray<float>                       Fractions;
    TArray<IntVector2>                  MinimumSizes;
    TArray<TSharedPtr<FVisualElement>>  Children;
    FOnSplitterFractionsChanged         OnFractionsChangedDelegate;
};
