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
    static void DebugBreak()
    {
    }

    static void OutputDebugString(const std::string& Message)
    {
    }
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif