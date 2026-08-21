#pragma once
#include "Core/Containers/Array.h"
#include "Application/Elements/VisualElement.h"

class FElementPath
{
public:
    FElementPath()
        : Filter(EVisibility::Visible)
        , Elements()
    {
    }

    FElementPath(EVisibility InFilter)
        : Filter(InFilter)
        , Elements()
    {
    }

    FElementPath(const FElementPath& Other)
        : Filter(Other.Filter)
        , Elements(Other.Elements)
    {
    }

    FElementPath(FElementPath&& Other)
        : Filter(Other.Filter)
        , Elements(Move(Other.Elements))
    {
        Other.Filter = EVisibility::None;
    }

    void Add(EVisibility InVisibility, const TSharedPtr<FVisualElement>& InElement)
    {
        CHECK(InElement != nullptr);

        if (AcceptVisbility(InVisibility))
        {
            Elements.Add(InElement);
        }
    }

    void Insert(EVisibility InVisibility, const TSharedPtr<FVisualElement>& InElement, int32 Position)
    {
        CHECK(InElement != nullptr);

        if (AcceptVisbility(InVisibility))
        {
            Elements.Insert(Position, InElement);
        }
    }

    bool AcceptVisbility(EVisibility Visibility) const
    {
        return (Filter & Visibility) != EVisibility::None;
    }

    FORCEINLINE bool IsEmpty() const
    {
        return Elements.IsEmpty();
    }

    FORCEINLINE bool Contains(const TSharedPtr<FVisualElement>& InElement) const
    {
        return Elements.Contains(InElement);
    }

    FORCEINLINE void Remove(const TSharedPtr<FVisualElement>& InElement)
    {
        Elements.Remove(InElement);
    }

    FORCEINLINE void RemoveAt(int32 Position)
    {
        Elements.RemoveAt(Position);
    }

    FORCEINLINE int32 LastIndex() const
    {
        return Elements.LastIndex();
    }

    FORCEINLINE int32 Size() const
    {
        return Elements.Size();
    }

    EVisibility GetFilter() const
    {
        return Filter;
    }

    TArray<TSharedPtr<FVisualElement>>& GetElements()
    {
        return Elements;
    }

    const TArray<TSharedPtr<FVisualElement>>& GetElements() const
    {
        return Elements;
    }

    FORCEINLINE TSharedPtr<FVisualElement>& operator[](int32 Index)
    {
        return Elements[Index];
    }

    FORCEINLINE const TSharedPtr<FVisualElement>& operator[](int32 Index) const
    {
        return Elements[Index];
    }

    FElementPath& operator=(const FElementPath& Other)
    {
        if (this != AddressOf(Other))
        {
            Filter  = Other.Filter;
            Elements = Other.Elements;
        }

        return *this;
    }

    FElementPath& operator=(FElementPath&& Other)
    {
        if (this != AddressOf(Other))
        {
            Filter       = Other.Filter;
            Other.Filter = EVisibility::None;
            Elements      = Move(Other.Elements);
        }

        return *this;
    }

private:
    EVisibility                        Filter;
    TArray<TSharedPtr<FVisualElement>> Elements;
};
