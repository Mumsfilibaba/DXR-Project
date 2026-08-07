#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

class ENGINE_API FActorFilter
{
public:

    /**
     * @return Returns the name the filter is presented under in the scene-hierarchy
     */
    const String& GetName() const
    {
        return Name;
    }

    /**
     * @brief Set the name the filter is presented under in the scene-hierarchy
     *
     * @param InName New name of the filter
     */
    void SetName(const String& InName)
    {
        Name = InName;
    }

    /**
     * @brief Retrieve the filter this filter is nested inside
     *
     * @return Returns the parent of the filter, or nullptr if the filter sits at the root of the hierarchy
     */
    FActorFilter* GetParentFilter() const
    {
        return ParentFilter;
    }

    /**
     * @return Returns every filter nested directly inside this filter, in creation order
     */
    const TArray<FActorFilter*>& GetChildFilters() const
    {
        return ChildFilters;
    }

    /**
     * @brief Check if this filter is nested anywhere below another filter
     *
     * @param PossibleParent Filter to look for among the ancestors of this filter
     * @return Returns true if the filter is a descendant of the specified filter
     */
    bool IsDescendantOf(const FActorFilter* PossibleParent) const
    {
        for (const FActorFilter* Current = ParentFilter; Current; Current = Current->ParentFilter)
        {
            if (Current == PossibleParent)
            {
                return true;
            }
        }

        return false;
    }

private:
    friend class FWorld;

    explicit FActorFilter(const String& InName)
        : Name(InName)
        , ParentFilter(nullptr)
        , ChildFilters()
    { }

    ~FActorFilter() = default;

    String                Name;
    FActorFilter*         ParentFilter;
    TArray<FActorFilter*> ChildFilters;
};
