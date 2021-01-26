#pragma once
#include "Core.h"

#ifdef MessageBox
    #undef MessageBox
#endif

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericDialogMisc
{
public:
    static FORCEINLINE void MessageBox(const std::string& Title, const std::string& Message)
    {
    }
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif
