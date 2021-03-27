#pragma once
#include "Core.h"

#ifdef MessageBox
    #undef MessageBox
#endif

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class Misc
{
public:
    FORCEINLINE static void MessageBox(const std::string& Title, const std::string& Message)
    {
    }

    FORCEINLINE static void RequestExit(int32 ExitCode)
    {
    }

    FORCEINLINE static void DebugBreak()
    {
    }

    FORCEINLINE static void OutputDebugString(const std::string& Message)
    {
    }

    FORCEINLINE static bool IsDebuggerPresent()
    {
        return false;
    }
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif
