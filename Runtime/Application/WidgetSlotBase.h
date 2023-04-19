#pragma once
#include "Widgets/NullWidget.h"

class FWidget;

class FSlotBase
{
public:
    FSlotBase(const FSlotBase&)            = delete;
    FSlotBase& operator=(const FSlotBase&) = delete;
    FSlotBase(FSlotBase&&)                 = default;
    FSlotBase& operator=(FSlotBase&&)      = default;

    FSlotBase()
        : Widget(FNullWidget::Get())
    {
    }

    void AttachWidget(const TSharedPtr<FWidget>& InWidget)
    {
        Widget = InWidget;
    }

    const TSharedPtr<FWidget>& GetAttachedWidget() const
    {
        return Widget;
    }

    FSlotBase& operator[](const TSharedPtr<FWidget>& InWidget)
    {
        AttachWidget(InWidget);
        return *this;
    }

private:
    TSharedPtr<FWidget> Widget;
};

template<typename SlotType>
class TSlotBase : public FSlotBase
{
public:
    class FSlotInitializer
    {
    public:
        FSlotInitializer(const FSlotInitializer&)            = delete;
        FSlotInitializer& operator=(const FSlotInitializer&) = delete;

        FSlotInitializer(FSlotInitializer&&)            = default;
        FSlotInitializer& operator=(FSlotInitializer&&) = default;

        FSlotInitializer()
            : Widget(FNullWidget::Get())
            , Slot(nullptr)
        {
        }

        FSlotInitializer(TUniquePtr<SlotType>&& InSlot)
            : Widget(FNullWidget::Get())
            , Slot(::Forward<TUniquePtr<SlotType>>(InSlot))
        {
        }

        SlotType* GetSlot() const
        {
            return Slot.Get();
        }

        TUniquePtr<SlotType> RemoveSlotOwnership()
        {
            return ::Move(Slot);
        }

        void AttachWidget(const TSharedPtr<FWidget>& InWidget)
        {
            Widget = InWidget;
        }

        typename SlotType::FSlotInitializer& GetThis()
        {
            return static_cast<SlotType::FSlotInitializer&>(*this);
        }

        const TSharedPtr<FWidget>& GetAttachedWidget() const
        {
            return Widget;
        }

        typename SlotType::FSlotInitializer& operator[](const TSharedPtr<FWidget>& InWidget)
        {
            Widget = InWidget;
            return GetThis();
        }

    private:
        TSharedPtr<FWidget>  Widget;
        TUniquePtr<SlotType> Slot;
    };

public:
    using FSlotBase::FSlotBase;
    using FSlotBase::GetAttachedWidget;
    using FSlotBase::AttachWidget;

    void Initialize(const FSlotInitializer& Initializer)
    {
        if (Initializer.GetAttachedWidget())
        {
            AttachWidget(Initializer.GetAttachedWidget());
        }
    }

    SlotType& operator[](const TSharedPtr<FWidget>& InWidget)
    {
        this->AttachWidget(InWidget);
        return static_cast<SlotType&>(*this);
    }
};