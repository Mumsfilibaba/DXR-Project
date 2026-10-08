#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Allocators.h"
#include "Application/Elements/VisualElement.h"

class FElementPath
{
public:

    /** @brief How deep a path can go before it spills from the inline storage to the heap. */
    static constexpr int32 InlineCapacity = 24;

    using FElementArray = TArray<TSharedPtr<FVisualElement>, TInlineArrayAllocator<TSharedPtr<FVisualElement>, InlineCapacity>>;

    FElementPath()
        : Filter(EVisibility::Visible)
        , Elements()
    {
    }

    explicit FElementPath(EVisibility InFilter)
        : Filter(InFilter)
        , Elements()
    {
    }

    FElementPath(const FElementPath&) = default;
    FElementPath(FElementPath&&) = default;
    FElementPath& operator=(const FElementPath&) = default;
    FElementPath& operator=(FElementPath&&) = default;

    NODISCARD FORCEINLINE bool AcceptsVisibility(EVisibility InVisibility)           const { return (Filter & InVisibility) != EVisibility::None; }
    NODISCARD FORCEINLINE bool IsEmpty()                                             const { return Elements.IsEmpty(); }
    NODISCARD FORCEINLINE bool Contains(const TSharedPtr<FVisualElement>& InElement) const { return Elements.Contains(InElement); }

    NODISCARD FORCEINLINE int32                LastIndex()   const { return Elements.LastIndex(); }
    NODISCARD FORCEINLINE int32                Size()        const { return Elements.Size(); }
    NODISCARD FORCEINLINE EVisibility          GetFilter()   const { return Filter; }
    NODISCARD FORCEINLINE FElementArray&       GetElements()       { return Elements; }
    NODISCARD FORCEINLINE const FElementArray& GetElements() const { return Elements; }

    NODISCARD FORCEINLINE TSharedPtr<FVisualElement>&       operator[](int32 Index)       { return Elements[Index]; }
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& operator[](int32 Index) const { return Elements[Index]; }

    FORCEINLINE void Reset()                                             { Elements.Clear(); }
    FORCEINLINE void Remove(const TSharedPtr<FVisualElement>& InElement) { Elements.Remove(InElement); }
    FORCEINLINE void RemoveAt(int32 Position)                            { Elements.RemoveAt(Position); }

    void Add(EVisibility InVisibility, const TSharedPtr<FVisualElement>& InElement)
    {
        CHECK(InElement != nullptr);

        if (AcceptsVisibility(InVisibility))
        {
            Elements.Add(InElement);
        }
    }

    void Insert(EVisibility InVisibility, const TSharedPtr<FVisualElement>& InElement, int32 Position)
    {
        CHECK(InElement != nullptr);

        if (AcceptsVisibility(InVisibility))
        {
            Elements.Insert(Position, InElement);
        }
    }

private:
    EVisibility   Filter;
    FElementArray Elements;
};
