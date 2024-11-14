#pragma once
#include "TypeTraits.h"
#include "Functional.h"
#include "Utility.h"

template<typename T>
class TReferenceWrapper
{
public:
    typedef T Type;

    TReferenceWrapper(const TReferenceWrapper&) = default;
    TReferenceWrapper& operator=(const TReferenceWrapper&) = default;
    TReferenceWrapper& operator=(TReferenceWrapper&&) = default;

    static_assert(TIsObject<T>::Value || TIsFunction<T>::Value, "TReferenceWrapper requires T to be of object or function type");

    /**
     * @brief    - Construct a new reference-wrapper from a reference
     * @param In - The reference to store
     */
    FORCEINLINE TReferenceWrapper(Type& In)
        : Pointer(::AddressOf(In))
    {
    }

    /**
     * @brief  - Retrieve reference
     * @return - Returns the stored reference
     */
    FORCEINLINE Type& Get() const noexcept
    {
        return *Pointer;
    }

    /**
     * @brief  - Retrieve the raw pointer 
     * @return - Retrieve the address of the stored reference
     */
    FORCEINLINE Type* AddressOf() const noexcept
    {
        return Pointer;
    }

    /**
     * @brief  - Retrieve reference
     * @return - Returns the stored reference
     */
    FORCEINLINE operator Type& () const noexcept
    {
        return Get();
    }

    /**
     * @brief      - Invoke if type is invokable 
     * @param Args - Arguments to call
     * @return     - Returns the return-value
     */
    template<typename... ArgTypes>
    FORCEINLINE auto operator()(ArgTypes&&... Args) const noexcept -> decltype(Invoke(this->Get(), Forward<ArgTypes>(Args)...))
    {
        return Invoke(this->Get(), Forward<ArgTypes>(Args)...);
    }

private:
    Type* Pointer;
};
