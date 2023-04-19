#pragma once
#include "Application/Widget.h"

template<typename SlotType>
class FScopedSlotInitilizer : public SlotType::FSlotInitializer
{
public:
    FScopedSlotInitilizer(const FScopedSlotInitilizer&)            = delete;
    FScopedSlotInitilizer& operator=(const FScopedSlotInitilizer&) = delete;

    FScopedSlotInitilizer(FScopedSlotInitilizer&&)            = default;
    FScopedSlotInitilizer& operator=(FScopedSlotInitilizer&&) = default;

    FScopedSlotInitilizer(TUniquePtr<SlotType>&& InSlot, TWidgetChildrenSlots<SlotType>& InChildren)
        : SlotType::FSlotInitializer(::Forward<TUniquePtr<SlotType>>(InSlot))
        , Children(InChildren)
    {
    }

    ~FScopedSlotInitilizer()
    {
        if (SlotType* Slot = this->GetSlot())
        {
            Children.AddSlot(*this);
        }
    }

private:
    TWidgetChildrenSlots<SlotType>& Children;
};

class APPLICATION_API FOverlay : public FWidget
{
    DECLARE_WIDGET(FOverlay, FWidget);

public:
    FSLOTINITIALIZER_START(FOverlaySlot)
    FSLOTINITIALIZER_END();

    static FOverlaySlot::FSlotInitializer Slot()
    {
        return FOverlaySlot::FSlotInitializer(MakeUnique<FOverlaySlot>());
    }

public:
    FINITIALIZER_START(FOverlay)
        FINITIALIZER_SLOT_ATTRIBUTE(FOverlaySlot, Slots);
    FINITIALIZER_END();

    FOverlay()
        : FWidget()
        , Children(this)
    {
    }

    void Initialize(const FInitializer& Initializer)
    {
        Children.AddSlots(::Move(Initializer.Slots));
    }

    virtual void Paint(const FRectangle& AssignedBounds) override;

    using FScopedSlotInitilizer = FScopedSlotInitilizer<FOverlaySlot>;
    FScopedSlotInitilizer AddSlot();
    
    void RemoveWidget(const TSharedPtr<FWidget>& InWidget)
    {
        Children.Remove(InWidget);
    }

    virtual FWidgetChildren* GetChildren() override
    {
        return &Children;
    }

private:
    TWidgetChildrenSlots<FOverlaySlot> Children;
};