#pragma once
#include "Core/Templates/TypeTraits.h"

template<typename InComInterfaceType>
class TComPtr
{
public:
    typedef InComInterfaceType ElementType;

    template<typename OtherType>
    friend class TComPtr;

    /** @brief Default constructor that sets the pointer to nullptr */
    TComPtr() = default;

    /**
     * @brief Copy-constructor 
     * @param Other ComPtr to copy from
     */
    FORCEINLINE TComPtr(const TComPtr& Other)
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /**
     * @brief Copy-constructor that copies from a ComPtr of a convertible type
     * @param Other ComPtr to copy from
     */
    template<typename OtherType>
    FORCEINLINE TComPtr(const TComPtr<OtherType>& Other) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
        : Ptr(Other.Ptr)
    {
        AddRef();
    }

    /**
     * @brief Move-constructor
     * @param Other ComPtr to move from
     */
    FORCEINLINE TComPtr(TComPtr&& Other)
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief Move-constructor that moves from a ComPtr of a convertible type
     * @param Other ComPtr to move from
     */
    template<typename OtherType>
    FORCEINLINE TComPtr(TComPtr<OtherType>&& Other) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
        : Ptr(Other.Ptr)
    {
        Other.Ptr = nullptr;
    }

    /**
     * @brief Construct a ComPtr from a raw pointer. The container takes ownership.
     * @param InPointer Pointer to reference
     */
    FORCEINLINE TComPtr(ElementType* InPointer)
        : Ptr(InPointer)
    {
    }

    /**
     * @brief Construct a ComPtr from a raw pointer of a convertible type. The container takes ownership.
     * @param InPointer Pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE TComPtr(OtherType* InPointer) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
        : Ptr(InPointer)
    {
    }

    /**
     * @brief Default destructor
     */
    FORCEINLINE ~TComPtr()
    {
        Release();
    }

    /**
     * @brief Resets the container and sets to a potential new raw pointer 
     * @param NewPtr New pointer to reference
     */
    FORCEINLINE void Reset(ElementType* NewPtr = nullptr)
    {
        TComPtr(NewPtr).Swap(*this);
    }

