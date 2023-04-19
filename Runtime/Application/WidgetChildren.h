#pragma once
#include "WidgetSlotBase.h"
#include "Core/Containers/Array.h"
#include "Application/Widgets/NullWidget.h"

class FWidgetChildren
{
public:
    FWidgetChildren(FWidget* InOwner)
        : Owner(InOwner)
    {
    }

    virtual ~FWidgetChildren() = default;

    virtual const FSlotBase& GetSlotAt(uint32 Index) const = 0;

    virtual TSharedPtr<FWidget> GetWidgetAt(uint32 Index) const = 0;

    virtual uint32 NumWidgets() const = 0;

    FWidget* GetOwner() const
    {
        return Owner;
    }

private:
    FWidget* Owner;
};


template<typename SlotType>
class TSingleWidgetChildrenSlot : public FWidgetChildren, public TSlotBase<SlotType>
{
public:
    using TSlotBase<SlotType>::GetAttachedWidget;

    TSingleWidgetChildrenSlot(FWidget* InOwner)
        : FWidgetChildren(InOwner)
        , TSlotBase<SlotType>()
    {
    }

    virtual const FSlotBase& GetSlotAt(uint32 Index) const override final
    {
        CHECK(Index == 0);
        return *this;
    }

    virtual TSharedPtr<FWidget> GetWidgetAt(uint32 Index) const override final
    {
        CHECK(Index == 0);
        return GetAttachedWidget();
    }

    virtual uint32 NumWidgets() const override final
    {
        return 1;
    }
};


template<typename SlotType>
class TWidgetChildrenSlots : public FWidgetChildren
{
public:
    TWidgetChildrenSlots(FWidget* InOwner)
        : FWidgetChildren(InOwner)
        , Slots()
    {
    }

    void AddSlot(const typename SlotType::FSlotInitializer& Initializer)
    {
        TUniquePtr<SlotType> NewSlot = MakeUnique<SlotType>();
        NewSlot->Initialize(Initializer);
        Slots.Add(::Move(NewSlot));
    }

    void AddSlots(const TArray<typename SlotType::FSlotInitializer>& SlotInitializers)
    {
        Slots.Reserve(Slots.Size() + SlotInitializers.Size());
        for (const typename SlotType::FSlotInitializer& Initializer : SlotInitializers)
        {
            AddSlot(Initializer);
        }
    }

    void InsertSlot(const typename SlotType::FSlotInitializer& Initializer, int32 Position)
    {
        CHECK(Position < NumWidgets());

        TUniquePtr<SlotType> NewSlot = MakeUnique<SlotType>();
        NewSlot->Initialize(Initializer);
        Slots.Insert(Position, ::Move(NewSlot));
    }

    void Remove(const TSharedPtr<FWidget>& InWidget)
    {
        for (int32 Index = 0; Index < Slots.Size(); ++Index)
        {
            if (Slots[Index]->GetAttachedWidget() == InWidget)
            {
                Slots.RemoveAt(Index);
            }
        }
    }
    
    void RemoveAt(uint32 Index)
    {
        CHECK(Index < NumWidgets());
        Slots.RemoveAt(Index);
    }

    virtual const FSlotBase& GetSlotAt(uint32 Index) const override final
    {
        CHECK(Index < NumWidgets());
        return *Slots[Index];
    }

    virtual TSharedPtr<FWidget> GetWidgetAt(uint32 Index) const override final
    {
        CHECK(Index < NumWidgets());
        return Slots[Index]->GetAttachedWidget();
    }

    virtual uint32 NumWidgets() const override final
    {
        return static_cast<uint32>(Slots.Size());
    }

private:
    TArray<TUniquePtr<SlotType>> Slots;
};


class FEmptyWidgetChildren : public FWidgetChildren
{
public:
    FEmptyWidgetChildren(FWidget* InOwner)
        : FWidgetChildren(InOwner)
    {
    }

    virtual const FSlotBase& GetSlotAt(uint32 Index) const
    {
        CHECK(false);
        static FSlotBase EmptySlot;
        return EmptySlot;
    }

    virtual TSharedPtr<FWidget> GetWidgetAt(uint32 Index) const
    {
        CHECK(false);
        return FNullWidget::Get();
    }

    virtual uint32 NumWidgets() const
    {
        return 0;
    }
};


enum EVisibility
{
    Visibility_None    = 0,
    Visibility_Hidden  = BIT(1),
    Visibility_Visible = BIT(2),
    Visibility_All     = Visibility_Hidden | Visibility_Visible
};

class FFilteredWidgets
{
public:
    FFilteredWidgets()
        : Children()
        , Filter{ Visibility_Visible }
    {
    }

    FFilteredWidgets(EVisibility InFilter)
        : Children()
        , Filter{ InFilter }
    {
    }

    void Add(EVisibility Visibility, const TSharedPtr<FWidget>& InWidget)
    {
        if (Accepts(Visibility))
        {
            Children.Add(InWidget);
        }
    }

    void Insert(EVisibility Visibility, const TSharedPtr<FWidget>& InWidget, int32 Position)
    {
        if (Accepts(Visibility))
        {
            Children.Insert(Position, InWidget);
        }
    }

    void Remove(const TSharedPtr<FWidget>& InWidget)
    {
        Children.Remove(InWidget);
    }

    bool Accepts(EVisibility InVisibility) const
    {
        return (Filter & InVisibility) != Visibility_None;
    }

    bool Contains(const TSharedPtr<FWidget>& InWidget) const
    {
		for (const TSharedPtr<FWidget>& Widget : Children)
		{
            if (InWidget == Widget)
            {
                return true;
            }
		}

        return false;
    }

    void Reverse()
    {
        Children.Reverse();
    }

    uint32 NumChildren() const
    {
        const uint32 Result = static_cast<uint32>(Children.Size());
        return Result;
    }

    uint32 LastIndex() const
    {
        const uint32 Result = static_cast<uint32>(Children.LastElementIndex());
        return Result;
    }

    EVisibility GetFilter() const
    {
        return Filter;
    }

    TArray<TSharedPtr<FWidget>>& GetArray()
    {
        return Children;
    }

    const TArray<TSharedPtr<FWidget>>& GetArray() const
    {
        return Children;
    }

    TSharedPtr<FWidget>& operator[](int32 Index)
    {
        return Children[Index];
    }

    const TSharedPtr<FWidget>& operator[](int32 Index) const
    {
        return Children[Index];
    }

private:
    TArray<TSharedPtr<FWidget>> Children;
    EVisibility                 Filter;
};