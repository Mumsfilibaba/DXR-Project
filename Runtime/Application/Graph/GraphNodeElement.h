#pragma once
#include "Application/Elements/CompoundElement.h"
#include "Application/Graph/GraphTypes.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FGraphNodeElement final : public FCompoundElement
{
public:

    /** @brief The height of the title bar, in graph space. */
    static constexpr int32 TitleHeight = 24;

    /** @brief The height of one pin row, in graph space. */
    static constexpr int32 PinRowHeight = 20;

    /** @brief The radius of the circle a pin is drawn as, in graph space. */
    static constexpr int32 PinRadius = 5;

    /** @brief The narrowest a node is allowed to measure, in graph space. */
    static constexpr int32 MinimumWidth = 140;

public:

    /**
     * @brief Creates the element for a node.
     *
     * @param Node  The node to show, which is copied for its labels and pins.
     * @param InFont The face labels are drawn with.
     * @return The new element.
     */
    static TSharedPtr<FGraphNodeElement> Create(const FGraphNode& Node, const TSharedPtr<IFontFace>& InFont);

public:
    FGraphNodeElement();
    virtual ~FGraphNodeElement();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Sets the scale the node is drawn at, which the canvas pushes in before arranging it.
     *
     * @param InZoom The scale, where one is graph space.
     */
    void SetZoom(float InZoom);

    /**
     * @brief Sets what the node is drawn with, which the canvas pushes in as it builds the element.
     *
     * @param InStyle The colors, the corner radius and the pin arrangement.
     */
    void SetStyle(const FGraphNodeStyle& InStyle);

    /** @return What the node is drawn with, which is the canvas' style until one is pushed in. */
    NODISCARD FORCEINLINE const FGraphNodeStyle& GetStyle() const
    {
        return Style;
    }

    /**
     * @brief Marks the node as one of the selected ones, which draws it with an accent border.
     *
     * @param bInIsSelected True while the node is selected.
     */
    void SetSelected(bool bInIsSelected);

    /**
     * @brief Marks a pin as the one under the cursor, which draws it larger.
     *
     * @param InPinId The pin under the cursor, or -1 for none.
     */
    void SetHoveredPin(int32 InPinId);

    /**
     * @brief Where a pin's link should start or end, in the client space the node was arranged in.
     *
     * @param PinId       The pin to place.
     * @param OutPosition Receives the centre of the pin.
     * @return True when the pin belongs to this node.
     */
    NODISCARD bool GetPinCenter(int32 PinId, Vector2& OutPosition) const;

    /**
     * @brief The pin whose circle covers a point, which is what starts a link drag.
     *
     * @param ClientPosition The position to test.
     * @return The pin id, or -1 when the point is not on a pin.
     */
    NODISCARD int32 FindPinAt(const IntVector2& ClientPosition) const;

    /** @return The id the model handed out for the node this element stands for. */
    NODISCARD FORCEINLINE int32 GetNodeId() const
    {
        return Node.NodeId;
    }

    /**
     * @brief Gets the node this element stands for. It is a copy taken when the canvas last built from
     * the model, so an edit made since does not show until the canvas builds again.
     *
     * @return The node, with its title, its pins and its graph-space position.
     */
    NODISCARD FORCEINLINE const FGraphNode& GetNode() const
    {
        return Node;
    }

    /** @return True while the node is one of the selected ones, which draws it with an accent border. */
    NODISCARD FORCEINLINE bool IsSelected() const
    {
        return bIsSelected;
    }

private:
    void DrawPinSeparator(const FRectangle& Bounds, FDrawCommandList& OutCommandList, int32 LayerId, const FFloatColor& Color) const;

    NODISCARD FRectangle GetTitleBounds(const FRectangle& Bounds) const;
    NODISCARD int32 Scaled(int32 GraphSpaceLength) const;
    NODISCARD int32 CountPinRows() const;
    NODISCARD int32 CountPins(EGraphPinDirection Direction) const;

    FGraphNode            Node;
    FGraphNodeStyle       Style;
    TSharedPtr<IFontFace> Font;
    TArray<Vector2>       PinCenters;
    float                 Zoom;
    int32                 HoveredPinId;
    bool                  bIsSelected;
};
