#pragma once
#include "Core.h"

#include <string>

#ifdef OutputDebugString
    #undef OutputDebugString
#endif

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericDebugMisc
{
public:
    static FORCEINLINE void DebugBreak()
    {
    }

    static FORCEINLINE void OutputDebugString(const std::string& Message)
    {
    }

    static FORCEINLINE Bool IsDebuggerPresent()
    {
        return false;
    }
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif