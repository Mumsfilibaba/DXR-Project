#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class FTabStrip;

DECLARE_DELEGATE(FOnTabActivated, const String& /*PanelId*/);
DECLARE_DELEGATE(FOnTabClosed, const String& /*PanelId*/);
DECLARE_DELEGATE(FOnTabReordered, const String& /*PanelId*/, int32 /*NewIndex*/);
DECLARE_DELEGATE(FOnTabDragDetached, const String& /*PanelId*/, const IntVector2& /*ClientPosition*/, const IntVector2& /*ScreenPosition*/);
DECLARE_DELEGATE(FOnTabDragMoved, const String& /*PanelId*/, const IntVector2& /*ClientPosition*/, const IntVector2& /*ScreenPosition*/);
DECLARE_DELEGATE(FOnTabDragFinished, const String& /*PanelId*/, const IntVector2& /*ClientPosition*/, const IntVector2& /*ScreenPosition*/);

class APPLICATION_API FTab final : public FInteractiveElement
{
public:

    /**
     * @brief Creates a tab.
     *
     * @param InPanelId    The panel the tab stands for.
     * @param InLabel      The text it shows.
     * @param InFont       The face the label is drawn with.
     * @param bInIsClosable Whether it draws a cross that closes the panel.
     */
    static TSharedPtr<FTab> Create(const String& InPanelId, const String& InLabel, const TSharedPtr<IFontFace>& InFont, bool bInIsClosable);

public:
    FTab();
    virtual ~FTab();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Tells the tab which strip it belongs to, which the strip does as it builds it.
     *
     * @param InOwnerStrip The strip the tab sits in, which outlives it.
     */
    void SetOwner(FTabStrip* InOwnerStrip);

    /**
     * @brief Marks the tab as the one whose panel is showing.
     *
     * @param bInIsActive True for the active tab.
     */
    void SetActive(bool bInIsActive);

    /**
     * @brief Replaces the look of the tab, which the strip holding it does as it builds it.
     *
     * @param InStyle The look to draw and measure with.
     */
    void SetStyle(const FUITabStyle& InStyle);

    /**
     * @brief Sets the glyph the close button draws, which falls back to a drawn cross while no texture is set.
     *
     * @param InCloseIcon The brush to draw.
     */
    void SetCloseIcon(const FUIBrush& InCloseIcon);

    /**
     * @brief Gets where the close cross sits, in the space the tab was arranged in.
     *
     * @return The rectangle, which is empty for a tab that cannot be closed.
     */
    NODISCARD FRectangle GetCloseButtonRectangle() const;

    /** @return The panel the tab stands for, which is how the strip and the area refer to it. */
    NODISCARD FORCEINLINE const String& GetPanelId() const
    {
        return PanelId;
    }

    /** @return The text the tab shows. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

    /** @return True for the tab whose panel is showing. */
    NODISCARD FORCEINLINE bool IsActive() const
    {
        return bIsActive;
    }

    /** @return True when the tab draws a cross that closes the panel. */
    NODISCARD FORCEINLINE bool IsClosable() const
    {
        return bIsClosable;
    }

protected:

    // FInteractiveElement Interface
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;

private:
    String                PanelId;
    String                Label;
    TSharedPtr<IFontFace> Font;
    FUITabStyle           Style;
    FUIBrush              CloseIcon;
    FTabStrip*            OwnerStrip;
    bool                  bIsClosable;
    bool                  bIsActive;
    bool                  bIsCloseHovered;
};

class APPLICATION_API FTabStrip final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The face every tab label is drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The look of the tabs, which defaults to the shared tab style. */
        FUITabStyle Style = FUIStyle::GetDefault().Tab;

        /** @brief The glyph every close button draws, which falls back to a drawn cross while no texture is set. */
        FUIBrush CloseIcon;

        /** @brief Whether a tab can be dragged into a new position within the strip. */
        bool bAllowReorder : 1 = true;

        /** @brief Whether a drag far enough off the strip detaches the tab instead of reordering it. */
        bool bAllowTearOut : 1 = true;

        /** @brief Fired when a tab is pressed and becomes the one whose panel shows. */
        FOnTabActivated OnTabActivated;

        /** @brief Fired when a press and a release both land on a tab's close cross. */
        FOnTabClosed OnTabClosed;

        /** @brief Fired once tabs have been reordered, carrying the index the tab landed on. */
        FOnTabReordered OnTabReordered;

        /** @brief Fired once a drag leaves the strip, which is what starts a dock drag. */
        FOnTabDragDetached OnTabDragDetached;

        /** @brief Fired for every move after that, so the drop target can follow the cursor. */
        FOnTabDragMoved OnTabDragMoved;

        /** @brief Fired when a detached drag is let go, which is what settles the drop. */
        FOnTabDragFinished OnTabDragFinished;
    };

public:

    /** @brief How far off the strip a drag has to go before the panel is torn out, in pixels. */
    static constexpr int32 TearOutDistance = 24;

    /** @brief How far one notch of the wheel scrolls the strip sideways, in pixels. */
    static constexpr int32 DefaultScrollAmountPerWheelStep = 60;

public:
    static TSharedPtr<FTabStrip> Create(const FDesc& Desc);

