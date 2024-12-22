#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

template<typename ElementType>
class TOptional
{
public:

    template<typename OtherType>
    friend class TOptional;

    /** @brief Default constructor */
    TOptional()
        : Value()
        , bHasValue(false)
    {
    }

    /**
     * @brief Construct from nullptr
     */
    FORCEINLINE TOptional(NULLPTR_TYPE)
        : Value()
        , bHasValue(false)
    {
    }

    /**
     * @brief Copy-constructor
     * @param Other Optional to copy from
     */
    FORCEINLINE TOptional(const TOptional& Other)
        : Value()
        , bHasValue(false)
    {
        CopyFrom(Other);
    }

    /**
     * @brief Copy-constructor that takes another type
     * @param Other Optional to copy from
     */
    template<typename OtherType>
    FORCEINLINE TOptional(const TOptional<OtherType>& Other) 
        requires(TIsConstructible<ElementType, const typename TRemoveReference<OtherType>::Type&>::Value)
        : Value()
        , bHasValue(false)
    {
        CopyFrom(Other);
    }

    /**
     * @brief Move-constructor
     * @param Other Optional to move from
     */
    FORCEINLINE TOptional(TOptional&& Other)
        : Value()
        , bHasValue(false)
    {
        MoveFrom(Move(Other));
    }

    /**
     * @brief Move-constructor taking another type
     * @param Other Optional to move from
     */
    template<typename OtherType>
    FORCEINLINE TOptional(TOptional<OtherType>&& Other) 
        requires(TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value)
        : Value()
        , bHasValue(false)
    {
        MoveFrom(Move(Other));
    }

