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
     * @brief Sets the share of the leftover space this slot takes. A coefficient of zero sizes the slot to
     * the child's desired size instead.
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

    /**
     * @brief Gets whether the slot takes a share of the leftover space rather than its child's size.
     *
     * @return True when the fill coefficient is above zero.
     */
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
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Appends a slot for the element and returns it so the caller can set padding and fill.
     *
     * @param InElement The element to place in the new slot.
     * @return The new slot.
     */
    FBoxSlot& AddSlot(const TSharedPtr<FVisualElement>& InElement);

    /** @brief Drops every slot, so the box can be refilled. */
    void ClearSlots();

    /**
     * @brief Drops one slot, moving the ones after it up.
     *
     * @param Index The slot to drop, ignored when out of range.
     */
    void RemoveSlotAt(int32 Index);

    /** @return How many slots the box holds, counting those whose element is null. */
    NODISCARD FORCEINLINE int32 GetNumSlots() const
    {
        return Slots.Size();
    }

    /**
     * @brief Reads one of the slots.
     *
     * @param Index The slot to read, which must be in range.
     * @return The slot at that index.
     */
    NODISCARD FORCEINLINE const FBoxSlot& GetSlot(int32 Index) const
    {
        return Slots[Index];
    }

    /**
     * @brief Reaches a slot so its padding and alignment can be changed after it was added.
     *
     * @param Index The slot to reach, which must be in range.
     * @return The slot at that index.
     */
    NODISCARD FORCEINLINE FBoxSlot& GetSlot(int32 Index)
    {
        return Slots[Index];
    }

protected:
    virtual EChildVisit VisitChildren(FChildVisitor& Visitor, EChildOrder Order) const override;

    NODISCARD static FRectangle ArrangeInSlot(const FRectangle& SlotBounds, const IntVector2& ChildDesiredSize, const FBoxSlot& Slot);

    TArray<FBoxSlot> Slots;
};

template<EOrientation Orientation>
class TStackBox : public FBox
{
public:
    TStackBox()
        : FBox()
        , ArrangedSlots()
    {
    }

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;

protected:
    using FAxis = TLayoutAxis<Orientation>;

    virtual void HitTestChildren(const IntVector2& ClientPosition, FElementPath& OutPath) override;

    struct FArrangedSlot
    {
        int32 Start;
        int32 SlotIndex;
    };

    TArray<FArrangedSlot> ArrangedSlots;
};

class APPLICATION_API FVerticalBox final : public TStackBox<EOrientation::Vertical>
{
public:
    static TSharedPtr<FVerticalBox> Create();

public:
    FVerticalBox();
    virtual ~FVerticalBox();
};

class APPLICATION_API FHorizontalBox final : public TStackBox<EOrientation::Horizontal>
{
public:
    static TSharedPtr<FHorizontalBox> Create();

public:
    FHorizontalBox();
    virtual ~FHorizontalBox();
};

template<EOrientation Orientation>
IntVector2 TStackBox<Orientation>::ComputeDesiredSize() const
{
    int32 MainExtent  = 0;
    int32 CrossExtent = 0;

    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        const IntVector2 ChildSize = Slot.Element->GetCachedDesiredSize();
        MainExtent += FAxis::Main(ChildSize) + FAxis::MainTotal(Slot.Padding);
        CrossExtent = Math::Max(CrossExtent, FAxis::Cross(ChildSize) + FAxis::CrossTotal(Slot.Padding));
    }

    return FAxis::MakeSize(MainExtent, CrossExtent);
}

template<EOrientation Orientation>
void TStackBox<Orientation>::OnArrange(const FRectangle& AllottedBounds)
{
    int32 AutoExtent   = 0;
    int32 NumFillSlots = 0;
    float FillTotal    = 0.0f;

    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        if (Slot.IsFillSlot())
        {
            FillTotal += Slot.FillCoefficient;
            ++NumFillSlots;
        }
        else
        {
            AutoExtent += FAxis::Main(Slot.Element->GetCachedDesiredSize()) + FAxis::MainTotal(Slot.Padding);
        }
    }

    const int32 Remaining   = Math::Max(0, FAxis::MainExtent(AllottedBounds) - AutoExtent);
    int32       Cursor      = FAxis::MainStart(AllottedBounds);
    int32       Distributed = 0;
    int32       FillSeen    = 0;

    ArrangedSlots.Reset();

    for (int32 SlotIndex = 0; SlotIndex < Slots.Size(); ++SlotIndex)
    {
        const FBoxSlot& Slot = Slots[SlotIndex];
        if (!Slot.Element)
        {
            continue;
        }

        const IntVector2 ChildDesiredSize = Slot.Element->GetCachedDesiredSize();

        int32 Extent = 0;
        if (Slot.IsFillSlot())
        {
            Extent = (++FillSeen == NumFillSlots)
                ? Remaining - Distributed
                : static_cast<int32>((static_cast<float>(Remaining) * Slot.FillCoefficient) / FillTotal);

            Distributed += Extent;
        }
        else
        {
            Extent = FAxis::Main(ChildDesiredSize) + FAxis::MainTotal(Slot.Padding);
        }

        ArrangedSlots.Add(FArrangedSlot{ Cursor, SlotIndex });
        Slot.Element->Arrange(ArrangeInSlot(FAxis::MakeSlot(AllottedBounds, Cursor, Extent), ChildDesiredSize, Slot));
        Cursor += Extent;
    }
}

template<EOrientation Orientation>
void TStackBox<Orientation>::HitTestChildren(const IntVector2& ClientPosition, FElementPath& OutPath)
{
    if (HasHitTestOverflow())
    {
        FBox::HitTestChildren(ClientPosition, OutPath);
        return;
    }

    const int32 Coordinate = FAxis::Main(ClientPosition);

    int32 Low  = 0;
    int32 High = ArrangedSlots.Size();
    while (Low < High)
    {
        const int32 Mid = (Low + High) / 2;
        if (ArrangedSlots[Mid].Start <= Coordinate)
        {
            Low = Mid + 1;
        }
        else
        {
            High = Mid;
        }
    }

    if (Low <= 0)
    {
        return;
    }

    const int32 SlotIndex = ArrangedSlots[Low - 1].SlotIndex;
    if (SlotIndex < Slots.Size() && Slots[SlotIndex].Element)
    {
        Slots[SlotIndex].Element->HitTest(ClientPosition, OutPath);
    }
}
