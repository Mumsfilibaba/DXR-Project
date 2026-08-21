#pragma once
#include "Application/Elements/CompoundElement.h"

class APPLICATION_API FScrollBoxElement final : public FCompoundElement
{
public:
    static constexpr int32 DefaultScrollAmountPerWheelStep = 48;

public:
    static TSharedPtr<FScrollBoxElement> Create();

public:
    FScrollBoxElement();
    virtual ~FScrollBoxElement();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Requests that the bottom of the content sit flush with the bottom of the view.
     *
     * Deferred to the next arrange, because the content height is only known once the children have
     * had their desired sizes computed.
     */
    void ScrollToEnd();

    /**
     * @brief Scrolls the least amount that brings the rectangle fully into view.
     *
     * @param ContentRelativeBounds The rectangle to reveal, relative to the top of the content.
     */
    void ScrollIntoView(const FRectangle& ContentRelativeBounds);

    /**
     * @brief Scrolls to an absolute offset, clamped into the scrollable range.
     *
     * @param InScrollOffset The offset from the top of the content, in pixels.
     */
    void SetScrollOffset(int32 InScrollOffset);

    /** @brief The current offset from the top of the content, in pixels. */
    NODISCARD FORCEINLINE int32 GetScrollOffset() const
    {
        return ScrollOffset;
    }

    /** @brief The largest offset that still shows content, given the current view and content size. */
    NODISCARD int32 GetMaxScrollOffset() const;

    /** @brief True when the bottom of the content is flush with the bottom of the view. */
    NODISCARD bool IsScrolledToEnd() const;

    /**
     * @brief Sets how far one wheel step scrolls.
     *
     * @param InAmount The distance in pixels.
     */
    void SetScrollAmountPerWheelStep(int32 InAmount);

private:
    int32 ScrollOffset;
    int32 ScrollAmountPerWheelStep;
    int32 ContentHeight;
    int32 ViewHeight;
    bool  bIsScrollToEndPending : 1;
};
