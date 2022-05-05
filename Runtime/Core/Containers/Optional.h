#pragma once
#include "Core/Templates/AlignedStorage.h"
#include "Core/Templates/IsNullptr.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/IsConstructible.h"
#include "Core/Templates/RemoveReference.h"
#include "Core/Templates/InPlace.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TOptional - Similar to std::optional

template<typename T>
class TOptional
{
public:

    using ElementType = T;

    template<typename OtherType>
    friend class TOptional;

    /**
     * @brief: Default constructor
     */
    FORCEINLINE TOptional() noexcept
        : Value()
        , bHasValue(false)
    { }

    /**
     * @brief: Construct from nullptr
     */
    FORCEINLINE TOptional(NullptrType) noexcept
        : Value()
        , bHasValue(false)
    
    { }

    /**
     * @brief: Copy-constructor
     * 
     * @param Other: Optional to copy from
     */
    FORCEINLINE TOptional(const TOptional& Other) noexcept
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(*(Other.Value.GetStorage()));
            bHasValue = true;
        }
    }

    /**
     * @brief: Copy-constructor that takes another type
     *
     * @param Other: Optional to copy from
     */
    template<typename OtherType, typename = typename TEnableIf<TIsConstructible<ElementType, const OtherType&>::Value>::Type>
    FORCEINLINE TOptional(const TOptional<OtherType>& Other)
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(*(Other.Value.GetStorage()));
            bHasValue = true;
        }
    }

    /**
     * @brief: Move-constructor
     *
     * @param Other: Optional to move from
     */
    FORCEINLINE TOptional(TOptional&& Other) noexcept
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(Move(*Other.Value.GetStorage()));
            Other.Reset();
            bHasValue = true;
        }
    }

    /**
     * @brief: Move-constructor taking another type
     *
     * @param Other: Optional to move from
     */
    template<typename OtherType, typename = typename TEnableIf<TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value>::Type>
    FORCEINLINE TOptional(TOptional<OtherType>&& Other) noexcept
        : Value()
        , bHasValue(false)
    {
        if (Other)
        {
            Construct(Move<ElementType>(*(Other.Value.GetStorage())));
            Other.Reset();
            bHasValue = true;
        }
    }

    /**
     * @brief: Constructor that creates a value in-place
     * 
     * @param Args: Arguments for the element to create
     */
    template<typename... ArgTypes>
    FORCEINLINE explicit TOptional(EInPlace, ArgTypes&&... Args) noexcept
        : Value()
        , bHasValue(true)
    {
        Construct(Forward<ArgTypes>(Args)...);
    }

    /**
     * @brief: Destructor
     */
    FORCEINLINE ~TOptional()
    {
        Reset();
    }

    /**
     * @brief: Check if optional has a value assigned
     * 
     * @return: Returns true if there is a value assigned
     */
    FORCEINLINE bool HasValue() const noexcept
    {
        return bHasValue;
    }

    /**
     * @brief: Emplace constructs a new element in the optional and destructs any old value
     * 
     * @param Args: Arguments for constructor
     * @return: Returns a reference to the newly constructed value
     */
    template<typename... ArgTypes>
    FORCEINLINE ElementType& Emplace(ArgTypes&&... Args) noexcept
    {
        Reset();

        Construct(Forward<ArgTypes>(Args)...);

        bHasValue = true;

        return *Value.GetStorage();
    }

    /**
     * @brief: Resets the Optional and destructs any existing element
     */
    FORCEINLINE void Reset() noexcept
    {
        if (HasValue())
        {
            Destruct();
            bHasValue = false;
        }
    }

    /**
     * @brief: Swap between this optional and another optional
     * 
     * @param Other: Optional to swap with
     */
    FORCEINLINE void Swap(TOptional& Other)
    {
        TOptional TempOptional;
        if (HasValue())
        {
            TempOptional.Construct(Move(GetValue()));
            Destruct();
            TempOptional.bHasValue = true;
        }

        if (Other.HasValue())
        {
            Construct(Move(Other.GetValue()));
            Other.Destruct();
            bHasValue = true;
        }

        if (TempOptional.HasValue())
        {
            Other.Construct(Move(TempOptional.GetValue()));
            TempOptional.Destruct();
            Other.bHasValue = true;
        }
    }

    /**
     * @brief: Retrieve the optional value
     * 
     * @return: Returns a reference to the stored value
     */
    FORCEINLINE ElementType& GetValue() noexcept
    {
        Check(HasValue());
        return *Value.GetStorage();
    }

    /**
     * @brief: Retrieve the optional value
     *
     * @return: Returns a reference to the stored value
     */
    FORCEINLINE const ElementType& GetValue() const noexcept
    {
        Check(HasValue());
        return *Value.GetStorage();
    }

    /**
     * @brief: Try and retrieve the optional value, if no value is stored, it returns nullptr
     *
     * @return: Returns a pointer to the stored value, or nullptr if no value is held
     */
    FORCEINLINE ElementType* TryGetValue() noexcept
    {
        return HasValue() ? Value.GetStorage() : nullptr;
    }

    /**
     * @brief: Try and retrieve the optional value, if no value is stored, it returns nullptr
     *
     * @return: Returns a pointer to the stored value, or nullptr if no value is held
     */
    FORCEINLINE const ElementType* TryGetValue() const noexcept
    {
        return HasValue() ? Value.GetStorage() : nullptr;
    }

    /**
     * @brief: Retrieve the optional value
     *
     * @param Default: Default value to return if a value is not set
     * @return: Returns a reference to the stored value
     */
    template<typename OtherType>
    FORCEINLINE const ElementType& GetValueOrDefault(const OtherType& Default) const noexcept
    {
        return HasValue() ? *Value.GetStorage() : static_cast<const ElementType&>(Default);
    }

