#pragma once
#include "Core/Containers/String.h"

inline void ConvertBackslashes(FString& OutString)
{
    auto Position = OutString.FindChar('\\');
    while (Position != FString::INVALID_INDEX)
    {
        OutString.Replace('/', Position);
        Position = OutString.FindChar('\\', Position + 1);
    }
}
