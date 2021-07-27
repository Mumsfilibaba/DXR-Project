#pragma once
#include "Array.h"

#include <string>

// TODO: Custom string implementation
using String = std::string;
using WString = std::wstring;

class CString
{
public:
    typedef char                                      CharType;
    typedef TArray<CharType>                          ContainerType;
    typedef typename ContainerType::SizeType          SizeType;
    typedef typename ContainerType::IteratorType      Iterator;
    typedef typename ContainerType::ConstIteratorType ConstIterator;

    FORCEINLINE CString() noexcept
        : Container()
    {
    }

    FORCEINLINE explicit CString( SizeType Size )
        : Container( Size + 1 ) // Reserve extra space for nullterminator
    {
    }

    FORCEINLINE SizeType Size() const noexcept
    {
        return Container.Size();
    }

    FORCEINLINE CharType* Raw() noexcept
    {
        return Container.Data();
    }

    FORCEINLINE const CharType* Raw() const noexcept
    {
        return Container.Data();
    }

private:
    ContainerType Container;
};