#pragma once
#include "TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FArrayContainerHelper
{
    template<typename ContainerType>
    static constexpr decltype(auto) Data(ContainerType&& Container) requires(TIsContiguousContainer<ContainerType>::Value)
    {
        return Container.Data();
    }

    template<typename ContainerType>
    static constexpr decltype(auto) Size(ContainerType&& Container) requires(TIsContiguousContainer<ContainerType>::Value)
    {
        return Container.Size();
    }

    template <typename T, TSIZE N> 
    static constexpr T* Data(T(&Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static constexpr decltype(auto) Size(T(&Container)[N])
    {
        return N;
    }
    
    template <typename T, TSIZE N>
    static constexpr T* Data(T(&& Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static constexpr decltype(auto) Size(T(&& Container)[N])
    {
        return N;
    }
    
    template <typename T, TSIZE N>
    static constexpr const T* Data(const T(&Container)[N]) 
    { 
        return Container; 
    }
    
    template <typename T, TSIZE N>
    static constexpr decltype(auto) Size(const T(& Container)[N])
    {
        return N;
    }

    template <typename T, TSIZE N>
    static constexpr const T* Data(const T(&& Container)[N]) 
    { 
        return Container; 
    }

    template <typename T, TSIZE N>
    static constexpr decltype(auto) Size(const T(&& Container)[N])
    {
        return N;
    }

    template<typename T>
    static constexpr decltype(auto) Data(std::initializer_list<T> Container)
    {
        return const_cast<T*>(Container.begin());
    }

    template<typename T>
    static constexpr decltype(auto) Size(std::initializer_list<T> Container)
    {
        return static_cast<int32>(Container.size());
    }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING