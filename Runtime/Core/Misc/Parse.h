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
    static FORCEINLINE void ParseLine(CHAR_T** Start)
    {
        CHAR_T* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart != '\n')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseLine(const CHAR_T** Start)
    {
        const CHAR_T* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart != '\n')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseWhiteSpace(CHAR_T** Start)
    {
        CHAR_T* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart == ' ')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseWhiteSpace(const CHAR_T** Start)
    {
        const CHAR_T* TempStart = *Start;
        while (*TempStart != '\0' && *TempStart == ' ')
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseAlnum(CHAR_T** Start)
    {
        CHAR_T* TempStart = *Start;
        while (CharTraits::IsAlnum(*TempStart))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseAlnum(const CHAR_T** Start)
    {
        const CHAR_T* TempStart = *Start;
        while (CharTraits::IsAlnum(*TempStart))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseOptionName(const CHAR_T** Start)
    {
        const CHAR_T* TempStart = *Start;
        while (CharTraits::IsAlnum(*TempStart) || (*TempStart == '.') || (*TempStart == '_'))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE void ParseValue(const CHAR_T** Start)
    {
        const CHAR_T* TempStart = *Start;
        while ((*TempStart != '\0') && (*TempStart != ' '))
        {
            ++TempStart;
        }

        *Start = TempStart;
    }

    static FORCEINLINE FLineStats CountLines(const CHAR_T* Text)
    {
        FLineStats Stats;
        if ((Text == nullptr) || (*Text == '\0'))
        {
            return Stats;
        }

        const CHAR_T* Cursor = Text;
        while (*Cursor != '\0')
        {
            const CHAR_T* LineStart = Cursor;
            ParseLine(&Cursor);

            bool bBlank = true;
            for (const CHAR_T* It = LineStart; It < Cursor; ++It)
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
