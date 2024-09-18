#pragma once
#include "Core/Containers/Array.h"
#include "Application/Widgets/Widget.h"

class FWidgetPath
{
public:
    FWidgetPath()
        : Filter(EVisibility::Visible)
        , Widgets()
    {
    }

    FWidgetPath(EVisibility InFilter)
        : Filter(InFilter)
        , Widgets()
    {
    }

    void Add(EVisibility InVisibility, const TSharedPtr<FWidget>& InWidget)
    {
        CHECK(InWidget != nullptr);

        if (AcceptVisbility(InVisibility))
        {
            Widgets.Add(InWidget);
        }
    }

    void Insert(EVisibility InVisibility, const TSharedPtr<FWidget>& InWidget, int32 Position)
    {
        CHECK(InWidget != nullptr);

        if (AcceptVisbility(InVisibility))
        {
            Widgets.Insert(Position, InWidget);
        }
    }

    bool AcceptVisbility(EVisibility Visibility) const
    {
        return (Filter & Visibility) != EVisibility::None;
    }

    FORCEINLINE bool IsEmpty() const
    {
        return Widgets.IsEmpty();
    }

    FORCEINLINE bool Contains(const TSharedPtr<FWidget>& InWidget) const
    {
        return Widgets.Contains(InWidget);
    }

    FORCEINLINE void Remove(const TSharedPtr<FWidget>& InWidget)
    {
        Widgets.Remove(InWidget);
    }

    FORCEINLINE void RemoveAt(int32 Position)
    {
        Widgets.RemoveAt(Position);
    }

    FORCEINLINE int32 LastIndex() const
    {
        return Widgets.LastElementIndex();
    }

    FORCEINLINE int32 Size() const
    {
        return Widgets.Size();
    }

    EVisibility GetFilter() const
    {
        return Filter;
    }

    TArray<TSharedPtr<FWidget>>& GetWidgets()
    {
        return Widgets;
    }

    const TArray<TSharedPtr<FWidget>>& GetWidgets() const
    {
        return Widgets;
    }

    FORCEINLINE TSharedPtr<FWidget>& operator[](int32 Index)
    {
        return Widgets[Index];
    }

    FORCEINLINE const TSharedPtr<FWidget>& operator[](int32 Index) const
    {
        return Widgets[Index];
    }

private:
    EVisibility                 Filter;
    TArray<TSharedPtr<FWidget>> Widgets;
};