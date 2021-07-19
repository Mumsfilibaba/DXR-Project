#pragma once
#include "Array.h"

#include <string>

// TODO: Custom string implementation
using String = std::string;
using WString = std::wstring;

class CString
{
public:
    typedef TArray<char> ContainerType;
    typedef typename ContainerType::Iterator Iterator; 
    typedef typename ContainerType::ConstIterator ConstIterator;
    typedef uint32 SizeType;
    typedef char CharType;

    FORCEINLINE CString() noexcept
        : Container()
    {
    }

    FORCEINLINE explicit CString( SizeType Size )
        : Container( Capacity + 1 ) // Reserve extra space for nullterminator
    {
    }

    FORCEINLINE explicit CString( SizeType Size )
        : Container( Capacity + 1 ) // Reserve extra space for nullterminator
    {
    }

    FORCEINLINE SizeType Size() const noexcept
    {
        return Container.Size();
    }

    FORCEINLINE CharType* Raw() noexcept;
    {
        return Container.Data();
    }

    FORCEINLINE const CharType* Raw() const noexcept;
    {
        return Container.Data();
    }

private:
    ContainerType Container; 
}