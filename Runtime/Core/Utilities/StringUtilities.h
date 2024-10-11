#pragma once
#include "Core/Containers/String.h"

inline void ConvertBackslashes(FString& OutString)
{
    int32 Position = OutString.FindChar('\\');
    while (Position != FString::INVALID_INDEX)
    {
        OutString.Replace('/', Position);
        Position = OutString.FindChar('\\', Position + 1);
    }
}

