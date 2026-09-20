#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Layout/LayoutTypes.h"

/** @brief Called with the offset the thumb was dragged or paged to. */
DECLARE_DELEGATE(FOnScrollBarOffsetChanged, int32 /*NewOffset*/);

class APPLICATION_API FScrollBar final : public FInteractiveElement
{
public:
    struct FDesc
    {
        EOrientation              Orientation = EOrientation::Vertical;
        int32                     Thickness = FUIStyle::GetDefault().Metrics.ScrollBarThickness;
        int32                     MinThumbLength = 24;
        FMargin                   TrackPadding = FMargin(2);
        FUIScrollBarStyle         Style = FUIStyle::GetDefault().ScrollBar;
        FOnScrollBarOffsetChanged OnOffsetChanged;

        /**
         * @brief Whether the bar keeps itself out of sight until its view is revealed to it.
         *
         * A bar that does starts clear and fades in and out as SetRevealed is told the cursor comes and
         * goes, so the chrome is only there while it is any use. One that does not stays fully drawn.
         */
        bool bAutoHide = false;
    };

public:
    static TSharedPtr<FScrollBar> Create(const FDesc& Desc);

public:
    FScrollBar();
    virtual ~FScrollBar();

    /**
     * @brief Initializes the scroll bar with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Tells the bar how much there is to scroll and how much of it is on screen.
     *
     * @param InContentLength The full extent of the content along the scrolled axis.
     * @param InViewLength    The extent of the window onto that content.
     * @param InOffset        How far the view has already been scrolled.
     */
    void SetScrollState(int32 InContentLength, int32 InViewLength, int32 InOffset);

    /**
     * @brief Scrolls to an absolute offset, clamped into the scrollable range, without firing the delegate.
     *
     * @param InOffset The offset from the start of the content, in pixels.
     */
    void SetOffset(int32 InOffset);

    /** @return How far the view has been scrolled, from the start of the content, in pixels. */
    NODISCARD FORCEINLINE int32 GetOffset() const
    {
        return Offset;
    }

    /** @return The content length less the view length, or zero when everything already fits. */
    NODISCARD int32 GetMaxOffset() const;

    /**
     * @brief Gets how much of the content is on screen, which is what the thumb is sized from.
     *
     * @return The view length over the content length, capped at one, and one when there is no content.
     */
    NODISCARD float GetVisibleFraction() const;

    /** @return True when the content is longer than the view. The bar draws only its track when it is not. */
    NODISCARD bool IsScrollable() const;

    /** @return The axis the thumb travels along, which decides which of the two extents the thickness is. */
    NODISCARD FORCEINLINE EOrientation GetOrientation() const
    {
        return Orientation;
    }

    /**
     * @brief Gets the rectangle the thumb occupies, which is what a drag grabs.
     *
     * @return The thumb rectangle, which fills the whole track when everything fits.
     */
    NODISCARD FRectangle GetThumbBounds() const;

    /**
     * @brief Scales the alpha of everything the bar draws, which is how an overlay bar fades in and out.
     *
     * @param InOpacity The multiplier, clamped into zero to one.
     */
    void SetOpacity(float InOpacity);

    /** @return The multiplier applied to the alpha of the track and the thumb. */
    NODISCARD FORCEINLINE float GetOpacity() const
    {
        return Opacity;
    }

    /**
     * @brief Tells a bar that hides itself whether its view has the cursor, which is what it fades on.
     *
     * The fade itself runs from the next arrange on, over the style's two durations. A bar being dragged
     * stays in sight however far the cursor has wandered off it.
     *
     * @param bInIsRevealed True while the cursor is over the view the bar scrolls.
     */
    void SetRevealed(bool bInIsRevealed);

    /** @return True when the bar keeps itself out of sight until its view is revealed to it. */
    NODISCARD FORCEINLINE bool IsAutoHiding() const
    {
        return bAutoHide;
    }

    /** @return True while the cursor was last reported as being over the view the bar scrolls. */
    NODISCARD FORCEINLINE bool IsRevealed() const
    {
        return bIsRevealed;
    }

    /** @return The look the bar draws itself with. */
    NODISCARD FORCEINLINE const FUIScrollBarStyle& GetStyle() const
    {
        return Style;
    }

protected:

    // FInteractiveElement Interface
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;

    virtual bool IsPressable() const override { return false; }

private:
    NODISCARD FRectangle ComputeTrackBounds(const FRectangle& Bounds) const;
    NODISCARD FRectangle ComputeThumbBounds(const FRectangle& Bounds) const;
    NODISCARD int32 GetThumbTravel(const FRectangle& Bounds) const;
    NODISCARD FFloatColor ApplyOpacity(const FFloatColor& Color) const;

    void ApplyOffset(int32 InOffset);
    void SetOffsetFromPosition(const IntVector2& ClientPosition);
    void AdvanceFade();

    EOrientation              Orientation;
    int32                     Thickness;
    int32                     MinThumbLength;
    FMargin                   TrackPadding;
    FUIScrollBarStyle         Style;
    int32                     ContentLength;
    int32                     ViewLength;
    int32                     Offset;
    int32                     ThumbGrabOffset;
    float                     Opacity;
    uint64                    FadeCounter;
    bool                      bAutoHide;
    bool                      bIsRevealed;

    FOnScrollBarOffsetChanged OnOffsetChangedDelegate;
};
