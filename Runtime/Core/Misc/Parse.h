#pragma once 
#include "Core/Templates/CString.h"

struct FParse
{
    static FORCEINLINE void ParseLine(CHAR** Start)
    {
        CHAR* TempStart = *Start;
        while(*TempStart != '\0' && *TempStart != '\n')
            ++TempStart;

        *Start = TempStart;
    }

    static FORCEINLINE void ParseLine(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart != '\n')
            ++TempStart;

        *Start = TempStart;
    }

    static FORCEINLINE void ParseWhiteSpace(CHAR** Start)
    {
        CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart == ' ')
            ++TempStart;

        *Start = TempStart;
    }

    static FORCEINLINE void ParseWhiteSpace(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart == ' ')
            ++TempStart;

        *Start = TempStart;
    }

    static FORCEINLINE void ParseAlnum(CHAR** Start)
    {
        CHAR* TempStart = *Start;
        while (FCharTraits::IsAlnum(*TempStart))
            ++TempStart;

        *Start = TempStart;
    }

    static FORCEINLINE void ParseAlnum(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (FCharTraits::IsAlnum(*TempStart))
            ++TempStart;

        *Start = TempStart;
    }
};