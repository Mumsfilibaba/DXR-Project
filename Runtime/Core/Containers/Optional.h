#pragma once
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

template<typename ElementType>
class TOptional
{
public:
    template<typename OtherType>
    friend class TOptional;

    /**
     * @brief - Default constructor
     */
    FORCEINLINE TOptional() noexcept
        : Value()
        , bHasValue(false)
    {
    }

    /**
     * @brief - Construct from nullptr
     */
    FORCEINLINE TOptional(nullptr_type) noexcept
        : Value()
        , bHasValue(false)
    {
    }

    /**
     * @brief       - Copy-constructor
     * @param Other - Optional to copy from
     */
    FORCEINLINE TOptional(const TOptional& Other) noexcept
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(*reinterpret_cast<ElementType*>(Other.Value.Data));
        }
    }

    /**
     * @brief       - Copy-constructor that takes another type
     * @param Other - Optional to copy from
     */
    template<
        typename OtherType,
        typename = typename TEnableIf<TIsConstructible<ElementType, const OtherType&>::Value>::Type>
    FORCEINLINE TOptional(const TOptional<OtherType>& Other)
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(*reinterpret_cast<OtherType*>(Other.Value.Data));
        }
    }

    /**
     * @brief       - Move-constructor
     * @param Other - Optional to move from
     */
    FORCEINLINE TOptional(TOptional&& Other) noexcept
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(::Move(*reinterpret_cast<ElementType*>(Other.Value.Data)));
            Other.Reset();
        }
    }

    /**
     * @brief       - Move-constructor taking another type
     * @param Other - Optional to move from
     */
    template<
        typename OtherType,
        typename = typename TEnableIf<TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value>::Type>
    FORCEINLINE TOptional(TOptional<OtherType>&& Other) noexcept
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(::Move<ElementType>(*reinterpret_cast<OtherType*>(Other.Value.Data)));
            Other.Reset();
        }
    }

    /**
     * @brief      - Constructor that creates a value in-place
     * @param Args - Arguments for the element to create
     */
    template<typename... ArgTypes>
    FORCEINLINE explicit TOptional(EInPlace, ArgTypes&&... Args) noexcept
        : Value()
        , bHasValue()
    {
        Construct(::Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief - Destructor
     */
    FORCEINLINE ~TOptional()
    {
        Reset();
    }

    /**
     * @brief  - Check if optional has a value assigned
     * @return - Returns true if there is a value assigned
     */
    NODISCARD FORCEINLINE bool HasValue() const noexcept
    {
        return bHasValue;
    }

    /**
     * @brief      - Emplace constructs a new element in the optional and destructs any old value
     * @param Args - Arguments for constructor
     * @return     - Returns a reference to the newly constructed value
     */
    template<typename... ArgTypes>
    FORCEINLINE ElementType& Emplace(ArgTypes&&... Args) noexcept
    {
        Reset();
        Construct(::Forward<ArgTypes>(Args)...);
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief - Resets the Optional and destructs any existing element
     */
    FORCEINLINE void Reset() noexcept
    {
        if (HasValue())
        {
            Destruct();
        }
    }

    /**
     * @brief       - Swap between this optional and another optional
     * @param Other - Optional to swap with
     */
    FORCEINLINE void Swap(TOptional& Other)
    {
        TOptional TempOptional(Move(*this));
        if (Other.HasValue())
        {
            Construct(::Move(Other.GetValue()));
            Other.Destruct();
        }

        if (TempOptional.HasValue())
        {
            Other.Construct(::Move(TempOptional.GetValue()));
            TempOptional.Destruct();
        }
    }

    /**
     * @brief  - Retrieve the optional value
     * @return - Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE ElementType& GetValue() noexcept
    {
        CHECK(HasValue());
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief  - Retrieve the optional value
     * @return - Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE const ElementType& GetValue() const noexcept
    {
        CHECK(HasValue());
        return *reinterpret_cast<const ElementType*>(Value.Data);
    }

    /**
     * @brief  - Try and retrieve the optional value, if no value is stored, it returns nullptr
     * @return - Returns a pointer to the stored value, or nullptr if no value is held
     */
    NODISCARD FORCEINLINE ElementType* TryGetValue() noexcept
    {
        return HasValue() ? reinterpret_cast<ElementType*>(Value.Data) : nullptr;
    }

    /**
     * @brief  - Try and retrieve the optional value, if no value is stored, it returns nullptr
     * @return - Returns a pointer to the stored value, or nullptr if no value is held
     */
    NODISCARD FORCEINLINE const ElementType* TryGetValue() const noexcept
    {
        return HasValue() ? reinterpret_cast<ElementType*>(Value.Data) : nullptr;
    }

    /**
     * @brief         - Retrieve the optional value
     * @param Default - Default value to return if a value is not set
     * @return        - Returns a reference to the stored value
     */
    template<typename OtherType>
    NODISCARD FORCEINLINE const ElementType& GetValueOrDefault(const OtherType& Default) const noexcept
    {
        return HasValue() ? *reinterpret_cast<const ElementType*>(Value.Data) : static_cast<const ElementType&>(Default);
    }

public:

    /**
     * @brief  - Nullptr assignment operator
     * @return - Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(nullptr_type) noexcept
    {
        Reset();
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator
     * @param Other - Optional to copy from
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(const TOptional& Other) noexcept
    {
        TOptional(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Copy-assignment operator that takes another type
     * @param Other - Optional to copy from
     * @return      - Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<
            TIsConstructible<ElementType, typename TAddRValueReference<typename TRemoveReference<OtherType>::Type>::Type>::Value,
            typename TAddLValueReference<TOptional>::Type
        >::Type operator=(const TOptional<OtherType>& Other) noexcept
    {
        TOptional(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator
     * @param Other - Optional to move from
     * @return      - Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(TOptional&& Other) noexcept
    {
        TOptional(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Move-assignment operator that takes another type
     * @param Other - Optional to move from
     * @return      - Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<
            TIsConstructible<ElementType, typename TAddRValueReference<typename TRemoveReference<OtherType>::Type>::Type>::Value,
            typename TAddLValueReference<TOptional>::Type
        >::Type operator=(TOptional<OtherType>&& Other) noexcept
    {
        TOptional(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief       - Assignment operator that can take a r-value reference
     * @param Other - Instance to move-construct value from
     * @return      - Returns a reference to this instance
     */
    template<typename OtherType = ElementType>
    FORCEINLINE typename TEnableIf<
            TIsConstructible<ElementType, typename TAddRValueReference<typename TRemoveReference<OtherType>::Type>::Type>::Value,
            typename TAddLValueReference<TOptional>::Type
        >::Type operator=(OtherType&& Other) noexcept
    {
        TOptional(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief  - Operator that returns true if there is a stored value
     * @return - Returns true if the optional has a stored value
     */
    NODISCARD FORCEINLINE operator bool() const noexcept
    {
        return HasValue();
    }

    /**
     * @brief  - Retrieve a pointer to the stored value
     * @return - Returns a pointer to the stored value
     */
    NODISCARD FORCEINLINE ElementType* operator->() noexcept
    {
        CHECK(HasValue());
        return reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief  - Retrieve a pointer to the stored value
     * @return - Returns a pointer to the stored value
     */
    NODISCARD FORCEINLINE const ElementType* operator->() const noexcept
    {
        CHECK(HasValue());
        return reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief  - Retrieve a reference to the stored value
     * @return - Returns a reference to the stored value
     */
    NODISCARD FORCEINLINE ElementType& operator*() noexcept
    {
        CHECK(HasValue());
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

    /**
     * @brief  - Retrieve a pointer to the stored value
     * @return - Returns a pointer to the stored value
     */
    NODISCARD FORCEINLINE const ElementType& operator*() const noexcept
    {
        CHECK(HasValue());
        return *reinterpret_cast<ElementType*>(Value.Data);
    }

public:

    /**
     * @brief     - Comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if the values are equal
     */
    NODISCARD friend FORCEINLINE bool operator==(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        if (!LHS.HasValue() && !RHS.HasValue())
        {
            return true;
        }

        if (!LHS.HasValue())
        {
            return false;
        }

        return LHS.IsEqual(RHS);
    }

    /**
     * @brief     - Comparison operator 
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns false if the values are equal
     */
    NODISCARD friend FORCEINLINE bool operator!=(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        return !(LHS != RHS);
    }

    /**
     * @brief     - Less than comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if the LHS is less than RHS
     */
    NODISCARD friend FORCEINLINE bool operator<(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        if (!LHS.HasValue() && !RHS.HasValue())
        {
            return true;
        }

        if (!LHS.HasValue())
        {
            return false;
        }

        return LHS.IsLessThan(RHS);
    }

    /**
     * @brief     - Less than or equal comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if the LHS is less than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator<=(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        if (!LHS.HasValue() && !RHS.HasValue())
        {
            return true;
        }

        if (!LHS.HasValue())
        {
            return false;
        }

        return LHS.IsLessThan(RHS) || LHS.IsEqual(RHS);
    }

    /**
     * @brief     - Great than comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if LHS is greater than RHS
     */
    NODISCARD friend FORCEINLINE bool operator>(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        return !(LHS <= RHS);
    }

    /**
     * @brief     - Greater than or equal comparison operator
     * @param LHS - Left side to compare with
     * @param RHS - Right side to compare with
     * @return    - Returns true if the LHS is greater than or equal to RHS
     */
    NODISCARD friend FORCEINLINE bool operator>=(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        return !(LHS < RHS);
    }

private:
    template<typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args) noexcept
    {
        new(reinterpret_cast<void*>(Value.Data)) ElementType(Forward<ArgTypes>(Args)...);
        bHasValue = true;
    }

    FORCEINLINE void Destruct() noexcept
    {
        typedef ElementType ElementDestructType;
        reinterpret_cast<ElementDestructType*>(Value.Data)->~ElementDestructType();
        bHasValue = false;
    }

    NODISCARD FORCEINLINE bool IsEqual(const TOptional& RHS) const noexcept
    {
        return (*reinterpret_cast<ElementType*>(Value.Data)) == (*reinterpret_cast<ElementType*>(RHS.Value.Data));
    }

    NODISCARD FORCEINLINE bool IsLessThan(const TOptional& RHS) const noexcept
    {
        return (*reinterpret_cast<ElementType*>(Value.Data)) < (*reinterpret_cast<ElementType*>(RHS.Value.Data));
    }

    TTypeAlignedBytes<ElementType> Value;
    bool bHasValue;
};
