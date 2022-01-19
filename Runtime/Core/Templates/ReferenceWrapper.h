#pragma once
#include "IsObject.h"
#include "IsFunction.h"
#include "Invoke.h"
#include "AddressOf.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Wrapper for a reference making them copyable

template<typename T>
class TReferenceWrapper
{
public:
    typedef T Type;

    TReferenceWrapper(const TReferenceWrapper&) = default;
    TReferenceWrapper(TReferenceWrapper&&) = default;
    TReferenceWrapper& operator=(const TReferenceWrapper&) = default;
    TReferenceWrapper& operator=(TReferenceWrapper&&) = default;

    static_assert(TIsObject<T>::Value || TIsFunction<T>::Value, "TReferenceWrapper requires T to be of object or function type");

    /* Constructor */
    FORCEINLINE TReferenceWrapper(Type& In)
        : Pointer(::AddressOf(In))
    {
    }

    TReferenceWrapper(Type&&) = delete;

    /* Retrieve reference */
    FORCEINLINE Type& Get() const noexcept
    {
        return *Pointer;
    }

    /* Retrieve the raw pointer */
    FORCEINLINE Type* AddressOf() const noexcept
    {
        return Pointer;
    }

    /* Retrieve reference */
    FORCEINLINE operator Type& () const noexcept
    {
        return Get();
    }

    /* Invoke if type is invokable */
    template<typename... ArgTypes>
    FORCEINLINE auto operator()(ArgTypes&&... Args) const noexcept
        -> decltype(Invoke(this->Get(), Forward<ArgTypes>(Args)...))
    {
        return Invoke(this->Get(), Forward<ArgTypes>(Args)...);
    }

private:
    Type* Pointer;
};
