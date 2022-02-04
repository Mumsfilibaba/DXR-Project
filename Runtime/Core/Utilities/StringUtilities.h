#pragma once
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Convert from back-slashes to forward-slashes

inline void ConvertBackslashes(String& OutString)
{
    auto Position = OutString.Find('\\');
    while (Position != String::NPos)
    {
        OutString.Replace('/', Position);
        Position = OutString.Find('\\', Position + 1);
    }
}
