#pragma once
#include "Application/Elements/CompoundElement.h"

enum class EScrollBarVisibility : uint8
{
    /** @brief Shown only while there is something to scroll, which is what a log view wants. */
    Auto,

    /** @brief Always shown, so the content never shifts sideways as it grows past the view. */
    Always,

    /** @brief Never shown, leaving the wheel as the only way to scroll. */
    Never,
};

class APPLICATION_API FScrollBox final : public FCompoundElement
{
public:
    static constexpr int32 DefaultScrollAmountPerWheelStep = 48;

public:
    static TSharedPtr<FScrollBox> Create();

public:
    FScrollBox();
    virtual ~FScrollBox();

    /** @brief Builds the scroll bar the box hosts, which needs the box to already be shared. */
    void Initialize();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Requests that the bottom of the content sit flush with the bottom of the view. Deferred to
     * the next arrange, because the content height is only known once the children have been measured.
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

    /** @return How far the content is scrolled, as an offset from its top in pixels. */
    NODISCARD FORCEINLINE int32 GetScrollOffset() const
    {
        return ScrollOffset;
    }

    /**
     * @brief Gets the far end of the scrollable range, measured on the last arrange.
     *
     * @return How far the content overhangs the view, in pixels, or zero when it fits.
     */
    NODISCARD int32 GetMaxScrollOffset() const;

    /**
     * @brief Gets whether the view sits at the far end of the content.
     *
     * @return True when the bottom of the content is flush with the bottom of the view, which content
     * that fits always is.
     */
    NODISCARD bool IsScrolledToEnd() const;

    /**
     * @brief Sets how far one wheel step scrolls.
     *
     * @param InAmount The distance in pixels.
     */
    void SetScrollAmountPerWheelStep(int32 InAmount);

    /**
     * @brief Sets when the scroll bar is drawn beside the content.
     *
     * @param InVisibility The rule to apply.
     */
    void SetScrollBarVisibility(EScrollBarVisibility InVisibility);

    /**
     * @brief Sets extra space held between the content and a visible bar, so framed views keep their gutter.
     *
     * @param InGutter The gap in pixels. Zero leaves the content flush with the bar.
     */
    void SetScrollBarGutter(int32 InGutter);

    /**
     * @brief Gets whether the scroll bar is drawn beside the content.
     *
     * @return True when the bar is drawn, which for Auto means there is something to scroll.
     */
    NODISCARD bool IsScrollBarVisible() const;

    /** @return This element as a scroll box. */
    NODISCARD virtual FScrollBox* AsScrollBox() override
    {
        return this;
    }

    /** @return The scroll bar the box hosts, which a caller can style but does not own. */
    NODISCARD FORCEINLINE const TSharedPtr<class FScrollBar>& GetScrollBar() const
    {
        return ScrollBar;
    }

private:
    NODISCARD FRectangle GetViewBounds(const FRectangle& AllottedBounds) const;

    TSharedPtr<class FScrollBar> ScrollBar;
    EScrollBarVisibility         ScrollBarVisibility;
    int32                        ScrollOffset;
    int32                        ScrollAmountPerWheelStep;
    int32                        ContentHeight;
    int32                        ViewHeight;
    int32                        ScrollBarGutter;
    bool                         bIsScrollToEndPending : 1;
};