public:

    /**
     * @brief: Nullptr assignment operator
     * 
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(NullptrType) noexcept
    {
        Reset();
        return *this;
    }

    /**
     * @brief: Copy-assignment operator
     *
     * @param RHS: Optional to copy from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(const TOptional& RHS) noexcept
    {
        TOptional(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Copy-assignment operator that takes another type
     *
     * @param RHS: Optional to copy from
     * @return: Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value, TOptional&>::Type operator=(const TOptional<OtherType>& RHS) noexcept
    {
        TOptional(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator
     *
     * @param RHS: Optional to move from
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(TOptional&& RHS) noexcept
    {
        TOptional(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Move-assignment operator that takes another type
     *
     * @param RHS: Optional to move from
     * @return: Returns a reference to this instance
     */
    template<typename OtherType>
    FORCEINLINE typename TEnableIf<TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value, TOptional&>::Type operator=(TOptional<OtherType>&& RHS) noexcept
    {
        TOptional(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Assignment operator that can take a r-value reference
     *
     * @param RHS: Instance to move-construct value from
     * @return: Returns a reference to this instance
     */
    template<typename OtherType = T>
    FORCEINLINE typename TEnableIf<TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value, TOptional&>::Type operator=(OtherType&& RHS) noexcept
    {
        TOptional(RHS).Swap(*this);
        return *this;
    }

    /**
     * @brief: Operator that returns true if there is a stored value
     * 
     * @return: Returns true if the optional has a stored value
     */
    FORCEINLINE operator bool() const noexcept
    {
        return HasValue();
    }

    /**
     * @brief: Retrieve a pointer to the stored value
     *
     * @return: Returns a pointer to the stored value
     */
    FORCEINLINE T* operator->() noexcept
    {
        Check(HasValue());
        return Value.GetStorage();
    }

    /**
     * @brief: Retrieve a pointer to the stored value
     *
     * @return: Returns a pointer to the stored value
     */
    FORCEINLINE const T* operator->() const noexcept
    {
        Check(HasValue());
        return Value.GetStorage();
    }

    /**
     * @brief: Retrieve a reference to the stored value
     *
     * @return: Returns a reference to the stored value
     */
    FORCEINLINE T& operator*() noexcept
    {
        Check(HasValue());
        return *Value.GetStorage();
    }

    /**
     * @brief: Retrieve a pointer to the stored value
     *
     * @return: Returns a pointer to the stored value
     */
    FORCEINLINE const T& operator*() const noexcept
    {
        Check(HasValue());
        return *Value.GetStorage();
    }

public:

    /**
     * @brief: Comparison operator
     *
     * @param LHS: Left side to compare with
     * @param RHS: Right side to compare with
     * @return: Returns true if the values are equal
     */
    friend FORCEINLINE bool operator==(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        if (!LHS.bHasValue && !RHS.bHasValue)
        {
            return true;
        }

        if (!LHS.bHasValue)
        {
            return false;
        }

        return LHS.IsEqual(RHS);
    }

    /**
     * @brief: Comparison operator
     *
     * @param LHS: Left side to compare with
     * @param RHS: Right side to compare with
     * @return: Returns false if the values are equal
     */
    friend FORCEINLINE bool operator!=(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        return !(LHS != RHS);
    }

    /**
     * @brief: Less than comparison operator
     *
     * @param LHS: Left side to compare with
     * @param RHS: Right side to compare with
     * @return: Returns true if the LHS is less than RHS
     */
    friend FORCEINLINE bool operator<(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        if (!LHS.bHasValue && !RHS.bHasValue)
        {
            return true;
        }

        if (!LHS.bHasValue)
        {
            return false;
        }

        return LHS.IsLessThan(RHS);
    }

    /**
     * @brief: Less than or equal comparison operator
     *
     * @param LHS: Left side to compare with
     * @param RHS: Right side to compare with
     * @return: Returns true if the LHS is less than or equal to RHS
     */
    friend FORCEINLINE bool operator<=(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        if (!LHS.bHasValue && !RHS.bHasValue)
        {
            return true;
        }

        if (!LHS.bHasValue)
        {
            return false;
        }

        return LHS.IsLessThan(RHS) || LHS.IsEqual(RHS);
    }

    /**
     * @brief: Great than comparison operator
     *
     * @param LHS: Left side to compare with
     * @param RHS: Right side to compare with
     * @return: Returns true if LHS is greater than RHS
     */
    friend FORCEINLINE bool operator>(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        return !(LHS <= RHS);
    }

    /**
     * @brief: Greater than or equal comparison operator
     *
     * @param LHS: Left side to compare with
     * @param RHS: Right side to compare with
     * @return: Returns true if the LHS is greater than or equal to RHS
     */
    friend FORCEINLINE bool operator>=(const TOptional& LHS, const TOptional& RHS) noexcept
    {
        return !(LHS < RHS);
    }

private:

    template<typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args) noexcept
    {
        new(reinterpret_cast<void*>(Value.GetStorage())) ElementType(Forward<ArgTypes>(Args)...);
    }

    FORCEINLINE void Destruct() noexcept
    {
        typedef ElementType ElementDestructType;
        Value.GetStorage()->ElementDestructType::~ElementDestructType();
    }

    FORCEINLINE bool IsEqual(const TOptional& RHS) const noexcept
    {
        return (*Value.GetStorage()) == (*RHS.Value.GetStorage());
    }

    FORCEINLINE bool IsLessThan(const TOptional& RHS) const noexcept
    {
        return (*Value.GetStorage()) < (*RHS.Value.GetStorage());
    }

    TTypedStorage<ElementType> Value;
    bool                       bHasValue;
};
