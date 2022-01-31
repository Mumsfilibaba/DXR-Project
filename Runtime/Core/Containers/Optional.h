#pragma once
#include "Core/Templates/AlignedStorage.h"
#include "Core/Templates/IsNullptr.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/IsConstructible.h"
#include "Core/Templates/RemoveReference.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TOptional - Similar to std::optional

template<typename T>
class TOptional
{
public:

    using ElementType = T;

    template<typename OtherType>
    class TOptional;

    /**
     * Default constructor
     */
    FORCEINLINE TOptional() noexcept
        : Value()
        , bHasValue(false)
    {
    }

    /**
     * Construct from nullptr
     */
    FORCEINLINE TOptional(NullptrType) noexcept
        : Value()
        , bHasValue(false)
    
    {
    }

    /**
     * Copy-constructor
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
     * Copy-constructor that takes another type
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
     * Move-constructor
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
     * Move-constructor taking another type
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
     * Constructor that creates a value in-place
     * 
     * @param Args: Arguments for the element to create
     */
    template<typename... ArgTypes>
    FORCEINLINE explicit TOptional(ArgTypes&&... Args) noexcept
        : Value()
        , bHasValue(true)
    {
        Construct(Forward<ArgTypes>(Args)...);
    }

    /**
     * Constructor constructing from a ElementType or a convertible type
     *
     * @param InValue: Value to move into this optional
     */
    template<typename OtherType = T, typename = typename TEnableIf<TIsConstructible<ElementType, typename TRemoveReference<OtherType>::Type&&>::Value>::Type>
    FORCEINLINE TOptional(OtherType&& InValue) noexcept
        : Value()
        , bHasValue(true)
    {
        Construct(Forward<OtherType>(InValue));
    }

    /**
     * Destructor
     */
    FORCEINLINE ~TOptional()
    {
        Reset();
    }

    /**
     * Check if optional has a value assigned
     * 
     * @return: Returns true if there is a value assigned
     */
    FORCEINLINE bool HasValue() const noexcept
    {
        return bHasValue;
    }

    /**
     * Emplace constructs a new element in the optional and destructs any old value
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

        return GetValue();
    }

    /**
     * Resets the Optional and destructs any existing element
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
     * Swap between this optional and another optional
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
     * Retrieve the optional value
     * 
     * @return: Returns a reference to the stored value
     */
    FORCEINLINE ElementType& GetValue() noexcept
    {
        Assert(HasValue());
        return *Value.GetStorage();
    }

    /**
     * Retrieve the optional value
     *
     * @return: Returns a reference to the stored value
     */
    FORCEINLINE const ElementType& GetValue() const noexcept
    {
        Assert(HasValue());
        return *Value.GetStorage();
    }

    /**
     * Retrieve the optional value
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
     * Nullptr assignment operator
     * 
     * @return: Returns a reference to this instance
     */
    FORCEINLINE TOptional& operator=(NullptrType) noexcept
    {
        Reset();
        return *this;
    }

    /**
     * Copy-assignment operator
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
     * Copy-assignment operator that takes another type
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
     * Move-assignment operator
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
     * Move-assignment operator that takes another type
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
     * Assignment operator that can take a r-value reference
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
     * Operator that returns true if there is a stored value
     * 
     * @return: Returns true if the optional has a stored value
     */
    FORCEINLINE operator bool() const noexcept
    {
        return HasValue();
    }

    /**
     * Comparison operator
     *
     * @param RHS: Optional to compare with
     * @return: Returns true if the values are equal
     */
    FORCEINLINE bool operator==(const TOptional& RHS) const noexcept
    {
        if (!bHasValue && !RHS.bHasValue)
        {
            return true;
        }

        if (!bHasValue)
        {
            return false;
        }

        return (GetValue() == RHS.GetValue());
    }

    /**
     * Comparison operator
     *
     * @param RHS: Optional to compare with
     * @return: Returns false if the values are equal
     */
    FORCEINLINE bool operator!=(const TOptional& RHS) const noexcept
    {
        return !(*this != RHS);
    }

    /**
     * Retrieve a pointer to the stored value
     *
     * @return: Returns a pointer to the stored value
     */
    FORCEINLINE T* operator->() noexcept
    {
        Assert(HasValue());
        return Value.GetStorage();
    }

    /**
     * Retrieve a pointer to the stored value
     *
     * @return: Returns a pointer to the stored value
     */
    FORCEINLINE const T* operator->() const noexcept
    {
        Assert(HasValue());
        return Value.GetStorage();
    }

    /**
     * Retrieve a reference to the stored value
     *
     * @return: Returns a reference to the stored value
     */
    FORCEINLINE T& operator*() noexcept
    {
        Assert(HasValue());
        return *Value.GetStorage();
    }

    /**
     * Retrieve a pointer to the stored value
     *
     * @return: Returns a pointer to the stored value
     */
    FORCEINLINE const T& operator*() const noexcept
    {
        Assert(HasValue());
        return *Value.GetStorage();
    }

private:

    template<typename... ArgTypes>
    FORCEINLINE void Construct(ArgTypes&&... Args)
    {
        new(reinterpret_cast<void*>(Value.GetStorage())) ElementType(Forward<ArgTypes>(Args)...);
    }

    FORCEINLINE void Destruct()
    {
        typedef ElementType ElementDestructType;
        Value.GetStorage()->ElementDestructType::~ElementDestructType();
    }

    /** Bytes to store the value in */
    TTypedStorage<ElementType> Value;
    /** Flag if there is a value or not */
    bool bHasValue;
};