public:
    FTabStrip();
    virtual ~FTabStrip();

    /**
     * @brief Initializes the strip with the specified parameters, and builds the scroll bar it hosts.
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
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Scrolls to an absolute offset, clamped into the scrollable range.
     *
     * @param InScrollOffset How far along the tabs to scroll, in pixels from the leading edge.
     */
    void SetScrollOffset(int32 InScrollOffset);

    /** @return How far the strip has been scrolled from its leading edge, in pixels. */
    NODISCARD FORCEINLINE int32 GetScrollOffset() const
    {
        return ScrollOffset;
    }

    /** @return The content width less the view width, or zero while every tab already fits. */
    NODISCARD int32 GetMaxScrollOffset() const;

    /**
     * @brief Scrolls the least distance that brings a tab fully into view, doing nothing when it already is.
     *
     * @param PanelId The panel whose tab to reveal. An unknown id is ignored.
     */
    void ScrollTabIntoView(const String& PanelId);

    /** @return The bar drawn under the tabs, which only shows itself while they overflow. */
    NODISCARD FORCEINLINE const TSharedPtr<class FScrollBar>& GetScrollBar() const
    {
        return ScrollBar;
    }

    /**
     * @brief Appends a tab at the far end.
     *
     * @param PanelId     The panel the tab stands for.
     * @param Label       The text it shows.
     * @param bIsClosable Whether it draws a cross that closes the panel.
     */
    void AddTab(const String& PanelId, const String& Label, bool bIsClosable);

    /**
     * @brief Removes a tab, moving the active one when it was the one removed.
     *
     * @param PanelId The panel whose tab to remove.
     */
    void RemoveTab(const String& PanelId);

    /**
     * @brief Shows a panel's tab as the active one.
     *
     * @param PanelId The panel to show. An unknown id is ignored.
     */
    void SetActiveTab(const String& PanelId);

    /** @brief Drops every tab, so the strip can be refilled. */
    void ClearTabs();

    /**
     * @brief Reports a press on a tab, which the tab does rather than acting on the press itself.
     *
     * @param Tab            The tab pressed.
     * @param ClientPosition Where the press landed.
     */
    void OnTabPressed(FTab* Tab, const IntVector2& ClientPosition);

    /**
     * @brief Reports the cursor moving while a tab is held, which reorders it or tears it out. The strip needs
     * the client position for its own geometry and the listener needs the screen position, because a torn-out
     * panel leaves the window the client position is measured against.
     *
     * @param Tab            The tab held.
     * @param ClientPosition Where the cursor is now, relative to the window the strip is in.
     * @param ScreenPosition Where the cursor is now, in screen space.
     */
    void OnTabDragged(FTab* Tab, const IntVector2& ClientPosition, const IntVector2& ScreenPosition);

    /**
     * @brief Reports a tab being let go, which closes it when both ends of the click landed on its cross
     * and otherwise settles whatever the drag was doing.
     *
     * @param Tab            The tab released.
     * @param ClientPosition Where the cursor was, relative to the window the strip is in.
     * @param ScreenPosition Where the cursor was, in screen space.
     */
    void OnTabReleased(FTab* Tab, const IntVector2& ClientPosition, const IntVector2& ScreenPosition);

    /** @return The tabs the strip holds, in the order they are laid out along the strip. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FTab>>& GetTabs() const
    {
        return Tabs;
    }

    /** @return The panel whose tab is active, or an empty string when the strip has no tabs. */
    NODISCARD const String& GetActivePanelId() const;

    /**
     * @brief Gets the panel being dragged along the strip.
     *
     * @return The panel id, which is empty when no tab is held and once a held tab has been torn out.
     */
    NODISCARD FORCEINLINE const String& GetDraggedPanelId() const
    {
        return DraggedPanelId;
    }

    /**
     * @brief Gets the panel dragged clear of the strip, which is the one a drop settles.
     *
     * @return The panel id, which is empty until a drag tears one out and stays set until it is let go.
     */
    NODISCARD FORCEINLINE const String& GetDetachedPanelId() const
    {
        return DetachedPanelId;
    }

    /** @return The look the strip and its tabs draw themselves with. */
    NODISCARD FORCEINLINE const FUITabStyle& GetStyle() const
    {
        return Style;
    }

private:
    NODISCARD int32 FindTabIndexAt(int32 PositionX) const;
    NODISCARD int32 FindTabIndex(const FTab* Tab) const;
    NODISCARD double GetSecondsSinceFadeStart() const;

    void MoveTab(int32 FromIndex, int32 ToIndex);
    void UpdateScrollBar(const FRectangle& AllottedBounds);
    void SetCursorOver(bool bInIsCursorOver);

    TSharedPtr<IFontFace>        Font;
    FUITabStyle                  Style;
    FUIBrush                     CloseIcon;
    TArray<TSharedPtr<FTab>>     Tabs;
    TSharedPtr<class FScrollBar> ScrollBar;
    String                       ActivePanelId;
    String                       DraggedPanelId;
    String                       DetachedPanelId;
    IntVector2                   DragOrigin;
    int32                        ScrollOffset;
    int32                        ScrollAmountPerWheelStep;
    int32                        ContentWidth;
    int32                        ViewWidth;
    uint64                       FadeStartCounter;
    float                        FadeStartOpacity;
    float                        ScrollBarOpacity;
    bool                         bIsCursorOver;
    bool                         bAllowReorder;
    bool                         bAllowTearOut;
    FOnTabActivated              OnTabActivatedDelegate;
    FOnTabClosed                 OnTabClosedDelegate;
    FOnTabReordered              OnTabReorderedDelegate;
    FOnTabDragDetached           OnTabDragDetachedDelegate;
    FOnTabDragMoved              OnTabDragMovedDelegate;
    FOnTabDragFinished           OnTabDragFinishedDelegate;
};