    /**
     * @brief Constructor that creates a value in-place
     * @param Args Arguments for the element to create
     */
    template<typename... ArgTypes>
    FORCEINLINE explicit TOptional(EInPlace, ArgTypes&&... Args)
        : Value()
        , bHasValue(false)
    {
        Construct(Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief Destructor
     */
    FORCEINLINE ~TOptional()
    {
        Reset();
    }

    /**
     * @brief Check if optional has a value assigned
     * @return Returns true if there is a value assigned
     */
    NODISCARD FORCEINLINE bool HasValue() const
    {
        return bHasValue;
    }

    /**
     * @brief Emplace constructs a new element in the optional and destructs any old value
     * @param Args Arguments for constructor
     * @return Returns a reference to the newly constructed value
     */
    template<typename... ArgTypes>
    FORCEINLINE ElementType& Emplace(ArgTypes&&... Args)
    {
        Reset();
        Construct(Forward<ArgTypes>(Args)...);
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief Resets the Optional and destructs any existing element
     */
    FORCEINLINE void Reset()
    {
        if (HasValue())
        {
            Destruct();
        }
    }

    /**
     * @brief Swap between this optional and another optional
     * @param Other Optional to swap with
     */
    FORCEINLINE void Swap(TOptional& Other)
    {
        if (HasValue() && Other.HasValue())
        {
            ::Swap(GetValue(), Other.GetValue());
        }
        else if (HasValue())
        {
            Other.Construct(Move(GetValue()));
            Destruct();
        }
        else if (Other.HasValue())
        {
            Construct(Move(Other.GetValue()));
            Other.Destruct();
        }
    }

    /**
     * @brief Retrieve the optional value
     * @return Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE ElementType& GetValue()
    {
        CHECK(HasValue());
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief Retrieve the optional value
     * @return Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE const ElementType& GetValue() const
    {
        CHECK(HasValue());
        return *reinterpret_cast<const ElementType*>(Value.Data);
    }

    /**
     * @brief Try and retrieve the optional value, if no value is stored, it returns nullptr
     * @return Returns a pointer to the stored value, or nullptr if no value is held
     */
    NODISCARD FORCEINLINE ElementType* TryGetValue()
    {
        return HasValue() ? reinterpret_cast<ElementType*>(Value.Data) : nullptr;
    }

    /**
     * @brief Try and retrieve the optional value, if no value is stored, it returns nullptr
     * @return Returns a pointer to the stored value, or nullptr if no value is held
     */
    NODISCARD FORCEINLINE const ElementType* TryGetValue() const
    {
        return HasValue() ? reinterpret_cast<const ElementType*>(Value.Data) : nullptr;
    }

    /**
     * @brief Retrieve the optional value
     * @param Default Default value to return if a value is not set
     * @return Returns a reference to the stored value
     */
    template<typename DefaultType>
    NODISCARD FORCEINLINE const ElementType& GetValueOrDefault(const DefaultType& Default) const
    {
        return HasValue() ? *reinterpret_cast<const ElementType*>(Value.Data) : static_cast<const ElementType&>(Default);
    }

    /**
     * @brief Retrieve the optional value or a default
     * @param Default Default value to return if a value is not set
     * @return Returns the stored value or the default
     */
    template<typename DefaultType>
    NODISCARD FORCEINLINE ElementType GetValueOrDefault(DefaultType&& Default) const
    {
        return HasValue() ? GetValue() : static_cast<ElementType>(Forward<DefaultType>(Default));
    }

public:

    /**
     * @brief Nullptr assignment operator
     * @return Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(NULLPTR_TYPE)
    {
        Reset();
        return *this;
    }

    /**
     * @brief Copy-assignment operator
     * @param Other Optional to copy from
     * @return Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(const TOptional& Other)
    {
        if (this != &Other)
        {
            if (HasValue() && Other.HasValue())
            {
                GetValue() = Other.GetValue();
            }
            else if (Other.HasValue())
            {
                Construct(Other.GetValue());
            }
            else
            {
                Reset();
            }
        }

        return *this;
    }

    /**
     * @brief Copy-assignment operator that takes another type
     * @param Other Optional to copy from
     * @return Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE TOptional& operator=(const TOptional<OtherType>& Other) 
        requires(TIsConstructible<ElementType, typename TAddLValueReference<typename TRemoveReference<OtherType>::Type>::Type>::Value)
    {
        if (Other.HasValue())
        {
            if (HasValue())
            {
                GetValue() = Other.GetValue();
            }
            else
            {
                Construct(Other.GetValue());
            }
        }
        else
        {
            Reset();
        }

        return *this;
    }

    /**
     * @brief Move-assignment operator
     * @param Other Optional to move from
     * @return Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(TOptional&& Other)
    {
        if (this != &Other)
        {
            if (HasValue() && Other.HasValue())
            {
                GetValue() = Move(Other.GetValue());
                Other.Reset();
            }
            else if (Other.HasValue())
            {
                Construct(Move(Other.GetValue()));
                Other.Reset();
            }
            else
            {
                Reset();
            }
        }

        return *this;
    }

    /**
     * @brief Move-assignment operator that takes another type
     * @param Other Optional to move from
     * @return Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE TOptional& operator=(TOptional<OtherType>&& Other) 
        requires(TIsConstructible<ElementType, typename TAddRValueReference<typename TRemoveReference<OtherType>::Type>::Type>::Value)
    {
        if (Other.HasValue())
        {
            if (HasValue())
            {
                GetValue() = Move(Other.GetValue());
                Other.Reset();
            }
            else
            {
                Construct(Move(Other.GetValue()));
                Other.Reset();
            }
        }
        else
        {
            Reset();
        }

        return *this;
    }

    /**
     * @brief Assignment operator that can take a r-value reference
     * @param Other Instance to move-construct value from
     * @return Returns a reference to this instance
     */
    template<typename OtherType = ElementType>
    FORCEINLINE TOptional& operator=(OtherType&& Other) 
        requires(TIsConstructible<ElementType, typename TAddRValueReference<typename TRemoveReference<OtherType>::Type>::Type>::Value)
    {
        if (HasValue())
        {
            GetValue() = Forward<OtherType>(Other);
        }
        else
        {
            Construct(Forward<OtherType>(Other));
        }

        return *this;
    }

    /**
     * @brief Operator that returns true if there is a stored value
     * @return Returns true if the optional has a stored value
     */
    NODISCARD FORCEINLINE operator bool() const
    {
        return HasValue();
    }

    /**
     * @brief Retrieve a pointer to the stored value
     * @return Returns a pointer to the stored value
     */
    NODISCARD FORCEINLINE ElementType* operator->()
    {
        CHECK(HasValue());
        return reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief Retrieve a pointer to the stored value
     * @return Returns a pointer to the stored value
     */
    NODISCARD FORCEINLINE const ElementType* operator->() const
    {
        CHECK(HasValue());
        return reinterpret_cast<const ElementType*>(Value.Data);
    }

    /**
     * @brief Retrieve a reference to the stored value
     * @return Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE ElementType& operator*()
    {
        CHECK(HasValue());
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief Retrieve a reference to the stored value
     * @return Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE const ElementType& operator*() const
    {
        CHECK(HasValue());
        return *reinterpret_cast<const ElementType*>(Value.Data);
    }

public:

    /**
     * @brief Comparison operator
     * @param LHS Left side to compare with
     * @param RHS Right side to compare with
     * @return Returns true if the values are equal
     */
    NODISCARD friend FORCEINLINE bool operator==(const TOptional& LHS, const TOptional& RHS)
    {
        if (!LHS.HasValue() && !RHS.HasValue())
        {
            return true;
        }

        if (!LHS.HasValue())
        {
            return false;
        }

        if (!RHS.HasValue())
        {
            return false;
        }

        return LHS.IsEqual(RHS);
    }

    /**
     * @brief Comparison operator 
     * @param LHS Left side to compare with
     * @param RHS Right side to compare with
     * @return Returns false if the values are equal
     */
    NODISCARD friend FORCEINLINE bool operator!=(const TOptional& LHS, const TOptional& RHS)
    {
        return !(LHS == RHS);
    }

    /**
     * @brief Less than comparison operator
     * @param LHS Left side to compare with
     * @param RHS Right side to compare with
     * @return Returns true if the LHS is less than RHS
     */
    NODISCARD friend FORCEINLINE bool operator<(const TOptional& LHS, const TOptional& RHS)
    {
        if (!LHS.HasValue() && !RHS.HasValue())
        {
            return false; // Neither is less than the other
        }

        if (!LHS.HasValue())
        {
            return true; // Typically, no value is considered less than any value
        }

        if (!RHS.HasValue())
        {
            return false;
        }

        return LHS.IsLessThan(RHS);
    }

    /**
     * @brief Less than or equal comparison operator
     * @param LHS Left side to compare with
     * @param RHS Right side to compare with
     * @return Returns true if the LHS is less than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator<=(const TOptional& LHS, const TOptional& RHS)
    {
        if (!LHS.HasValue() && !RHS.HasValue())
        {
            return true;
        }

        if (!LHS.HasValue())
        {
            return true; // Typically, no value is considered less than any value
        }

        if (!RHS.HasValue())
        {
            return false;
        }

        return LHS.IsLessThan(RHS) || LHS.IsEqual(RHS);
    }

    /**
     * @brief Greater than comparison operator
     * @param LHS Left side to compare with
     * @param RHS Right side to compare with
     * @return Returns true if LHS is greater than RHS
     */
    NODISCARD friend FORCEINLINE bool operator>(const TOptional& LHS, const TOptional& RHS)
    {
        return !(LHS <= RHS);
    }

    /**
     * @brief Greater than or equal comparison operator
     * @param LHS Left side to compare with
     * @param RHS Right side to compare with
     * @return Returns true if the LHS is greater than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator>=(const TOptional& LHS, const TOptional& RHS)
    {
        return !(LHS < RHS);
    }

private:
    template<typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args)
    {
        new(reinterpret_cast<void*>(Value.Data)) ElementType(Forward<ArgTypes>(Args)...);
        bHasValue = true;
    }

    FORCEINLINE void Destruct()
    {
        typedef ElementType ElementDestructType;
        reinterpret_cast<ElementDestructType*>(Value.Data)->~ElementDestructType();
        bHasValue = false;
    }

    template<typename OtherType>
    FORCEINLINE void CopyFrom(const TOptional<OtherType>& Other)
        requires(TIsConstructible<ElementType, const typename TRemoveReference<OtherType>::Type&>::Value)
    {
        if (Other.HasValue())
        {
            Construct(*reinterpret_cast<const OtherType*>(Other.Value.Data));
        }
    }

    template<typename OtherType>
    FORCEINLINE void MoveFrom(TOptional<OtherType>&& Other)
        requires(TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value)
    {
        if (Other.HasValue())
        {
            Construct(Move(*reinterpret_cast<OtherType*>(Other.Value.Data)));
            Other.Reset();
        }
    }

    NODISCARD FORCEINLINE bool IsEqual(const TOptional& RHS) const
    {
        return *reinterpret_cast<ElementType*>(Value.Data) == *reinterpret_cast<ElementType*>(RHS.Value.Data);
    }

    NODISCARD FORCEINLINE bool IsLessThan(const TOptional& RHS) const
    {
        return *reinterpret_cast<ElementType*>(Value.Data) < *reinterpret_cast<ElementType*>(RHS.Value.Data);
    }

    TTypeAlignedBytes<ElementType> Value;
    bool bHasValue{ false };
};
