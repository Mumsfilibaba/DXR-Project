#pragma once 
#include "Core/Templates/CString.h"

struct FLineStats
{
    int32 Total    = 0;
    int32 Blank    = 0;
    int32 NonBlank = 0;
};

struct Parse
{
    static FORCEINLINE void ParseLine(CHAR** Start)
    {
        CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart != '\n')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseLine(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart != '\n')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseWhiteSpace(CHAR** Start)
    {
        CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart == ' ')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseWhiteSpace(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart == ' ')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseAlnum(CHAR** Start)
    {
        CHAR* TempStart = *Start;
        while (CharTraits::IsAlnum(*TempStart))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseAlnum(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (CharTraits::IsAlnum(*TempStart))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseOptionName(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while (CharTraits::IsAlnum(*TempStart) || (*TempStart == '.') || (*TempStart == '_'))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseValue(const CHAR** Start)
    {
        const CHAR* TempStart = *Start;
        while ((*TempStart != '\0') && (*TempStart != ' '))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE FLineStats CountLines(const CHAR* Text)
    {
        FLineStats Stats;
        if ((Text == nullptr) || (*Text == '\0'))
        {
            return Stats;
        }

        const CHAR* Cursor = Text;
        while (*Cursor != '\0')
        {
            const CHAR* LineStart = Cursor;
            ParseLine(&Cursor);

            bool bBlank = true;
            for (const CHAR* It = LineStart; It < Cursor; ++It)
            {
                if ((*It != '\r') && !CharTraits::IsWhitespace(*It))
                {
                    bBlank = false;
                    break;
                }
            }

            ++Stats.Total;
            if (bBlank)
            {
                ++Stats.Blank;
            }
            else
            {
                ++Stats.NonBlank;
            }

            if (*Cursor == '\n')
            {
                ++Cursor;
            }
        }

        return Stats;
    }
};
