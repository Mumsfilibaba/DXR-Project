#pragma once
#include "Application/Elements/VisualElement.h"
#include "Application/Layout/LayoutTypes.h"

struct FOverlaySlot
{
    FOverlaySlot()
        : Element(nullptr)
        , Padding()
        , HorizontalAlignment(EHorizontalAlignment::Fill)
        , VerticalAlignment(EVerticalAlignment::Fill)
    {
    }

    /**
     * @brief Sets how the layer is placed on the horizontal axis.
     *
     * @param InAlignment The alignment to use.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FOverlaySlot& SetHorizontalAlignment(EHorizontalAlignment InAlignment)
    {
        HorizontalAlignment = InAlignment;
        return *this;
    }

    /**
     * @brief Sets how the layer is placed on the vertical axis.
     *
     * @param InAlignment The alignment to use.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FOverlaySlot& SetVerticalAlignment(EVerticalAlignment InAlignment)
    {
        VerticalAlignment = InAlignment;
        return *this;
    }

    /**
     * @brief Sets the space held clear around the layer.
     *
     * @param InPadding The padding to use.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FOverlaySlot& SetPadding(const FMargin& InPadding)
    {
        Padding = InPadding;
        return *this;
    }

    TSharedPtr<FVisualElement> Element;
    FMargin                    Padding;
    EHorizontalAlignment       HorizontalAlignment;
    EVerticalAlignment         VerticalAlignment;
};

class APPLICATION_API FOverlay final : public FVisualElement
{
public:
    static TSharedPtr<FOverlay> Create();

public:
    FOverlay();
    virtual ~FOverlay();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;

    /**
     * @brief Adds a layer above every layer already there.
     *
     * @param InElement The element to add.
     * @return The slot, so its alignment and padding can be set.
     */
    FOverlaySlot& AddSlot(const TSharedPtr<FVisualElement>& InElement);

    /**
     * @brief Removes a layer, which does nothing when the element is not in the overlay.
     *
     * @param InElement The element to remove.
     * @return True when a layer was removed.
     */
    bool RemoveSlot(const TSharedPtr<FVisualElement>& InElement);

    /** @brief Removes every layer. */
    void ClearSlots();

    /** @return The layers the overlay stacks, in the order they draw, so the last one is on top. */
    NODISCARD FORCEINLINE const TArray<FOverlaySlot>& GetSlots() const
    {
        return Slots;
    }

private:
    TArray<FOverlaySlot> Slots;
};
