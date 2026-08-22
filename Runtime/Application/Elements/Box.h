#pragma once
#include "Core/Containers/Array.h"
#include "Application/Elements/VisualElement.h"

struct FBoxSlot
{
    FBoxSlot()
        : Element(nullptr)
        , Padding()
        , FillCoefficient(0.0f)
        , HorizontalAlignment(EHorizontalAlignment::Fill)
        , VerticalAlignment(EVerticalAlignment::Fill)
    {
    }

    /**
     * @brief Sets the space reserved around the child.
     *
     * @param InPadding The padding to apply.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FBoxSlot& SetPadding(const FMargin& InPadding)
    {
        Padding = InPadding;
        return *this;
    }

    /**
     * @brief Sets the share of the leftover space this slot takes.
     *
     * A coefficient of zero sizes the slot to the child's desired size instead.
     *
     * @param InFillCoefficient The share, relative to the other filling slots.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FBoxSlot& SetFillCoefficient(float InFillCoefficient)
    {
        FillCoefficient = InFillCoefficient;
        return *this;
    }

    /**
     * @brief Sets how the child is placed horizontally inside the slot.
     *
     * @param InAlignment The alignment to apply.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FBoxSlot& SetHorizontalAlignment(EHorizontalAlignment InAlignment)
    {
        HorizontalAlignment = InAlignment;
        return *this;
    }

    /**
     * @brief Sets how the child is placed vertically inside the slot.
     *
     * @param InAlignment The alignment to apply.
     * @return This slot, so the setters can be chained.
     */
    FORCEINLINE FBoxSlot& SetVerticalAlignment(EVerticalAlignment InAlignment)
    {
        VerticalAlignment = InAlignment;
        return *this;
    }

    /** @brief True when this slot takes a share of the leftover space. */
    NODISCARD FORCEINLINE bool IsFillSlot() const
    {
        return FillCoefficient > 0.0f;
    }

    TSharedPtr<FVisualElement> Element;
    FMargin                    Padding;
    float                      FillCoefficient;
    EHorizontalAlignment       HorizontalAlignment;
    EVerticalAlignment         VerticalAlignment;
};

class APPLICATION_API FBox : public FVisualElement
{
public:
    FBox();
    virtual ~FBox();

    // FVisualElement Interface
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;

    /**
     * @brief Appends a slot for the element and returns it so the caller can set padding and fill.
     *
     * @param InElement The element to place in the new slot.
     * @return The new slot.
     */
    FBoxSlot& AddSlot(const TSharedPtr<FVisualElement>& InElement);

    /** @brief Drops every slot, so the box can be refilled. */
    void ClearSlots();

    /** @brief The number of slots in the box. */
    NODISCARD FORCEINLINE int32 GetNumSlots() const
    {
        return Slots.Size();
    }

    NODISCARD FORCEINLINE const FBoxSlot& GetSlot(int32 Index) const
    {
        return Slots[Index];
    }

protected:
    NODISCARD static FRectangle ArrangeInSlot(const FRectangle& SlotBounds, const IntVector2& ChildDesiredSize, const FBoxSlot& Slot);

    NODISCARD int32 CountFillSlots() const;

    TArray<FBoxSlot> Slots;
};

class APPLICATION_API FVerticalBox final : public FBox
{
public:
    static TSharedPtr<FVerticalBox> Create();

public:
    FVerticalBox();
    virtual ~FVerticalBox();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
};

class APPLICATION_API FHorizontalBox final : public FBox
{
public:
    static TSharedPtr<FHorizontalBox> Create();

public:
    FHorizontalBox();
    virtual ~FHorizontalBox();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
};
