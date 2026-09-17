#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Graph/GraphLayout.h"
#include "Application/Graph/GraphModel.h"
#include "Application/Graph/GraphNodeElement.h"
#include "Application/Text/IFontFace.h"

/** @brief Builds the menu a right-click on empty canvas opens, given where in graph space it landed. */
DECLARE_RETURN_DELEGATE(FOnGetGraphContextMenu, TSharedPtr<FVisualElement>, const Vector2& /*GraphPosition*/);

/** @brief Called when the set of selected nodes changed. */
DECLARE_DELEGATE(FOnGraphSelectionChanged);

/** @brief What a drag in progress is doing, which one press picks and the release ends. */
enum class EGraphDragMode : uint8
{
    None,

    /** @brief Sliding the view, on the middle button or on Alt and the left. */
    Pan,

    /** @brief Moving every selected node with the cursor. */
    MoveNodes,

    /** @brief Sweeping a rectangle that selects what it touches. */
    Marquee,

    /** @brief Pulling a link out of a pin, looking for one to land on. */
    Link,
};

class APPLICATION_API FGraphCanvas final : public FInteractiveElement
{
public:

    /** @brief How far the view can zoom out, where one is graph space. */
    static constexpr float MinZoom = 0.25f;

    /** @brief How far the view can zoom in. */
    static constexpr float MaxZoom = 2.5f;

    /** @brief How close to a link the cursor has to be to pick it, in pixels. */
    static constexpr float LinkGrabDistance = 8.0f;

    /** @brief The spacing of the finest grid lines, in graph space, unless the desc names another. */
    static constexpr int32 GridSpacing = 24;

    /** @brief How far the cursor moves before a press on a node counts as a drag rather than a click. */
    static constexpr int32 DragThreshold = 3;

public:
    struct FDesc
    {
        /** @brief The face node titles and pin names are drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The graph the canvas shows and edits, which can be swapped later. */
        TSharedPtr<FGraphModel> Model = nullptr;

        /** @brief What every node is drawn with, apart from the title tint each one carries itself. */
        FGraphNodeStyle NodeStyle;

        /** @brief The fill behind the grid. */
        FFloatColor BackgroundColor = FUIStyle::GetDefault().Colors.WindowBackground;

        /**
         * @brief The color every link is drawn with, left transparent to take the source pin's tint instead,
         * which is how a graph that colors its pins by type shows what a link carries.
         */
        FFloatColor LinkColor = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        /** @brief The spacing of the finest grid lines, in graph space. */
        int32 GridSpacing = FGraphCanvas::GridSpacing;

        /** @brief True to draw the background grid, which a printed or embedded view turns off. */
        bool bShowGrid : 1 = true;

        /**
         * @brief True to run as a viewer: nodes still move, and the selection, the marquee, the pan and the
         * zoom all work, but no link can be drawn and nothing can be deleted.
         */
        bool bIsViewer : 1 = false;

        /** @brief Builds the menu a right-click on empty canvas opens, given where in graph space it landed. */
        FOnGetGraphContextMenu OnGetContextMenu;

        /** @brief Fired when the set of selected nodes changed. */
        FOnGraphSelectionChanged OnSelectionChanged;
    };

public:
    static TSharedPtr<FGraphCanvas> Create(const FDesc& Desc);

public:
    FGraphCanvas();
    virtual ~FGraphCanvas();

    /**
     * @brief Initializes the canvas with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 PrepareDesiredSize() override;
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseScroll(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;
    virtual bool SupportsKeyboardFocus() const override;

    /**
     * @brief Points the canvas at a model, which it observes rather than owns.
     *
     * @param InModel The graph to show.
     */
    void SetModel(const TSharedPtr<FGraphModel>& InModel);

    /** @return The model the canvas is showing, which is null until one is set. */
    NODISCARD FORCEINLINE const TSharedPtr<FGraphModel>& GetModel() const
    {
        return Model;
    }

    /** @brief Pans and zooms so every node fits, which is the Reset View action. */
    void FitToNodes();

    /** @brief Runs FGraphLayout over the model and writes the resulting positions back into it. */
    void AutoLayout();

    /**
     * @brief Zooms about a fixed client point, so the graph position under it does not move.
     *
     * @param ClientPosition The point to hold still.
     * @param ZoomDelta      How much to zoom by, as a multiplier applied to the current zoom.
     */
    void ZoomAt(const IntVector2& ClientPosition, float ZoomDelta);

    /**
     * @brief Sets the scale the graph is drawn at, clamped between the two limits.
     *
     * @param InZoom The scale to use.
     */
    void SetZoom(float InZoom);

    /**
     * @brief Sets where the graph origin sits, relative to the canvas' top-left corner.
     *
     * @param InPan The offset, in pixels.
     */
    void SetPan(const Vector2& InPan);

    /**
     * @brief Projects a graph-space point into the client space the canvas was arranged in.
     *
     * @param GraphPosition The point to project.
     * @return The point in client space, which the pan offsets and the zoom scales.
     */
    NODISCARD Vector2 GraphToScreen(const Vector2& GraphPosition) const;

    /**
     * @brief Projects a client-space point back into graph space, the inverse of GraphToScreen.
     *
     * @param ClientPosition The point to project.
     * @return The point in graph space.
     */
    NODISCARD Vector2 ScreenToGraph(const Vector2& ClientPosition) const;

    /**
     * @brief The node whose rectangle covers a point, in front to back order.
     *
     * @param ClientPosition The position to test.
     * @return The node id, or -1 when the point is over empty canvas.
     */
    NODISCARD int32 FindNodeAt(const IntVector2& ClientPosition) const;

