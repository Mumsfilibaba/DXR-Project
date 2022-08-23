#pragma once
#include "Core/Containers/String.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Convert from back-slashes to forward-slashes

inline void ConvertBackslashes(FString& OutString)
{
    auto Position = OutString.Find('\\');
    while (Position != FString::INVALID_INDEX)
    {
        OutString.Replace('/', Position);
        Position = OutString.Find('\\', Position + 1);
    }
}
