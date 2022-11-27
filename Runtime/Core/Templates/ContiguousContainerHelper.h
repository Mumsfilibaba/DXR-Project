#pragma once
#include "TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FContiguousContainerHelper
{
    template<
        typename ContainerType,
        typename = typename TEnableIf<TIsContiguousContainer<ContainerType>::Value>::Type>
    static CONSTEXPR decltype(auto) GetData(ContainerType&& Container)
    {
        return Container.GetData();
    }

    template<
        typename ContainerType,
        typename = typename TEnableIf<TIsContiguousContainer<ContainerType>::Value>::Type>
    static CONSTEXPR decltype(auto) GetSize(ContainerType&& Container)
    {
        return Container.GetSize();
    }

    template <typename T, TSIZE N> 
    static CONSTEXPR T* GetData(T(&Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) GetSize(T(&Container)[N])
    {
        return N;
    }
    
    template <typename T, TSIZE N>
    static CONSTEXPR T* GetData(T(&& Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) GetSize(T(&& Container)[N])
    {
        return N;
    }
    
    template <typename T, TSIZE N>
    static CONSTEXPR const T* GetData(const T(&Container)[N]) 
    { 
        return Container; 
    }
    
    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) GetSize(const T(& Container)[N])
    {
        return N;
    }

    template <typename T, TSIZE N>
    static CONSTEXPR const T* GetData(const T(&& Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static CONSTEXPR decltype(auto) GetSize(const T(&& Container)[N])
    {
        return N;
    }

    template<typename T>
    static CONSTEXPR decltype(auto) GetData(std::initializer_list<T> Container)
    {
        return const_cast<T*>(Container.begin());
    }

    template<typename T>
    static CONSTEXPR decltype(auto) GetSize(std::initializer_list<T> Container)
    {
        return static_cast<int32>(Container.size());
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING