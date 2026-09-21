#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Text/IFontFace.h"

class FScrollBar;

/** @brief Called with the bar a click landed on, or with two invalid indices when the click cleared the selection. */
DECLARE_DELEGATE(FOnProfilerBarSelected, int32 /*LaneIndex*/, int32 /*BarIndex*/);

/** @brief Builds the context menu for a bar; returning null leaves the right-click's reset behavior in place. */
DECLARE_RETURN_DELEGATE(FOnGetProfilerBarContextMenu, TSharedPtr<FVisualElement>, int32 /*LaneIndex*/, int32 /*BarIndex*/);

struct FProfilerTimelineBar
{
    /** @brief The scope's name, which the host owns and which outlives the widget. */
    const CHAR* Name = nullptr;

    /** @brief When the scope opened, relative to whatever zero the host handed the lane set. */
    uint64 StartNanoseconds = 0;

    /** @brief When the scope closed, on the same clock as StartNanoseconds. */
    uint64 EndNanoseconds = 0;

    /** @brief How many scopes are open around this one, which is the row it is drawn on inside its lane. */
    int32 Depth = 0;

    /** @brief The bar this one opened inside, or -1 for a bar at the root of its lane. */
    int32 ParentIndex = -1;

    /** @brief True when a search is active and this name does not match, so the bar is drawn faded. */
    bool bDimmed = false;

    /** @brief True for a zero-duration marker, which is drawn as a tick rather than a span. */
    bool bInstant = false;
};

struct FProfilerTimelineLane
{
    /** @brief The name drawn in the gutter, which is the thread's name or "GPU". */
    String Label;

    /** @brief The dimmed line under the label, which is where a lane says how busy it was. */
    String SubLabel;

    /** @brief The scopes on the lane, in the order they opened. */
    TArray<FProfilerTimelineBar> Bars;

    /** @brief True for the queue lane, which is shaded apart from the thread lanes. */
    bool bIsGpu = false;
};

class APPLICATION_API FProfilerTimeline final : public FInteractiveElement
{
public:

    /** @brief The index reported for no lane and no bar. */
    static constexpr int32 InvalidIndex = -1;

    struct FDesc
    {
        /** @brief The face the gutter, the ruler and the bar labels are drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief Fired with the bar a click landed on. */
        FOnProfilerBarSelected OnBarSelected;

        /** @brief Builds the menu shown by a right-click on a bar. */
        FOnGetProfilerBarContextMenu OnGetBarContextMenu;

        /** @brief The height the widget asks for before its lanes outgrow it, in pixels. */
        int32 PreferredHeight = 180;
    };

public:
    static TSharedPtr<FProfilerTimeline> Create(const FDesc& Desc);

public:
    FProfilerTimeline();
    virtual ~FProfilerTimeline();

    void Initialize(const FDesc& Desc);

    // FVisualElement interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Replaces the lanes, moving every bar onto a zero that is the earliest one of them.
     *
     * The selection survives when the bar it named is still there, and the view survives whenever the
     * user has moved it, so refilling this every frame does not throw away what is being looked at.
     *
     * @param InLanes The lanes to show, in the order their tracks appear.
     */
    void SetLanes(TArray<FProfilerTimelineLane> InLanes);

    /**
     * @brief Selects a bar without firing the delegate, which is how the tree pushes its selection over.
     *
     * @param LaneIndex The lane the bar is on, an out of range one clearing the selection.
     * @param BarIndex  The bar on that lane, an out of range one clearing the selection.
     */
    void SetSelectedBar(int32 LaneIndex, int32 BarIndex);

    /** @brief Puts the view back over the whole capture and hands it back to whatever fills the lanes. */
    void ResetView();

    /** @return The lane the selection is on, or InvalidIndex while nothing is selected. */
    NODISCARD FORCEINLINE int32 GetSelectedLane() const
    {
        return SelectedLane;
    }

    /** @return The selected bar on that lane, or InvalidIndex while nothing is selected. */
    NODISCARD FORCEINLINE int32 GetSelectedBar() const
    {
        return SelectedBar;
    }

    /** @return The lanes as they are held, which is with every bar moved onto the shared zero. */
    NODISCARD FORCEINLINE const TArray<FProfilerTimelineLane>& GetLanes() const
    {
        return Lanes;
    }

    /** @return How long the whole capture ran, in nanoseconds, which is what the view is fitted to. */
    NODISCARD FORCEINLINE uint64 GetTotalSpanNanoseconds() const
    {
        return TotalSpanNanoseconds;
    }

    /** @return True once the wheel or a drag has moved the view off the whole capture. */
    NODISCARD FORCEINLINE bool IsViewUserAdjusted() const
    {
        return bViewIsUserAdjusted;
    }

    /** @return Where the view starts, in nanoseconds from the capture's zero. */
    NODISCARD FORCEINLINE float GetViewStartNanoseconds() const
    {
        return ViewStartNs;
    }

    /** @return How much time the view covers, in nanoseconds. */
    NODISCARD FORCEINLINE float GetViewSpanNanoseconds() const
    {
        return ViewSpanNs;
    }

    /** @return The horizontal bar that represents the zoomed viewport, primarily for hosting and tests. */
    NODISCARD FORCEINLINE const TSharedPtr<FScrollBar>& GetHorizontalScrollBar() const
    {
        return HorizontalScrollBar;
    }

    /** @return True while zoom leaves some of the capture outside the viewport. */
    NODISCARD bool IsHorizontalScrollBarVisible() const;

protected:
    virtual bool IsPressable() const override
    {
        return false;
    }

private:
    struct FHit
    {
        int32 Lane = InvalidIndex;
        int32 Bar  = InvalidIndex;
    };

    NODISCARD static int32 GetLaneRowCount(const FProfilerTimelineLane& Lane);
    NODISCARD static String FormatBarToolTip(const FProfilerTimelineBar& Bar);

    NODISCARD int32 GetRowHeight() const;
    NODISCARD int32 GetRulerHeight() const;
    NODISCARD int32 GetLaneHeight(const FProfilerTimelineLane& Lane) const;
    NODISCARD FRectangle GetTrackBounds(const FRectangle& Bounds) const;
    NODISCARD FRectangle GetLanesBounds(const FRectangle& Bounds) const;
    NODISCARD FRectangle GetScrollBarBounds(const FRectangle& Bounds) const;
    NODISCARD int32 GetLaneTop(const FRectangle& Bounds, int32 LaneIndex) const;
    NODISCARD FHit HitTest(const IntVector2& ClientPosition) const;
    NODISCARD bool ProjectBar(const FRectangle& Track, const FProfilerTimelineBar& Bar, int32& OutLeft, int32& OutWidth) const;

    void FitView();
    void ClampView();
    void SyncScrollBar();
    void OnScrollBarMoved(int32 NewOffset);
    void ZoomAt(const IntVector2& ClientPosition, float Factor);
    void UpdateBarToolTip(const FCursorEvent& CursorEvent);
    void DrawRuler(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId) const;

    TSharedPtr<IFontFace>         Font;
    TSharedPtr<FScrollBar>        HorizontalScrollBar;
    TArray<FProfilerTimelineLane> Lanes;
    FOnProfilerBarSelected        OnBarSelected;
    FOnGetProfilerBarContextMenu  OnGetBarContextMenu;
    uint64                        TotalSpanNanoseconds;
    int32                         PreferredHeight;
    int32                         SelectedLane;
    int32                         SelectedBar;
    int32                         HoveredLane;
    int32                         HoveredBar;
    float                         ViewStartNs;
    float                         ViewSpanNs;
    float                         DragStartView;
    IntVector2                    DragOrigin;
    bool                          bDragging;
    bool                          bViewIsUserAdjusted;
};