    /**
     * @brief Resets the container and sets to a new raw pointer from a convertible type
     * @param NewPtr New pointer to reference
     */
    template<typename OtherType>
    FORCEINLINE void Reset(OtherType* NewPtr) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        Reset(static_cast<ElementType*>(NewPtr));
    }

    /**
     * @brief Swaps the pointers in the two containers 
     * @param Other ComPtr to swap with
     */
    FORCEINLINE void Swap(TComPtr& Other)
    {
        ElementType* Temp = Ptr;
        Ptr = Other.Ptr;
        Other.Ptr = Temp; 
    }

    /**
     * @brief Releases the ownership of the pointer and returns the pointer
     * @return Returns the pointer that was previously held by the container
     */
    NODISCARD FORCEINLINE ElementType* ReleaseOwnership()
    {
        ElementType* OldPtr = Ptr;
        Ptr = nullptr;
        return OldPtr;
    }

    /**
     * @brief Adds a reference to the stored pointer 
     */
    FORCEINLINE void AddRef()
    {
        if (Ptr)
        {
            Ptr->AddRef();
        }
    }

    /**
     * @brief Retrieve the raw pointer 
     * @return Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* Get() const
    {
        return Ptr;
    }

    /**
     * @brief Retrieve the current reference count of the object. The object needs to be valid.
     * @return The current reference count of the stored pointer
     */
    NODISCARD FORCEINLINE uint64 GetRefCount() const
    {
        CHECK(IsValid());

        // There are no functions to retrieve the refcount for COM-Objects, add a ref and then release it
        Ptr->AddRef();
        return Ptr->Release();
    }

    /**
     * @brief Retrieve the raw pointer and add a reference 
     * @return Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* GetAndAddRef()
    {
        AddRef();
        return Ptr;
    }

    /**
     * @brief Releases the objects and returns the address of the stored pointer
     * @return Pointer to the stored pointer
     */
    NODISCARD FORCEINLINE ElementType** ReleaseAndGetAddressOf()
    {
        Ptr->Release();
        return &Ptr;
    }

    /** 
     * @brief Retrieve the pointer as another type that is convertible by querying the interface type
     * @param NewPointer A pointer to store the result in
     * @return The result of the operation
     */
    template<typename CastType>
    FORCEINLINE HRESULT GetAs(CastType** NewPointer)
    {
        return Ptr->QueryInterface(IID_PPV_ARGS(NewPointer));
    }

    /**
     * @brief Retrieve the pointer as another type that is convertible by querying the interface type
     * @param Riid The IID of the type to query
     * @param NewPointer A ComPtr of unknown type to store the result in
     * @return The result of the operation
     */
    FORCEINLINE HRESULT GetAs(REFIID Riid, TComPtr<IUnknown>* ComObject)
    {
        TComPtr<IUnknown> NewPointer;

        HRESULT Result = Ptr->QueryInterface(Riid, &NewPointer);
        if (SUCCEEDED(Result))
        {
            *ComObject = NewPointer;
        }

        return Result;
    }

    /**
     * @brief Get the address of the raw pointer 
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** GetAddressOf()
    {
        return &Ptr;
    }

    /**
     * @brief Get the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* GetAddressOf() const
    {
        return &Ptr;
    }

    /**
     * @brief Checks whether the pointer is valid or not 
     * @return True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return (Ptr != nullptr);
    }

public:

    /**
     * @brief Retrieve the raw pointer
     * @return Returns the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* operator->() const
    {
        return Get();
    }

    /**
     * @brief Get the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType** operator&() 
    {
        return GetAddressOf();
    }

    /**
     * @brief Get the address of the raw pointer
     * @return The address of the raw pointer
     */
    NODISCARD FORCEINLINE ElementType* const* operator&() const
    {
        return GetAddressOf();
    }

    /**
     * @brief Dereference the stored pointer
     * @return A reference to the object pointed to by the pointer
     */
    NODISCARD FORCEINLINE ElementType& operator*() const
    {
        return *Ptr;
    }

    /**
     * @brief Checks whether the pointer is valid or not
     * @return True if the pointer is not nullptr otherwise false
     */
    NODISCARD FORCEINLINE operator bool() const
    {
        return IsValid();
    }

    /**
     * @brief Copy-assignment operator
     * @param Other Instance to copy from
     * @return A reference to this object
     */
    FORCEINLINE TComPtr& operator=(const TComPtr& Other)
    {
        TComPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Copy-assignment operator that takes a convertible type
     * @param Other Instance to copy from
     * @return A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE TComPtr& operator=(const TComPtr<OtherType>& Other) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        TComPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Move-assignment operator
     * @param Other Instance to move from
     * @return A reference to this object
     */
    FORCEINLINE TComPtr& operator=(TComPtr&& Other)
    {
        TComPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Move-assignment operator that takes a convertible type
     * @param Other Instance to move from
     * @return A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE TComPtr& operator=(TComPtr<OtherType>&& Other) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        TComPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Assignment operator that takes a raw pointer
     * @param Other Pointer to store
     * @return A reference to this object
     */
    FORCEINLINE TComPtr& operator=(ElementType* Other)
    {
        TComPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Assignment operator that takes a raw pointer of a convertible type
     * @param Other Pointer to store
     * @return A reference to this object
     */
    template<typename OtherType>
    FORCEINLINE TComPtr& operator=(OtherType* Other) requires(TIsPointerConvertible<OtherType, ElementType>::Value)
    {
        TComPtr(Other).Swap(*this);
        return *this;
    }

    /**
     * @brief Set the pointer to nullptr 
     * @return A reference to this object
     */
    FORCEINLINE TComPtr& operator=(nullptr_type)
    {
        TComPtr().Swap(*this);
        return *this;
    }

private:
    FORCEINLINE void Release()
    {
        if (Ptr)
        {
            Ptr->Release();
            Ptr = nullptr;
        }
    }

    ElementType* Ptr{nullptr};
};

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TComPtr<T>& LHS, U* Other)
{
    return LHS.Get() == Other;
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(T* LHS, const TComPtr<U>& Other)
{
    return LHS == Other.Get();
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TComPtr<T>& LHS, U* Other)
{
    return LHS.Get() != Other;
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(T* LHS, const TComPtr<U>& Other)
{
    return LHS != Other.Get();
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator==(const TComPtr<T>& LHS, const TComPtr<U>& Other)
{
    return LHS.Get() == Other.Get();
}

template<typename T, typename U>
NODISCARD FORCEINLINE bool operator!=(const TComPtr<T>& LHS, const TComPtr<U>& Other)
{
    return LHS.Get() != Other.Get();
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(const TComPtr<T>& LHS, nullptr_type)
{
    return LHS.Get() == nullptr;
}

template<typename T>
NODISCARD FORCEINLINE bool operator==(nullptr_type, const TComPtr<T>& Other)
{
    return nullptr == Other.Get();
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(const TComPtr<T>& LHS, nullptr_type)
{
    return LHS.Get() != nullptr;
}

template<typename T>
NODISCARD FORCEINLINE bool operator!=(nullptr_type, const TComPtr<T>& Other)
{
    return nullptr != Other.Get();
}

/** 
 * @brief Converts a raw pointer into a TComPtr 
 */
template<typename T, typename U>
NODISCARD FORCEINLINE TComPtr<T> MakeComPtr(U* InRefCountedObject)
{
    if (InRefCountedObject)
    {
        InRefCountedObject->AddRef();
        return TComPtr<T>(static_cast<T*>(InRefCountedObject));
    }

    return nullptr;
}
