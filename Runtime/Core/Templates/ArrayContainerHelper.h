#pragma once
#include "TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FArrayContainerHelper
{
    template<
        typename ContainerType,
        typename = typename TEnableIf<TIsContiguousContainer<ContainerType>::Value>::Type>
    static CONSTEXPR decltype(auto) Data(ContainerType&& Container)
    {
        return Container.Data();
    }

    template<
        typename ContainerType,
        typename = typename TEnableIf<TIsContiguousContainer<ContainerType>::Value>::Type>
    static CONSTEXPR decltype(auto) Size(ContainerType&& Container)
    {
        return Container.Size();
    }

    template <typename T, TSIZE N> 
    static CONSTEXPR T* Data(T(&Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) Size(T(&Container)[N])
    {
        return N;
    }
    
    template <typename T, TSIZE N>
    static CONSTEXPR T* Data(T(&& Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) Size(T(&& Container)[N])
    {
        return N;
    }
    
    template <typename T, TSIZE N>
    static CONSTEXPR const T* Data(const T(&Container)[N]) 
    { 
        return Container; 
    }
    
    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) Size(const T(& Container)[N])
    {
        return N;
    }

    template <typename T, TSIZE N>
    static CONSTEXPR const T* Data(const T(&& Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) Size(const T(&& Container)[N])
    {
        return N;
    }

    template<typename T>
    static CONSTEXPR decltype(auto) Data(std::initializer_list<T> Container)
    {
        return const_cast<T*>(Container.begin());
    }

    template<typename T>
    static CONSTEXPR decltype(auto) Size(std::initializer_list<T> Container)
    {
        return static_cast<int32>(Container.size());
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING