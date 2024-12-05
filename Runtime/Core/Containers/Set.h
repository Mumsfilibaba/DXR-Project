#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/TypeHash.h"

// TODO: Custom set implementation
#include <unordered_set>

template<typename InElementType>
class TSet
{
    template<typename SetType, typename ElementType>
    friend class TSetIterator;

public:
    typedef InElementType ElementType;

private:
    struct FHasher
    {
        size_t operator()(const ElementType& Value) const
        {
            return static_cast<size_t>(GetHashForType(Value));
        }
    };

public:
    typedef std::unordered_set<ElementType, FHasher>    BaseSetType;
    typedef TSetIterator<TSet, ElementType>             IteratorType;
    typedef TSetIterator<const TSet, const ElementType> ConstIteratorType;

    typedef int32 SizeType;

public:

    /** @brief Default constructor */
    TSet() = default;

    /** @brief Copy constructor */
    TSet(const TSet& Other)
        : BaseSet(Other.BaseSet)
    {
    }
    
    /** @brief Move constructor */
    TSet(TSet&& Other)
        : BaseSet(Move(Other.BaseSet))
    {
    }
    
    TSet(std::initializer_list<ElementType> InitializerList)
        : BaseSet(InitializerList)
    {
    }

    void Append(const TSet& Other)
    {
        BaseSet.insert(Other.BaseSet.begin(), Other.BaseSet.end());
    }

    const ElementType& Add(const ElementType& InElement)
    {
        typename BaseSetType::iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            return *Element;
        }
        else
        {
            auto InsertedElement = BaseSet.insert(InElement);
            return *InsertedElement.first;
        }
    }

    const ElementType& Add(ElementType&& InElement)
    {
        typename BaseSetType::iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            return *Element;
        }
        else
        {
            auto InsertedElement = BaseSet.emplace(Forward<ElementType>(InElement));
            return *InsertedElement.first;
        }
    }

    const ElementType& Add(const ElementType& InElement, bool* OutAlreadyInSet)
    {
        typename BaseSetType::iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            if (OutAlreadyInSet)
            {
                *OutAlreadyInSet = true;
            }

            return *Element;
        }
        else
        {
            if (OutAlreadyInSet)
            {
                *OutAlreadyInSet = false;
            }

            auto InsertedElement = BaseSet.insert(InElement);
            return *InsertedElement.first;
        }
    }

    const ElementType& Add(ElementType&& InElement, bool* OutAlreadyInSet)
    {
        typename BaseSetType::iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            if (OutAlreadyInSet)
            {
                *OutAlreadyInSet = true;
            }

            return *Element;
        }
        else
        {
            if (OutAlreadyInSet)
            {
                *OutAlreadyInSet = false;
            }

            auto InsertedElement = BaseSet.emplace(Forward<ElementType>(InElement));
            return *InsertedElement.first;
        }
    }

    template<typename... ArgTypes>
    const ElementType& Emplace(ArgTypes&&... Args)
    {
        ElementType NewElement(Forward<ArgTypes>(Args)...);
        
        typename BaseSetType::iterator Element = BaseSet.find(NewElement);
        if (Element != BaseSet.end())
        {
            return *Element;
        }
        else
        {
            auto InsertedElement = BaseSet.emplace(Move(NewElement));
            return *InsertedElement.first;
        }
    }

    template<typename... ArgTypes>
    const ElementType& Emplace(bool* OutAlreadyInSet, ArgTypes&&... Args)
    {
        ElementType NewElement(Forward<ArgTypes>(Args)...);
        
        typename BaseSetType::iterator Element = BaseSet.find(NewElement);
        if (Element != BaseSet.end())
        {
            if (OutAlreadyInSet)
            {
                *OutAlreadyInSet = true;
            }

            return *Element;
        }
        else
        {
            if (OutAlreadyInSet)
            {
                *OutAlreadyInSet = false;
            }

            auto InsertedElement = BaseSet.emplace(Move(NewElement));
            return *InsertedElement.first;
        }
    }

    NODISCARD const ElementType* Find(const ElementType& InElement) const
    {
        typename BaseSetType::const_iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            return &(*Element);
        }
        else
        {
            return nullptr;
        }
    }

    NODISCARD const ElementType& FindOrAdd(const ElementType& InElement)
    {
        typename BaseSetType::iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            return *Element;
        }
        else
        {
            auto InsertedElement = BaseSet.insert(InElement);
            return *InsertedElement.first;
        }
    }

    NODISCARD const ElementType& FindOrAdd(ElementType&& InElement)
    {
        typename BaseSetType::iterator Element = BaseSet.find(InElement);
        if (Element != BaseSet.end())
        {
            return *Element;
        }
        else
        {
            auto InsertedElement = BaseSet.emplace(Move(InElement));
            return *InsertedElement.first;
        }
    }

    void Remove(const ElementType& InElement)
    {
        BaseSet.erase(InElement);
    }

    NODISCARD bool Contains(const ElementType& InElement) const
    {
        typename BaseSetType::const_iterator Element = BaseSet.find(InElement);
        return Element != BaseSet.end();
    }

    void Reserve(SizeType InCapacity)
    {
        BaseSet.reserve(InCapacity);
    }
    
    void Clear()
    {
        BaseSet.clear();
    }

    NODISCARD bool IsEmpty() const
    {
        return BaseSet.empty();
    }

    NODISCARD SizeType Size() const
    {
        return static_cast<SizeType>(BaseSet.size());
    }
    
    NODISCARD SizeType Capacity() const
    {
        return static_cast<SizeType>(BaseSet.max_size());
    }

    NODISCARD TArray<ElementType> GetValues() const
    {
        TArray<ElementType> Values;
        Values.Reserve(BaseSet.size());

        for (const ElementType& Element : BaseSet)
        {
            Values.Add(Element);
        }

        return Values;
    }

public:

    // Iterators
    NODISCARD IteratorType CreateIterator()
    {
        return TSetIterator<TSet, ElementType>(*this, BaseSet.begin());
    }

    NODISCARD ConstIteratorType CreateIterator() const
    {
        return TSetIterator<const TSet, const ElementType>(*this, BaseSet.begin());
    }

    NODISCARD ConstIteratorType CreateConstIterator() const
    {
        return TSetIterator<const TSet, const ElementType>(*this, BaseSet.begin());
    }

public:

    // STL Iterators
    NODISCARD FORCEINLINE IteratorType      begin()       { return TSetIterator<TSet, ElementType>(*this, BaseSet.begin()); }
    NODISCARD FORCEINLINE ConstIteratorType begin() const { return TSetIterator<const TSet, const ElementType>(*this, BaseSet.begin()); }
    
    NODISCARD FORCEINLINE IteratorType      end()       { return TSetIterator<TSet, ElementType>(*this, BaseSet.end()); }
    NODISCARD FORCEINLINE ConstIteratorType end() const { return TSetIterator<const TSet, const ElementType>(*this, BaseSet.end()); }
    
public:
    TSet& operator=(const TSet& Other)
    {
        BaseSet = Other.BaseSet;
        return *this;
    }

    TSet& operator=(TSet&& Other)
    {
        BaseSet = Move(Other.BaseSet);
        return *this;
    }

    NODISCARD bool operator==(const TSet& Other) const
    {
        return BaseSet == Other.BaseSet;
    }

    NODISCARD bool operator!=(const TSet& Other) const
    {
        return BaseSet != Other.BaseSet;
    }

private:
    BaseSetType BaseSet;
};