    /**
     * @brief The pin whose circle covers a point.
     *
     * @param ClientPosition The position to test.
     * @return The pin id, or -1 when the point is not on a pin.
     */
    NODISCARD int32 FindPinAt(const IntVector2& ClientPosition) const;

    /**
     * @brief The link whose curve passes within the grab distance of a point.
     *
     * @param ClientPosition The position to test.
     * @return The link id, or -1 when no link is near enough.
     */
    NODISCARD int32 FindLinkAt(const IntVector2& ClientPosition) const;

    /**
     * @brief Replaces the selection.
     *
     * @param NodeIds The nodes to select.
     */
    void SetSelectedNodes(const TArray<int32>& NodeIds);

    /** @brief Drops the node and link selection. */
    void ClearSelection();

    /**
     * @brief Whether a node is one of the selected ones.
     *
     * @param NodeId The node to check.
     * @return True while the node is selected.
     */
    NODISCARD bool IsNodeSelected(int32 NodeId) const;

    /** @brief Removes every selected node and the selected link from the model. */
    void DeleteSelection();

    /** @return The selected node ids, which is empty while a link is selected or nothing is. */
    NODISCARD FORCEINLINE const TArray<int32>& GetSelectedNodes() const
    {
        return SelectedNodeIds;
    }

    /** @return The selected link id, or -1 when no link is selected, as selecting a node drops it. */
    NODISCARD FORCEINLINE int32 GetSelectedLink() const
    {
        return SelectedLinkId;
    }

    /** @return True while the canvas is a viewer, which moves and selects nodes but authors nothing. */
    NODISCARD FORCEINLINE bool IsViewer() const
    {
        return bIsViewer;
    }

    /** @return What every node is drawn with, apart from the title tint each one carries itself. */
    NODISCARD FORCEINLINE const FGraphNodeStyle& GetNodeStyle() const
    {
        return NodeStyle;
    }

    NODISCARD FORCEINLINE float GetZoom() const
    {
        return Zoom;
    }

    NODISCARD FORCEINLINE const Vector2& GetPan() const
    {
        return Pan;
    }

    /** @return The mode the press picked, which is None when nothing is being dragged. */
    NODISCARD FORCEINLINE EGraphDragMode GetDragMode() const
    {
        return DragMode;
    }

    /**
     * @brief Gets the pin a link is being pulled from, which is the one the press landed on.
     *
     * @return The pin id, which is either end of the link to be, or -1 when no link is being pulled.
     */
    NODISCARD FORCEINLINE int32 GetDraggingFromPin() const
    {
        return DraggingFromPinId;
    }

    /** @return The rectangle a marquee covers, in client space, and empty unless one is being swept. */
    NODISCARD FORCEINLINE const FRectangle& GetMarqueeBounds() const
    {
        return MarqueeBounds;
    }

    /** @return The pin under the cursor, tracked while a link is being pulled too, or -1 when over no pin. */
    NODISCARD FORCEINLINE int32 GetHoveredPin() const
    {
        return HoveredPinId;
    }

    /**
     * @brief Gets the link under the cursor, which a hovered pin takes precedence over.
     *
     * @return The link id, or -1 when the cursor is over no link. Left as it was during a drag.
     */
    NODISCARD FORCEINLINE int32 GetHoveredLink() const
    {
        return HoveredLinkId;
    }

    /**
     * @brief The element built for a node, which is what a test or a host reaches a node body through.
     *
     * @param NodeId The node to look for.
     * @return The element, or null.
     */
    NODISCARD TSharedPtr<FGraphNodeElement> FindNodeElement(int32 NodeId) const;

protected:

    // FInteractiveElement Interface
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;
    virtual bool AcceptsPressFromKey(FKey Key) const override;

    virtual bool IsPressable() const override { return false; }

private:
    void RebuildElements();
    void MeasureNodeElements();
    void ArrangeNodeElements();
    void EndDrag(const IntVector2& ClientPosition);

    NODISCARD bool GetLinkCurve(const FGraphLink& Link, Vector2& OutStart, Vector2& OutStartControl, Vector2& OutEndControl, Vector2& OutEnd) const;
    NODISCARD float DistanceToLink(const FGraphLink& Link, const Vector2& ClientPosition) const;

    void DrawGrid(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId) const;
    void DrawLinks(FDrawCommandList& OutCommandList, int32 LayerId) const;

    NODISCARD FFloatColor ResolveLinkColor(const FGraphPin* FromPin) const;

    void OpenContextMenu(const FCursorEvent& CursorEvent);

    TSharedPtr<FGraphModel>                Model;
    TSharedPtr<IFontFace>                  Font;
    TArray<TSharedPtr<FGraphNodeElement>>  NodeElements;
    TArray<int32>                          SelectedNodeIds;
    FGraphNodeStyle                        NodeStyle;
    FFloatColor                            BackgroundColor;
    FFloatColor                            LinkColor;
    FRectangle                             MarqueeBounds;
    Vector2                                Pan;
    Vector2                                DraggingToPosition;
    IntVector2                             DragAnchor;
    IntVector2                             LastDragPosition;
    float                                  Zoom;
    int32                                  GridSpacingInGraphSpace;
    int32                                  SelectedLinkId;
    int32                                  DraggingFromPinId;
    int32                                  HoveredPinId;
    int32                                  HoveredLinkId;
    int32                                  BuiltRevision;
    EGraphDragMode                         DragMode;
    bool                                   bShowGrid;
    bool                                   bIsViewer;
    bool                                   bHasMovedSincePress;
    FOnGetGraphContextMenu                 OnGetContextMenuDelegate;
    FOnGraphSelectionChanged               OnSelectionChangedDelegate;
};
